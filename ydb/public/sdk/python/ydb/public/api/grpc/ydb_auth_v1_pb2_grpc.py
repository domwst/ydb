# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

from ydb.public.api.protos import ydb_auth_pb2 as ydb_dot_public_dot_api_dot_protos_dot_ydb__auth__pb2


class AuthServiceStub(object):
    """Missing associated documentation comment in .proto file."""

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.Login = channel.unary_unary(
                '/Ydb.Auth.V1.AuthService/Login',
                request_serializer=ydb_dot_public_dot_api_dot_protos_dot_ydb__auth__pb2.LoginRequest.SerializeToString,
                response_deserializer=ydb_dot_public_dot_api_dot_protos_dot_ydb__auth__pb2.LoginResponse.FromString,
                )


class AuthServiceServicer(object):
    """Missing associated documentation comment in .proto file."""

    def Login(self, request, context):
        """Perform login using built-in auth system
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_AuthServiceServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'Login': grpc.unary_unary_rpc_method_handler(
                    servicer.Login,
                    request_deserializer=ydb_dot_public_dot_api_dot_protos_dot_ydb__auth__pb2.LoginRequest.FromString,
                    response_serializer=ydb_dot_public_dot_api_dot_protos_dot_ydb__auth__pb2.LoginResponse.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'Ydb.Auth.V1.AuthService', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class AuthService(object):
    """Missing associated documentation comment in .proto file."""

    @staticmethod
    def Login(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/Ydb.Auth.V1.AuthService/Login',
            ydb_dot_public_dot_api_dot_protos_dot_ydb__auth__pb2.LoginRequest.SerializeToString,
            ydb_dot_public_dot_api_dot_protos_dot_ydb__auth__pb2.LoginResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)
