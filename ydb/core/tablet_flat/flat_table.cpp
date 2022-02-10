#include "flat_table.h"
#include "flat_row_celled.h"
#include "flat_row_remap.h"
#include "flat_bloom_hash.h"
#include "flat_part_iter_multi.h"
#include "flat_part_laid.h"
#include "flat_part_shrink.h"
#include "flat_part_charge.h"
#include "flat_part_dump.h"
#include "flat_range_cache.h"
#include "flat_util_misc.h"
#include "util_fmt_abort.h"

#include <ydb/core/util/yverify_stream.h>

namespace NKikimr {
namespace NTable {

TTable::TTable(TEpoch epoch) : Epoch(epoch) { }

TTable::~TTable() { }

void TTable::SetScheme(const TScheme::TTableInfo &table)
{
    Snapshot();

    Levels.Reset();
    ErasedKeysCache.Reset();

    Y_VERIFY(!Mutable && table.Columns);

    auto to = TRowScheme::Make(table.Columns, NUtil::TSecond());

    if (auto was = std::exchange(Scheme, to))
        was->CheckCompatability(*Scheme);

    /* This restriction is required for external blobs inverted index, for
        details read NPage::TFrames and NFwd blobs cache implementation. */

    Y_VERIFY(Scheme->Cols.size() <= ui32(-Min<i16>()), "Too many columns in row");

    EraseCacheEnabled = table.EraseCacheEnabled;
    EraseCacheConfig = { };
    if (table.EraseCacheMinRows) {
        EraseCacheConfig.MinRows = table.EraseCacheMinRows;
    }
    if (table.EraseCacheMaxBytes) {
        EraseCacheConfig.MaxBytes = table.EraseCacheMaxBytes;
    }
}

TIntrusiveConstPtr<TRowScheme> TTable::GetScheme() const noexcept
{
    return Scheme;
}

TAutoPtr<TSubset> TTable::Subset(TArrayRef<const TLogoBlobID> bundle, TEpoch head)
{
    head = Min(head, Epoch);

    TAutoPtr<TSubset> subset = new TSubset(head, Scheme);

    if (head > TEpoch::Zero()) {
        for (auto &x : Frozen) {
            if (x->Epoch < head) {
                subset->Frozen.emplace_back(x, x->Immediate());
            }
        }
        for (const auto &pr : TxStatus) {
            if (pr.second->Epoch < head) {
                subset->TxStatus.emplace_back(pr.second);
            }
        }
    }

    subset->Flatten.reserve(bundle.size());
    for (const TLogoBlobID &token : bundle) {
        if (auto* c = ColdParts.FindPtr(token)) {
            subset->ColdParts.push_back(*c);
            continue;
        }
        auto* p = Flatten.FindPtr(token);
        Y_VERIFY_S(p, "Cannot find part " << token);
        subset->Flatten.push_back(*p);
    }

    subset->CommittedTransactions = CommittedTransactions;
    subset->RemovedTransactions = RemovedTransactions;

    return subset;
}

TAutoPtr<TSubset> TTable::Subset(TEpoch head) const noexcept
{
    head = Min(head, Epoch);

    TAutoPtr<TSubset> subset = new TSubset(head, Scheme);

    for (const auto &it : TxStatus) {
        if (it.second->Epoch < head) {
            subset->TxStatus.emplace_back(it.second);
        }
    }

    for (auto &it: ColdParts)
        if (it.second->Epoch < head)
            subset->ColdParts.push_back(it.second);

    for (auto &it: Flatten)
        if (it.second->Epoch < head)
            subset->Flatten.push_back(it.second);

    for (auto &it : Frozen)
        if (it->Epoch < head)
            subset->Frozen.emplace_back(it, it->Immediate());

    // This method is normally used when we want to take some state snapshot
    // However it can still theoretically be used for iteration or compaction
    subset->CommittedTransactions = CommittedTransactions;
    subset->RemovedTransactions = RemovedTransactions;

    return subset;
}

TAutoPtr<TSubset> TTable::ScanSnapshot(TRowVersion snapshot) noexcept
{
    TAutoPtr<TSubset> subset = new TSubset(Epoch, Scheme);

    // TODO: we could filter LSM by the provided snapshot version, but it
    // cannot be a simple if condition since row versions may intersect in
    // non-trivial ways. E.g. update x may be on top of y, x < snapshot < y,
    // but we cannot drop y since x is actually visible. Only the guaranteed
    // invisible top layer (as sorted by epoch) may be excluded from subset.

    for (auto& it : ColdParts) {
        subset->ColdParts.push_back(it.second);
    }

    for (auto& it : Flatten) {
        subset->Flatten.push_back(it.second);
    }

    for (auto& it : Frozen) {
        subset->Frozen.emplace_back(it, it->Immediate());
    }

    if (Mutable && Mutable->GetMinRowVersion() <= snapshot) {
        subset->Frozen.emplace_back(Mutable, Mutable->Snapshot());
    }

    subset->CommittedTransactions = CommittedTransactions;

    return subset;
}

TAutoPtr<TSubset> TTable::Unwrap() noexcept
{
    Snapshot();

    auto subset = Subset(TEpoch::Max());

    Replace({ }, *subset);
    ReplaceTxStatus({ }, *subset);

    Y_VERIFY(!(Flatten || Frozen || Mutable || TxStatus));

    return subset;
}

TBundleSlicesMap TTable::LookupSlices(TArrayRef<const TLogoBlobID> bundles) const noexcept
{
    TBundleSlicesMap slices;
    for (const TLogoBlobID &bundle : bundles) {
        auto it = Flatten.find(bundle);
        if (it != Flatten.end()) {
            slices[bundle] = it->second.Slices;
        }
    }
    return slices;
}

void TTable::ReplaceSlices(TBundleSlicesMap slices) noexcept
{
    for (auto &kv : slices) {
        auto it = Flatten.find(kv.first);
        Y_VERIFY(it != Flatten.end(), "Got an unknown TPart in ReplaceSlices");
        Y_VERIFY(kv.second && *kv.second, "Got an empty TPart in ReplaceSlices");
        it->second.Slices = std::move(kv.second);
        it->second.Screen = it->second.Slices->ToScreen();
    }
    if (slices) {
        Levels.Reset();
        ErasedKeysCache.Reset();
    }
}

void TTable::Replace(TArrayRef<const TPartView> partViews, const TSubset &subset) noexcept
{
    for (const auto &partView : partViews) {
        Y_VERIFY(partView, "Replace(...) shouldn't get empty parts");
        Y_VERIFY(!partView.Screen, "Replace(...) shouldn't get screened parts");
        Y_VERIFY(partView.Slices && *partView.Slices, "Got parts without slices");
        if (Flatten.contains(partView->Label) || ColdParts.contains(partView->Label)) {
            Y_Fail("Duplicate bundle " << partView->Label);
        }
    }

    if (subset.Flatten) {
        Levels.Reset();
    }

    THashSet<ui64> checkNewTransactions;

    for (auto &memTable : subset.Frozen) {
        const auto found = Frozen.erase(memTable.MemTable);

        Y_VERIFY(found == 1, "Got an unknown TMemTable table in TSubset");

        NUtil::SubSafe(Stat_.FrozenWaste, memTable->GetWastedMem());
        NUtil::SubSafe(Stat_.FrozenSize, memTable->GetUsedMem());
        NUtil::SubSafe(Stat_.FrozenOps,  memTable->GetOpsCount());
        NUtil::SubSafe(Stat_.FrozenRows, memTable->GetRowCount());

        for (const auto &pr : memTable.MemTable->GetTxIdStats()) {
            const ui64 txId = pr.first;
            auto& tx = OpenTransactions[txId];
            bool removed = tx.Mem.erase(memTable.MemTable);
            Y_VERIFY(removed);
            if (tx.Mem.empty() && tx.Parts.empty()) {
                checkNewTransactions.insert(txId);
            }
        }
    }

    for (auto &part : subset.Flatten) {
        Y_VERIFY(part.Slices && *part.Slices,
            "Got an empty TPart subset in TSubset");

        auto it = Flatten.find(part->Label);
        Y_VERIFY(it != Flatten.end(), "Got an unknown TPart table in TSubset");
        auto& existing = it->second;

        Y_VERIFY(existing.Slices && *existing.Slices,
            "Existing table part has an unexpected empty bounds run");

        if (!TSlices::EqualByRowId(existing.Slices, part.Slices)) {
            if (!TSlices::SupersetByRowId(existing.Slices, part.Slices)) {
                Y_Fail("Removing unexpected subset " << NFmt::Do(*part.Slices)
                    << " from existing " << NFmt::Do(*existing.Slices));
            }

            auto left = TSlices::Subtract(existing.Slices, part.Slices);
            if (left->empty()) {
                Y_Fail("Empty result after removing " << NFmt::Do(*part.Slices)
                    << " from existing " << NFmt::Do(*existing.Slices));
            }

            existing.Slices = std::move(left);
            existing.Screen = existing.Slices->ToScreen();
            continue;
        }

        if (existing->TxIdStats) {
            for (const auto& item : existing->TxIdStats->GetItems()) {
                const ui64 txId = item.GetTxId();
                auto& tx = OpenTransactions[txId];
                bool removed = tx.Parts.erase(existing.Part);
                Y_VERIFY(removed);
                if (tx.Mem.empty() && tx.Parts.empty()) {
                    checkNewTransactions.insert(txId);
                }
            }
        }

        // Remove this part completely
        Flatten.erase(it);

        Stat_.Parts.Remove(part);
        if (!Stat_.PartsPerTablet[part->Label.TabletID()].Remove(part)) {
            Stat_.PartsPerTablet.erase(part->Label.TabletID());
        }
    }

    for (auto &part : subset.ColdParts) {
        auto it = ColdParts.find(part->Label);
        Y_VERIFY(it != ColdParts.end(), "Got an unknown TColdPart in TSubset");
        ColdParts.erase(it);
    }

    for (const auto &partView : partViews) {
        if (Mutable && partView->Epoch >= Mutable->Epoch) {
            Y_Fail("Replace with " << NFmt::Do(*partView) << " after mutable epoch " << Mutable->Epoch);
        }

        if (Frozen && partView->Epoch >= (*Frozen.begin())->Epoch) {
            Y_Fail("Replace with " << NFmt::Do(*partView) << " after frozen epoch " << (*Frozen.begin())->Epoch);
        }

        Epoch = Max(Epoch, partView->Epoch + 1);

        AddSafe(partView);
    }

    for (ui64 txId : checkNewTransactions) {
        auto it = OpenTransactions.find(txId);
        Y_VERIFY(it != OpenTransactions.end());
        auto& tx = it->second;
        if (tx.Mem.empty() && tx.Parts.empty()) {
            // Transaction no longer needs to be tracked
            if (!ColdParts) {
                CommittedTransactions.Remove(txId);
                RemovedTransactions.Remove(txId);
            } else {
                CheckTransactions.insert(txId);
            }
            OpenTransactions.erase(it);
        }
    }

    ProcessCheckTransactions();

    ErasedKeysCache.Reset();
}

void TTable::ReplaceTxStatus(TArrayRef<const TIntrusiveConstPtr<TTxStatusPart>> newTxStatus, const TSubset &subset) noexcept
{
    for (auto &part : subset.TxStatus) {
        Y_VERIFY(part, "Unexpected empty TTxStatusPart in TSubset");

        auto it = TxStatus.find(part->Label);
        Y_VERIFY(it != TxStatus.end());
        TxStatus.erase(it);
    }

    for (const auto& txStatus : newTxStatus) {
        if (Mutable && txStatus->Epoch >= Mutable->Epoch) {
            Y_Fail("Replace with " << NFmt::Do(*txStatus) << " after mutable epoch " << Mutable->Epoch);
        }

        if (Frozen && txStatus->Epoch >= (*Frozen.begin())->Epoch) {
            Y_Fail("Replace with " << NFmt::Do(*txStatus) << " after frozen epoch " << (*Frozen.begin())->Epoch);
        }

        Epoch = Max(Epoch, txStatus->Epoch + 1);

        auto res = TxStatus.emplace(txStatus->Label, txStatus);
        Y_VERIFY(res.second, "Unexpected failure to add a new TTxStatusPart");
    }
}

void TTable::Merge(TPartView partView) noexcept
{
    Y_VERIFY(partView, "Merge(...) shouldn't get empty part");
    Y_VERIFY(partView.Slices, "Merge(...) shouldn't get parts without slices");

    if (Mutable && partView->Epoch >= Mutable->Epoch) {
        Y_Fail("Merge " << NFmt::Do(*partView) << " after mutable epoch " << Mutable->Epoch);
    }

    if (Frozen && partView->Epoch >= (*Frozen.begin())->Epoch) {
        Y_Fail("Merge " << NFmt::Do(*partView) << " after frozen epoch " << (*Frozen.begin())->Epoch);
    }

    auto it = Flatten.find(partView->Label);

    if (it == Flatten.end()) {
        Epoch = Max(Epoch, partView->Epoch + 1);

        AddSafe(std::move(partView));
    } else if (it->second->Epoch != partView->Epoch) {
        Y_FAIL("Got the same labeled parts with different epoch");
    } else {
        Levels.Reset();
        it->second.Screen = TScreen::Join(it->second.Screen, partView.Screen);
        it->second.Slices = TSlices::Merge(it->second.Slices, partView.Slices);
    }

    ErasedKeysCache.Reset();
}

void TTable::Merge(TIntrusiveConstPtr<TColdPart> part) noexcept
{
    Y_VERIFY(part, "Merge(...) shouldn't get empty parts");

    if (Mutable && part->Epoch >= Mutable->Epoch) {
        Y_Fail("Merge " << NFmt::Do(*part) << " after mutable epoch " << Mutable->Epoch);
    }

    if (Frozen && part->Epoch >= (*Frozen.begin())->Epoch) {
        Y_Fail("Merge " << NFmt::Do(*part) << " after frozen epoch " << (*Frozen.begin())->Epoch);
    }

    auto it = Flatten.find(part->Label);
    Y_VERIFY_S(it == Flatten.end(), "Merge " << NFmt::Do(*part) << " when a loaded part already exists");

    auto itCold = ColdParts.find(part->Label);
    Y_VERIFY_S(itCold == ColdParts.end(), "Merge " << NFmt::Do(*part) << " when another cold part already exists");

    const auto label = part->Label;

    Epoch = Max(Epoch, part->Epoch + 1);
    ColdParts.emplace(label, std::move(part));

    ErasedKeysCache.Reset();
    Levels.Reset();
}

void TTable::Merge(TIntrusiveConstPtr<TTxStatusPart> txStatus) noexcept
{
    Y_VERIFY(txStatus, "Unexpected empty TTxStatusPart");

    for (auto& item : txStatus->TxStatusPage->GetCommittedItems()) {
        const ui64 txId = item.GetTxId();
        const auto rowVersion = item.GetRowVersion();
        if (const auto* prev = CommittedTransactions.Find(txId); Y_LIKELY(!prev) || *prev > rowVersion) {
            CommittedTransactions.Add(txId, rowVersion);
            if (!prev) {
                RemovedTransactions.Remove(txId);
            }
        }
        if (!OpenTransactions.contains(txId)) {
            CheckTransactions.insert(txId);
        }
    }
    for (auto& item : txStatus->TxStatusPage->GetRemovedItems()) {
        const ui64 txId = item.GetTxId();
        if (const auto* prev = CommittedTransactions.Find(txId); Y_LIKELY(!prev)) {
            RemovedTransactions.Add(txId);
        }
        if (!OpenTransactions.contains(txId)) {
            CheckTransactions.insert(txId);
        }
    }

    if (Mutable && txStatus->Epoch >= Mutable->Epoch) {
        Y_Fail("Merge " << NFmt::Do(*txStatus) << " after mutable epoch " << Mutable->Epoch);
    }

    if (Frozen && txStatus->Epoch >= (*Frozen.begin())->Epoch) {
        Y_Fail("Merge " << NFmt::Do(*txStatus) << " after frozen epoch " << (*Frozen.begin())->Epoch);
    }

    Epoch = Max(Epoch, txStatus->Epoch + 1);

    auto res = TxStatus.emplace(txStatus->Label, txStatus);
    Y_VERIFY(res.second, "Unexpected failure to add a new TTxStatusPart");
}

void TTable::ProcessCheckTransactions() noexcept
{
    if (!ColdParts) {
        for (ui64 txId : CheckTransactions) {
            auto it = OpenTransactions.find(txId);
            if (it == OpenTransactions.end()) {
                CommittedTransactions.Remove(txId);
                RemovedTransactions.Remove(txId);
            }
        }
        CheckTransactions.clear();
    }
}

const TLevels& TTable::GetLevels() const noexcept
{
    if (!Levels) {
        Y_VERIFY(ColdParts.empty(), "Cannot construct Levels with cold parts");
        TVector<const TPartView*> parts; // TPartView* avoids expensive atomic ops
        parts.reserve(Flatten.size());
        for (const auto& kv : Flatten) {
            parts.push_back(&kv.second);
        }
        std::sort(parts.begin(), parts.end(),
            [](const TPartView* a, const TPartView* b) {
                if (a->Part->Epoch != b->Part->Epoch) {
                    return a->Part->Epoch < b->Part->Epoch;
                }
                return a->Part->Label < b->Part->Label;
            });
        Levels.Reset(new TLevels(Scheme->Keys));
        for (const TPartView* p : parts) {
            Levels->Add(p->Part, p->Slices);
        }
    }
    return *Levels;
}

ui64 TTable::GetSearchHeight() const noexcept
{
    if (!ColdParts.empty())
        return 0;

    ui64 height = GetLevels().size() + Frozen.size();
    if (Mutable)
        ++height;

    return height;
}

TVector<TIntrusiveConstPtr<TMemTable>> TTable::GetMemTables() const noexcept
{
    TVector<TIntrusiveConstPtr<TMemTable>> vec(Frozen.begin(), Frozen.end());

    if (Mutable)
        vec.emplace_back(Mutable);

    return vec;
}

TEpoch TTable::Snapshot() noexcept
{
    if (Mutable) {
        Annexed = Mutable->GetBlobs()->Tail();

        Frozen.insert(Mutable);
        Stat_.FrozenWaste += Mutable->GetWastedMem();
        Stat_.FrozenSize += Mutable->GetUsedMem();
        Stat_.FrozenOps += Mutable->GetOpsCount();
        Stat_.FrozenRows += Mutable->GetRowCount();

        Mutable = nullptr; /* have to make new TMemTable on next update */

        if (++Epoch == TEpoch::Max()) {
            Y_FAIL("Table epoch counter has reached infinity value");
        }
    }

    return Epoch;
}

void TTable::AddSafe(TPartView partView)
{
    if (partView) {
        Y_VERIFY(partView->Epoch < Epoch, "Cannot add part above head epoch");

        Stat_.Parts.Add(partView);
        Stat_.PartsPerTablet[partView->Label.TabletID()].Add(partView);

        if (partView->TxIdStats) {
            for (const auto& item : partView->TxIdStats->GetItems()) {
                const ui64 txId = item.GetTxId();
                OpenTransactions[txId].Parts.insert(partView.Part);
            }
        }

        using TVal = decltype(Flatten)::value_type;

        if (FlattenEpoch <= partView->Epoch) {
            FlattenEpoch = partView->Epoch;
            if (Levels) {
                // Slices from this part may be added on top
                Levels->Add(partView.Part, partView.Slices);
            }
        } else {
            Levels.Reset();
        }

        bool done = Flatten.insert(TVal(partView->Label, std::move(partView))).second;
        Y_VERIFY(done);
    }
}

TTable::TReady TTable::Precharge(TRawVals minKey_, TRawVals maxKey_, TTagsRef tags,
                        IPages* env, ui64 flg, 
                        ui64 items, ui64 bytes,
                        EDirection direction,
                        TRowVersion snapshot) const
{
    TReady res;
    bool ready = true;
    bool includeHistory = !snapshot.IsMax();

    if (items == Max<ui64>()) {
        items = 0; // disable limits
    }

    if (bytes == Max<ui64>()) {
        bytes = 0; // disable limits
    }

    if (minKey_.size() && minKey_.data() == maxKey_.data()) {
        const TCelled key(minKey_, *Scheme->Keys, false);
        const NBloom::TPrefix prefix(key);

        for (const auto& run : GetLevels()) {
            auto pos = run.Find(key);
            if (pos != run.end()) {
                const auto* part = pos->Part.Get();
                if ((flg & EHint::NoByKey) ||
                    part->MightHaveKey(prefix.Get(part->Scheme->Groups[0].KeyTypes.size())))
                {
                    TRowId row1 = pos->Slice.BeginRowId();
                    TRowId row2 = pos->Slice.EndRowId() - 1;
                    ready &= TCharge(env, *pos->Part, tags, includeHistory)
                        .Do(key, key, row1, row2, *Scheme->Keys, items, bytes)
                        .Ready;
                    ++res.Sieved;
                } else {
                    ++res.Weeded;
                }
            } 
        } 
    } else { 
        const TCelled minKey(minKey_, *Scheme->Keys, false);
        const TCelled maxKey(maxKey_, *Scheme->Keys, false);
 
        for (const auto& run : GetLevels()) {
            switch (direction) {
                case EDirection::Forward:
                    ready &= TCharge::Range(env, minKey, maxKey, run, *Scheme->Keys, tags, items, bytes, includeHistory);
                    break;
                case EDirection::Reverse:
                    ready &= TCharge::RangeReverse(env, maxKey, minKey, run, *Scheme->Keys, tags, items, bytes, includeHistory);
                    break;
            }
        }
    }

    res.Ready = ready ? EReady::Data : EReady::Page;
    return res;
}

void TTable::Update(ERowOp rop, TRawVals key, TOpsRef ops, TArrayRef<TMemGlob> apart, TRowVersion rowVersion)
{
    Y_VERIFY(!(ops && TCellOp::HaveNoOps(rop)), "Given ERowOp can't have ops");

    if (ErasedKeysCache && rop != ERowOp::Erase) {
        const TCelled cells(key, *Scheme->Keys, true);
        auto res = ErasedKeysCache->FindKey(cells);
        if (res.second) {
            ErasedKeysCache->Invalidate(res.first);
        }
    }

    MemTable().Update(rop, key, ops, apart, rowVersion, CommittedTransactions);
}

void TTable::UpdateTx(ERowOp rop, TRawVals key, TOpsRef ops, TArrayRef<TMemGlob> apart, ui64 txId)
{
    // Use a special row version that marks this update as uncommitted
    TRowVersion rowVersion(Max<ui64>(), txId);
    MemTable().Update(rop, key, ops, apart, rowVersion, CommittedTransactions);
    OpenTransactions[txId].Mem.insert(Mutable);
}

void TTable::CommitTx(ui64 txId, TRowVersion rowVersion)
{
    // TODO: track suspicious transactions (not open at commit time)
    MemTable().CommitTx(txId, rowVersion);

    // Note: it is possible to have multiple CommitTx for the same TxId but at
    // different row versions. The commit with the minimum row version wins.
    if (const auto* prev = CommittedTransactions.Find(txId); Y_LIKELY(!prev) || *prev > rowVersion) {
        CommittedTransactions.Add(txId, rowVersion);
        if (!prev) {
            RemovedTransactions.Remove(txId);
        }
    }

    // We don't know which keys have been commited, invalidate everything
    ErasedKeysCache.Reset();
}

void TTable::RemoveTx(ui64 txId)
{
    // TODO: track suspicious transactions (not open at remove time)
    MemTable().RemoveTx(txId);

    // Note: it is possible to have both CommitTx and RemoveTx for the same TxId
    // due to complicated split/merge shard interactions. The commit actually
    // wins over removes in all cases.
    if (const auto* prev = CommittedTransactions.Find(txId); Y_LIKELY(!prev)) {
        RemovedTransactions.Add(txId);
    }
}

TMemTable& TTable::MemTable()
{
    return
        *(Mutable ? Mutable : (Mutable = new TMemTable(Scheme, Epoch, Annexed)));
}

TAutoPtr<TTableIt> TTable::Iterate(TRawVals key_, TTagsRef tags, IPages* env, ESeek seek, TRowVersion snapshot) const noexcept
{
    Y_VERIFY(ColdParts.empty(), "Cannot iterate with cold parts");

    const TCelled key(key_, *Scheme->Keys, false);
    const ui64 limit = seek == ESeek::Exact ? 1 : Max<ui64>();

    TAutoPtr<TTableIt> dbIter(new TTableIt(Scheme.Get(), tags, limit, snapshot, CommittedTransactions));

    if (Mutable) {
        dbIter->Push(TMemIt::Make(*Mutable, Mutable->Immediate(), key, seek, Scheme->Keys, &dbIter->Remap, env, EDirection::Forward));
    }

    for (auto& fti : Frozen) {
        const TMemTable* memTable = fti.Get();

        dbIter->Push(TMemIt::Make(*memTable, memTable->Immediate(), key, seek, Scheme->Keys, &dbIter->Remap, env, EDirection::Forward));
    }

    if (Flatten) {
        for (const auto& run : GetLevels()) {
            auto iter = MakeHolder<TRunIt>(run, dbIter->Remap.Tags, Scheme->Keys, env);

            if (iter->Seek(key, seek) != EReady::Gone)
                dbIter->Push(std::move(iter));
        }
    }

    if (EraseCacheEnabled) {
        if (!ErasedKeysCache) {
            ErasedKeysCache = new TKeyRangeCache(*Scheme->Keys, EraseCacheConfig);
        }
        dbIter->ErasedKeysCache = ErasedKeysCache;
    }

    return dbIter;
}

TAutoPtr<TTableReverseIt> TTable::IterateReverse(TRawVals key_, TTagsRef tags, IPages* env, ESeek seek, TRowVersion snapshot) const noexcept
{
    Y_VERIFY(ColdParts.empty(), "Cannot iterate with cold parts");

    const TCelled key(key_, *Scheme->Keys, false);
    const ui64 limit = seek == ESeek::Exact ? 1 : Max<ui64>();

    TAutoPtr<TTableReverseIt> dbIter(new TTableReverseIt(Scheme.Get(), tags, limit, snapshot, CommittedTransactions));

    if (Mutable) {
        dbIter->Push(TMemIt::Make(*Mutable, Mutable->Immediate(), key, seek, Scheme->Keys, &dbIter->Remap, env, EDirection::Reverse));
    }

    for (auto& fti : Frozen) {
        const TMemTable* memTable = fti.Get();

        dbIter->Push(TMemIt::Make(*memTable, memTable->Immediate(), key, seek, Scheme->Keys, &dbIter->Remap, env, EDirection::Reverse));
    }

    if (Flatten) {
        for (const auto& run : GetLevels()) {
            auto iter = MakeHolder<TRunIt>(run, dbIter->Remap.Tags, Scheme->Keys, env);

            if (iter->SeekReverse(key, seek) != EReady::Gone)
                dbIter->Push(std::move(iter));
        }
    }

    if (EraseCacheEnabled) {
        if (!ErasedKeysCache) {
            ErasedKeysCache = new TKeyRangeCache(*Scheme->Keys, EraseCacheConfig);
        }
        dbIter->ErasedKeysCache = ErasedKeysCache;
    }

    return dbIter;
}

TTable::TReady TTable::Select(TRawVals key_, TTagsRef tags, IPages* env, TRowState& row, 
                             ui64 flg, TRowVersion snapshot,
                             TDeque<TPartSimpleIt>& tempIterators) const noexcept
{
    Y_VERIFY(ColdParts.empty(), "Cannot select with cold parts");
    Y_VERIFY(key_.size() == Scheme->Keys->Types.size());

    const TCelled key(key_, *Scheme->Keys, false);

    const TRemap remap(*Scheme, tags);

    row.Reset(remap.Nulls());

    for (auto &pin: remap.KeyPins())
        row.Set(pin.Pos, { ECellOp::Set, ELargeObj::Inline }, key[pin.Key]);

    TReady result;

    const NBloom::TPrefix prefix(key);

    TEpoch lastEpoch = TEpoch::Max();

    bool snapshotFound = (snapshot == TRowVersion::Max());

    // Mutable has the newest data
    if (Mutable) {
        lastEpoch = Mutable->Epoch;
        if (auto it = TMemIt::Make(*Mutable, Mutable->Immediate(), key, ESeek::Exact, Scheme->Keys, &remap, env, EDirection::Forward)) {
            if (it->IsValid() && (snapshotFound || it->SkipToRowVersion(snapshot, CommittedTransactions))) {
                // N.B. stop looking for snapshot after the first hit
                snapshotFound = true;
                it->Apply(row, CommittedTransactions);
            }
            result.Invisible += it->InvisibleRowSkips;
        } 
    }

    // Frozen are sorted by epoch, apply in reverse order
    for (auto pos = Frozen.rbegin(); !row.IsFinalized() && pos != Frozen.rend(); ++pos) {
        const auto& memTable = *pos;
        Y_VERIFY(lastEpoch > memTable->Epoch, "Ordering of epochs is incorrect");
        lastEpoch = memTable->Epoch;
        if (auto it = TMemIt::Make(*memTable, memTable->Immediate(), key, ESeek::Exact, Scheme->Keys, &remap, env, EDirection::Forward)) {
            if (it->IsValid() && (snapshotFound || it->SkipToRowVersion(snapshot, CommittedTransactions))) {
                // N.B. stop looking for snapshot after the first hit
                snapshotFound = true;
                it->Apply(row, CommittedTransactions);
            }
            result.Invisible += it->InvisibleRowSkips;
        }
    }

    bool ready = true;
    if (!row.IsFinalized() && Flatten) {
        // Levels are ordered from newest to oldest, apply in order
        for (const auto& run : GetLevels()) {
            auto pos = run.Find(key);
            if (pos != run.end()) {
                const auto* part = pos->Part.Get();
                if ((flg & EHint::NoByKey) ||
                    part->MightHaveKey(prefix.Get(part->Scheme->Groups[0].KeyTypes.size())))
                {
                    ++result.Sieved;
                    TPartSimpleIt& it = tempIterators.emplace_back(part, tags, Scheme->Keys, env);
                    it.SetBounds(pos->Slice);
                    auto res = it.Seek(key, ESeek::Exact);
                    if (res == EReady::Data) {
                        Y_VERIFY(lastEpoch > part->Epoch, "Ordering of epochs is incorrect");
                        lastEpoch = part->Epoch;
                        if (!snapshotFound) {
                            res = it.SkipToRowVersion(snapshot, CommittedTransactions);
                            result.Invisible += std::exchange(it.InvisibleRowSkips, 0);
                            if (res == EReady::Data) {
                                // N.B. stop looking for snapshot after the first hit
                                snapshotFound = true;
                            }
                        }
                    }
                    if (ready = ready && bool(res)) {
                        if (res == EReady::Data) {
                            it.Apply(row, CommittedTransactions);
                            if (row.IsFinalized()) {
                                break;
                            }
                        } else {
                            ++result.NoKey;
                        }
                    }
                } else {
                    ++result.Weeded;
                }
            }
        }
    }

    Y_VERIFY_DEBUG(result.Invisible == 0 || !snapshot.IsMax());

    if (!ready || row.Need()) {
        result.Ready = EReady::Page;
    } else if (row == ERowOp::Erase || row == ERowOp::Absent) {
        result.Ready = EReady::Gone;
    } else {
        result.Ready = EReady::Data;
    }
    return result;
}

void TTable::DebugDump(IOutputStream& str, IPages* env, const NScheme::TTypeRegistry& reg) const
{
    str << "Mutable: " << (Mutable ? Mutable->Epoch : TEpoch::Zero()) << Endl;
    str << "Frozen: [";
    for (const auto& it : Frozen) {
        str << it->Epoch;
    } 
    str << "]" << Endl 
        << "Parts: ["
        << Endl; 
    for (const auto& fpIt: Flatten) {
        str << "    "; 
        NFmt::Ln(*fpIt.second);
    } 
    str << "]" << Endl; 
    if (ColdParts) {
        str << "ColdParts: [" << Endl;
        for (const auto& it : ColdParts) {
            str << "    ";
            NFmt::Ln(*it.second);
        }
        str << "]" << Endl;
    }
    str << "Mutable dump: " << Endl;

    if (Mutable)
        Mutable->DebugDump(str, reg);
    for (const auto& it : Frozen) {
        str << "Frozen " << it->Epoch << " dump: " << Endl;
        it->DebugDump(str, reg);
    } 
 
    TDump dump(str, env, &reg);

    for (const auto &it: Flatten) dump.Part(*it.second);
} 
 
TKeyRangeCache* TTable::GetErasedKeysCache() const
{
    return ErasedKeysCache.Get();
}

bool TTable::RemoveRowVersions(const TRowVersion& lower, const TRowVersion& upper)
{
    return RemovedRowVersions.Add(lower, upper);
}

TCompactionStats TTable::GetCompactionStats() const
{
    TCompactionStats stats;
    stats.MemRowCount = GetMemRowCount();
    stats.MemDataSize = GetMemSize();
    stats.MemDataWaste = GetMemWaste();

    for (auto &it: ColdParts)
        stats.PartOwners.insert(it.second->Label.TabletID());

    for (auto &it: Flatten)
        stats.PartOwners.insert(it.second->Label.TabletID());

    for (auto &it: TxStatus)
        stats.PartOwners.insert(it.second->Label.TabletID());

    stats.PartCount = Flatten.size() + ColdParts.size();

    return stats;
}

void TPartStats::Add(const TPartView& partView)
{
    PartsCount += 1;
    IndexBytes += partView->IndexesRawSize;
    ByKeyBytes += partView->ByKey ? partView->ByKey->Raw.size() : 0;
    PlainBytes += partView->Stat.Bytes;
    CodedBytes += partView->Stat.Coded;
    RowsErase += partView->Stat.Drops;
    RowsTotal += partView->Stat.Rows;
    SmallBytes += partView->Small ? partView->Small->Stats().Size : 0;
    SmallItems += partView->Small ? partView->Small->Stats().Items : 0;
    LargeBytes += partView->Large ? partView->Large->Stats().Size : 0;
    LargeItems += partView->Blobs ? partView->Blobs->Total() : 0;

    OtherBytes += (partView->Small ? partView->Small->Raw.size() : 0);
    OtherBytes += (partView->Large ? partView->Large->Raw.size() : 0);
    OtherBytes += (partView->Blobs ? partView->Blobs->Raw.size() : 0);
}

bool TPartStats::Remove(const TPartView& partView)
{
    NUtil::SubSafe(PartsCount, ui64(1));
    NUtil::SubSafe(IndexBytes, partView->IndexesRawSize);
    NUtil::SubSafe(ByKeyBytes, partView->ByKey ? partView->ByKey->Raw.size() : 0);
    NUtil::SubSafe(PlainBytes, partView->Stat.Bytes);
    NUtil::SubSafe(CodedBytes, partView->Stat.Coded);
    NUtil::SubSafe(RowsErase, partView->Stat.Drops);
    NUtil::SubSafe(RowsTotal, partView->Stat.Rows);

    if (auto *small = partView->Small.Get()) {
        NUtil::SubSafe(SmallBytes, small->Stats().Size);
        NUtil::SubSafe(SmallItems, ui64(small->Stats().Items));
        NUtil::SubSafe(OtherBytes, ui64(small->Raw.size()));
    }

    if (auto *large = partView->Large.Get()) {
        NUtil::SubSafe(LargeBytes, large->Stats().Size);
        NUtil::SubSafe(OtherBytes, large->Raw.size());
    }

    if (auto *blobs = partView->Blobs.Get()) {
        NUtil::SubSafe(LargeItems, ui64(blobs->Total()));
        NUtil::SubSafe(OtherBytes, blobs->Raw.size());
    }

    return PartsCount > 0;
}

TPartStats& TPartStats::operator+=(const TPartStats& rhs)
{
    PartsCount += rhs.PartsCount;
    IndexBytes += rhs.IndexBytes;
    OtherBytes += rhs.OtherBytes;
    ByKeyBytes += rhs.ByKeyBytes;
    PlainBytes += rhs.PlainBytes;
    CodedBytes += rhs.CodedBytes;
    SmallBytes += rhs.SmallBytes;
    SmallItems += rhs.SmallItems;
    LargeBytes += rhs.LargeBytes;
    LargeItems += rhs.LargeItems;
    RowsErase += rhs.RowsErase;
    RowsTotal += rhs.RowsTotal;
    return *this;
}

TPartStats& TPartStats::operator-=(const TPartStats& rhs)
{
    NUtil::SubSafe(PartsCount, rhs.PartsCount);
    NUtil::SubSafe(IndexBytes, rhs.IndexBytes);
    NUtil::SubSafe(OtherBytes, rhs.OtherBytes);
    NUtil::SubSafe(ByKeyBytes, rhs.ByKeyBytes);
    NUtil::SubSafe(PlainBytes, rhs.PlainBytes);
    NUtil::SubSafe(CodedBytes, rhs.CodedBytes);
    NUtil::SubSafe(SmallBytes, rhs.SmallBytes);
    NUtil::SubSafe(SmallItems, rhs.SmallItems);
    NUtil::SubSafe(LargeBytes, rhs.LargeBytes);
    NUtil::SubSafe(LargeItems, rhs.LargeItems);
    NUtil::SubSafe(RowsErase, rhs.RowsErase);
    NUtil::SubSafe(RowsTotal, rhs.RowsTotal);
    return *this;
}

}}
