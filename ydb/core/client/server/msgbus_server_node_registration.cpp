#include "msgbus_servicereq.h"

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/interconnect/interconnect.h>
#include <ydb/core/base/appdata.h>
#include <ydb/core/base/tablet_pipe.h>
#include <ydb/core/mind/node_broker.h>
#include <ydb/core/kqp/kqp.h>

namespace NKikimr {
namespace NMsgBusProxy {

using namespace NKikimrNodeBroker;
using namespace NNodeBroker;

namespace {

class TNodeRegistrationActor : public TActorBootstrapped<TNodeRegistrationActor>, public TMessageBusSessionIdentHolder
{
    using TActorBase = TActorBootstrapped<TNodeRegistrationActor>;

public:
    static constexpr NKikimrServices::TActivity::EType ActorActivityType() {
        return NKikimrServices::TActivity::MSGBUS_COMMON;
    } 
 
    TNodeRegistrationActor(NKikimrClient::TNodeRegistrationRequest &request, NMsgBusProxy::TBusMessageContext &msg)
        : TMessageBusSessionIdentHolder(msg)
        , Request(request)
    {
    }

    void Bootstrap(const TActorContext &ctx)
    {
        auto dinfo = AppData(ctx)->DomainsInfo;
        ui32 group;

        if (Request.GetDomainPath()) {
            auto *domain = dinfo->GetDomainByName(Request.GetDomainPath());
            if (!domain) {
                auto error = Sprintf("Unknown domain %s", Request.GetDomainPath().data());
                ReplyWithErrorAndDie(error, ctx);
                return;
            }
            group = dinfo->GetDefaultStateStorageGroup(domain->DomainUid);
        } else {
            if (dinfo->Domains.size() > 1) {
                auto error = "Ambiguous domain (specify DomainPath in request)";
                ReplyWithErrorAndDie(error, ctx);
                return;
            }

            auto domain = dinfo->Domains.begin()->second;
            group = dinfo->GetDefaultStateStorageGroup(domain->DomainUid);
        }

        NTabletPipe::TClientConfig pipeConfig;
        pipeConfig.RetryPolicy = {.RetryLimitCount = 10};
        auto pipe = NTabletPipe::CreateClient(ctx.SelfID, MakeNodeBrokerID(group), pipeConfig);
        NodeBrokerPipe = ctx.RegisterWithSameMailbox(pipe);

        TAutoPtr<TEvNodeBroker::TEvRegistrationRequest> request
            = new TEvNodeBroker::TEvRegistrationRequest;
        request->Record.SetHost(Request.GetHost());
        request->Record.SetPort(Request.GetPort());
        request->Record.SetResolveHost(Request.GetResolveHost());
        request->Record.SetAddress(Request.GetAddress());
        request->Record.MutableLocation()->CopyFrom(Request.GetLocation());
        request->Record.SetFixedNodeId(Request.GetFixedNodeId());
        if (Request.HasPath()) {
            request->Record.SetPath(Request.GetPath());
        }
        NTabletPipe::SendData(ctx, NodeBrokerPipe, request.Release());

        Become(&TNodeRegistrationActor::MainState);
    }

    void Handle(TEvNodeBroker::TEvRegistrationResponse::TPtr &ev, const TActorContext &ctx)
    {
        auto &rec = ev->Get()->Record;

        if (rec.GetStatus().GetCode() != TStatus::OK) {
            ReplyWithErrorAndDie(rec.GetStatus().GetReason(), ctx);
            return;
        }

        Response.SetNodeId(rec.GetNode().GetNodeId());
        Response.SetExpire(rec.GetNode().GetExpire());
        Response.SetDomainPath(Request.GetDomainPath());
        Response.AddNodes()->CopyFrom(rec.GetNode());

        if (rec.HasScopeTabletId()) {
            Response.SetScopeTabletId(rec.GetScopeTabletId());
        }
        if (rec.HasScopePathId()) {
            Response.SetScopePathId(rec.GetScopePathId());
        }

        const TActorId nameserviceId = GetNameserviceActorId();
        ctx.Send(nameserviceId, new TEvInterconnect::TEvListNodes());
    }

    void Handle(TEvInterconnect::TEvNodesInfo::TPtr &ev, const TActorContext &ctx)
    {
        auto config = AppData(ctx)->DynamicNameserviceConfig;

        for (const auto &node : ev->Get()->Nodes) {
            // Copy static nodes only.
            if (!config || node.NodeId <= config->MaxStaticNodeId) {
                auto &info = *Response.AddNodes();
                info.SetNodeId(node.NodeId);
                info.SetHost(node.Host);
                info.SetAddress(node.Address);
                info.SetResolveHost(node.ResolveHost);
                info.SetPort(node.Port);
                node.Location.Serialize(info.MutableLocation());
            }
        }

        Response.MutableStatus()->SetCode(TStatus::OK);

        SendReplyAndDie(ctx);
    }

    void Undelivered(const TActorContext &ctx) {
        ReplyWithErrorAndDie("Node Broker is unavailable", ctx);
    }

    void Handle(TEvTabletPipe::TEvClientConnected::TPtr &ev, const TActorContext &ctx) noexcept
    {
        if (ev->Get()->Status != NKikimrProto::OK)
            Undelivered(ctx);
    }

    void Die(const TActorContext &ctx)
    {
        NTabletPipe::CloseClient(ctx, NodeBrokerPipe);
        TActorBase::Die(ctx);
    }

    void SendReplyAndDie(const TActorContext &ctx)
    {
        auto response = MakeHolder<TBusNodeRegistrationResponse>();
        response->Record = std::move(Response);
        SendReplyMove(response.Release());
        Die(ctx);
    }

    void ReplyWithErrorAndDie(const TString &error, const TActorContext &ctx)
    {
        Response.MutableStatus()->SetCode(TStatus::ERROR);
        Response.MutableStatus()->SetReason(error);
        SendReplyAndDie(ctx);
    }

    STFUNC(MainState) {
        switch (ev->GetTypeRewrite()) {
            CFunc(TEvents::TEvUndelivered::EventType, Undelivered);
            HFunc(TEvNodeBroker::TEvRegistrationResponse, Handle);
            HFunc(TEvInterconnect::TEvNodesInfo, Handle);
            CFunc(TEvTabletPipe::EvClientDestroyed, Undelivered);
            HFunc(TEvTabletPipe::TEvClientConnected, Handle);
        }
    }

private:
    NKikimrClient::TNodeRegistrationRequest Request;
    NKikimrClient::TNodeRegistrationResponse Response;
    TActorId NodeBrokerPipe;
};

} // namespace

IActor *CreateMessageBusRegisterNode(NMsgBusProxy::TBusMessageContext &msg) {
    NKikimrClient::TNodeRegistrationRequest &record
        = static_cast<TBusNodeRegistrationRequest*>(msg.GetMessage())->Record;
    return new TNodeRegistrationActor(record, msg);
}

} // namespace NMsgBusProxy
} // namespace NKikimr
