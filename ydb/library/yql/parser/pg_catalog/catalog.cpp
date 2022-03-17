#include "catalog.h"
#include <util/generic/utility.h>
#include <util/generic/hash.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <library/cpp/resource/resource.h>

namespace NYql::NPg {

using TOperators = THashMap<ui32, TOperDesc>;

using TProcs = THashMap<ui32, TProcDesc>;

using TTypes = THashMap<ui32, TTypeDesc>;

using TCasts = THashMap<ui32, TCastDesc>;

using TAggregations = THashMap<ui32, TAggregateDesc>;

class TParser {
public:
    void Do(const TString& dat) {
        enum class EState {
            WaitBracket,
            InsideBrackets,
            WaitForEndOfKey,
            WaitForValue,
            WaitForEndOfValue
        };

        EState state = EState::WaitBracket;
        bool AfterBackSlash = false;
        TStringBuilder key;
        TStringBuilder value;
        for (char c : dat) {
            switch (state) {
                case EState::WaitBracket: {
                    if (c == '{') {
                        state = EState::InsideBrackets;
                    }

                    break;
                }
                case EState::InsideBrackets: {
                    if (c == '}') {
                        state = EState::WaitBracket;
                        OnFinish();
                        continue;
                    }

                    if (c == ' ' || c == ',' || c == '\n') {
                        continue;
                    }

                    key.clear();
                    key << c;
                    state = EState::WaitForEndOfKey;
                    break;
                }
                case EState::WaitForEndOfKey: {
                    if (c != ' ') {
                        key << c;
                        continue;
                    }

                    state = EState::WaitForValue;
                    break;
                }
                case EState::WaitForValue: {
                    if (c != '\'') {
                        continue;
                    }

                    state = EState::WaitForEndOfValue;
                    value.clear();
                    break;
                }
                case EState::WaitForEndOfValue: {
                    if (c == '\\' && !AfterBackSlash) {
                        AfterBackSlash = true;
                        continue;
                    }

                    if (AfterBackSlash) {
                        AfterBackSlash = false;
                        value << c;
                        continue;
                    }

                    if (c != '\'') {
                        value << c;
                        continue;
                    }

                    state = EState::InsideBrackets;
                    OnKey(key, value);
                    break;
                }
            }
        }
    }

    virtual void OnFinish() = 0;
    virtual void OnKey(const TString& key, const TString& value) = 0;
};

class TOperatorsParser : public TParser {
public:
    TOperatorsParser(TOperators& operators, const THashMap<TString, ui32>& typeByName,
        const THashMap<TString, TVector<ui32>>& procByName)
        : Operators(operators)
        , TypeByName(typeByName)
        , ProcByName(procByName)
    {}

    void OnKey(const TString& key, const TString& value) override {
        if (key == "oid") {
            LastOperator.OperId = FromString<ui32>(value);
        } else if (key == "oprname") {
            LastOperator.Name = value;
        } else if (key == "oprkind") {
            if (value == "r") {
                LastOperator.Kind = EOperKind::RightUnary;
            } else if (value == "l") {
                LastOperator.Kind = EOperKind::LeftUnary;
            }
        } else if (key == "oprleft") {
            if (value != "0") {
                auto typeIdPtr = TypeByName.FindPtr(value);
                Y_ENSURE(typeIdPtr);
                LastOperator.LeftType = *typeIdPtr;
            }
        } else if (key == "oprright") {
            if (value != "0") {
                auto typeIdPtr = TypeByName.FindPtr(value);
                Y_ENSURE(typeIdPtr);
                LastOperator.RightType = *typeIdPtr;
            }
        } else if (key == "oprresult") {
            auto typeIdPtr = TypeByName.FindPtr(value);
            Y_ENSURE(typeIdPtr);
            LastOperator.ResultType = *typeIdPtr;
        } else if (key == "oprcode") {
            auto procIdPtr = ProcByName.FindPtr(value);
            // skip operator if proc isn't buildin, e.g. path_contain_pt
            if (!procIdPtr) {
                IsSupported = false;
                return;
            }

            Y_ENSURE(procIdPtr->size() == 1);
            LastOperator.ProcId = procIdPtr->at(0);
        }
    }

    void OnFinish() override {
        if (IsSupported) {
            Y_ENSURE(!LastOperator.Name.empty());
            Operators[LastOperator.OperId] = LastOperator;
        }

        LastOperator = TOperDesc();
        IsSupported = true;
    }

private:
    TOperators& Operators;
    const THashMap<TString, ui32>& TypeByName;
    const THashMap<TString, TVector<ui32>>& ProcByName;
    TOperDesc LastOperator;
    bool IsSupported = true;
};

class TProcsParser : public TParser {
public:
    TProcsParser(TProcs& procs, const THashMap<TString, ui32>& typeByName)
        : Procs(procs)
        , TypeByName(typeByName)
    {}

    void OnKey(const TString& key, const TString& value) override {
        if (key == "oid") {
            LastProc.ProcId = FromString<ui32>(value);
        } else if (key == "provariadic") {
            IsSupported = false;
        } else if (key == "prorettype") {
            auto idPtr = TypeByName.FindPtr(value);
            Y_ENSURE(idPtr);
            LastProc.ResultType = *idPtr;
        } else if (key == "proname") {
            LastProc.Name = value;
        } else if (key == "prosrc") {
            LastProc.Src = value;
        } else if (key == "prolang") {
            IsSupported = false;
        } else if (key == "proargtypes") {
            TVector<TString> strArgs;
            Split(value, " ", strArgs);
            LastProc.ArgTypes.reserve(strArgs.size());
            for (const auto& s : strArgs) {
                auto idPtr = TypeByName.FindPtr(s);
                Y_ENSURE(idPtr);
                LastProc.ArgTypes.push_back(*idPtr);
            }
        } else if (key == "proisstrict") {
            LastProc.IsStrict = (value == "t");
        } else if (key == "proretset") {
            IsSupported = false;
        }
    }

    void OnFinish() override {
        if (IsSupported) {
            Y_ENSURE(!LastProc.Name.empty());
            Procs[LastProc.ProcId] = LastProc;
        }

        IsSupported = true;
        LastProc = TProcDesc();
    }

private:
    TProcs& Procs;
    const THashMap<TString, ui32>& TypeByName;
    TProcDesc LastProc;
    bool IsSupported = true;
};

struct TLazyTypeInfo {
    TString ElementType;
    TString InFunc;
    TString OutFunc;
    TString SendFunc;
    TString ReceiveFunc;
};

class TTypesParser : public TParser {
public:
    TTypesParser(TTypes& types, THashMap<ui32, TLazyTypeInfo>& lazyInfos)
        : Types(types)
        , LazyInfos(lazyInfos)
    {}

    void OnKey(const TString& key, const TString& value) override {
        if (key == "oid") {
            LastType.TypeId = FromString<ui32>(value);
        } else if (key == "array_type_oid") {
            LastType.ArrayTypeId = FromString<ui32>(value);
        } else if (key == "typname") {
            LastType.Name = value;
        } else if (key == "typcategory") {
            Y_ENSURE(value.size() == 1);
            LastType.Category = value[0];
        } else if (key == "typlen") {
            if (value == "NAMEDATALEN") {
                LastType.TypeLen = 64;
            } else if (value == "SIZEOF_POINTER") {
                LastType.TypeLen = 8;
            } else {
                LastType.TypeLen = FromString<i32>(value);
            }
        } else if (key == "typelem") {
            LastLazyTypeInfo.ElementType = value; // resolve later
        } else if (key == "typinput") {
            LastLazyTypeInfo.InFunc = value; // resolve later
        } else if (key == "typoutput") {
            LastLazyTypeInfo.OutFunc = value; // resolve later
        } else if (key == "typsend") {
            LastLazyTypeInfo.SendFunc = value; // resolve later
        } else if (key == "typreceive") {
            LastLazyTypeInfo.ReceiveFunc = value; // resolve later
        } else if (key == "typbyval") {
            if (value == "f") {
                LastType.PassByValue = false;
            } else if (value == "t" || value == "FLOAT8PASSBYVAL") {
                LastType.PassByValue = true;
            } else {
                ythrow yexception() << "Unknown typbyval value: " << value;
            }
        }
    }

    void OnFinish() override {
        Y_ENSURE(!LastType.Name.empty());
        if (LastType.TypeLen < 0 || LastType.TypeLen > 8) {
            Y_ENSURE(!LastType.PassByValue);
        }

        Types[LastType.TypeId] = LastType;
        if (LastType.ArrayTypeId) {
            auto arrayType = LastType;
            arrayType.Name = "_" + arrayType.Name;
            arrayType.ElementTypeId = arrayType.TypeId;
            arrayType.TypeId = LastType.ArrayTypeId;
            Types[LastType.ArrayTypeId] = arrayType;
        }

        LazyInfos[LastType.TypeId] = LastLazyTypeInfo;

        LastType = TTypeDesc();
        LastLazyTypeInfo = TLazyTypeInfo();
    }

private:
    TTypes& Types;
    THashMap<ui32, TLazyTypeInfo>& LazyInfos;
    TTypeDesc LastType;
    TLazyTypeInfo LastLazyTypeInfo;
};

class TCastsParser : public TParser {
public:
    TCastsParser(TCasts& casts, const THashMap<TString, ui32>& typeByName,
        const THashMap<TString, TVector<ui32>>& procByName, const TProcs& procs)
        : Casts(casts)
        , TypeByName(typeByName)
        , ProcByName(procByName)
        , Procs(procs)
    {}

    void OnKey(const TString& key, const TString& value) override {
        if (key == "castsource") {
            auto typePtr = TypeByName.FindPtr(value);
            Y_ENSURE(typePtr);
            LastCast.SourceId = *typePtr;
        } else if (key == "casttarget") {
            auto typePtr = TypeByName.FindPtr(value);
            Y_ENSURE(typePtr);
            LastCast.TargetId = *typePtr;
        } else if (key == "castfunc") {
            if (value != "0") {
                if (value.Contains(',')) {
                    // e.g. castfunc => 'bit(int8,int4)'
                    IsSupported = false;
                } else if (value.Contains('(')) {
                    auto pos1 = value.find('(');
                    auto pos2 = value.find(')');
                    Y_ENSURE(pos1 != TString::npos);
                    Y_ENSURE(pos2 != TString::npos);
                    auto funcName = value.substr(0, pos1);
                    auto inputType = value.substr(pos1 + 1, pos2 - pos1 - 1);
                    auto inputTypeIdPtr = TypeByName.FindPtr(inputType);
                    Y_ENSURE(inputTypeIdPtr);
                    auto procIdPtr = ProcByName.FindPtr(funcName);
                    Y_ENSURE(procIdPtr);
                    bool found = false;
                    for (const auto& procId : *procIdPtr) {
                        auto procPtr = Procs.FindPtr(procId);
                        Y_ENSURE(procPtr);
                        if (procPtr->ArgTypes.size() != 1) {
                            continue;
                        }

                        if (procPtr->ArgTypes.at(0) == *inputTypeIdPtr) {
                            LastCast.FunctionId = procPtr->ProcId;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        // e.g. convert circle to 12-vertex polygon, used sql proc
                        IsSupported = false;
                    }
                } else {
                    auto procIdPtr = ProcByName.FindPtr(value);
                    Y_ENSURE(procIdPtr);
                    Y_ENSURE(procIdPtr->size() == 1);
                    LastCast.FunctionId = procIdPtr->at(0);
                }
            }
        } else if (key == "castmethod") {
            if (value == "f") {
                LastCast.Method = ECastMethod::Function;
            } else if (value == "i") {
                LastCast.Method = ECastMethod::InOut;
            } else if (value == "b") {
                LastCast.Method = ECastMethod::Binary;
            } else {
                ythrow yexception() << "Unknown castmethod value: " << value;
            }
        }
    }

    void OnFinish() override {
        if (IsSupported) {
            auto id = 1 + Casts.size();
            Casts[id] = LastCast;
        }

        LastCast = TCastDesc();
        IsSupported = true;
    }

private:
    TCasts& Casts;
    const THashMap<TString, ui32>& TypeByName;
    const THashMap<TString, TVector<ui32>>& ProcByName;
    const TProcs& Procs;
    TCastDesc LastCast;
    bool IsSupported = true;
};

class TAggregationsParser : public TParser {
public:
    TAggregationsParser(TAggregations& aggregations, const THashMap<TString, ui32>& typeByName,
        const THashMap<TString, TVector<ui32>>& procByName, const TProcs& procs)
        : Aggregations(aggregations)
        , TypeByName(typeByName)
        , ProcByName(procByName)
        , Procs(procs)
    {}

    void OnKey(const TString& key, const TString& value) override {
        Y_UNUSED(ProcByName);
        if (key == "aggtranstype") {
            auto typeId = TypeByName.FindPtr(value);
            Y_ENSURE(typeId);
            LastAggregation.TransTypeId = *typeId;
        } else if (key == "aggfnoid") {
            LastOid = value;
        } else if (key == "aggtransfn") {
            LastTransFunc = value;
        } else if (key == "aggfinalfn") {
            LastFinalFunc = value;
        } else if (key == "aggcombinefn") {
            LastCombineFunc = value;
        } else if (key == "aggserialfn") {
            LastSerializeFunc = value;
        } else if (key == "aggdeserialfn") {
            LastDeserializeFunc = value;
        } else if (key == "aggkind") {
            if (value == "n") {
                LastAggregation.Kind = EAggKind::Normal;
            } else if (value == "o") {
                LastAggregation.Kind = EAggKind::OrderedSet;
            } else if (value == "h") {
                LastAggregation.Kind = EAggKind::Hypothetical;
            } else {
                ythrow yexception() << "Unknown aggkind value: " << value;
            }
        }
    }

    void OnFinish() override {
        if (IsSupported) {
            if (FillSupported()) {
                auto id = Aggregations.size() + 1;
                Aggregations[id] = LastAggregation;
            }
        }

        LastAggregation = TAggregateDesc();
        IsSupported = true;
        LastOid = "";
        LastTransFunc = "";
        LastFinalFunc = "";
        LastCombineFunc = "";
        LastSerializeFunc = "";
        LastDeserializeFunc = "";
    }

    bool FillSupported() {
        Y_ENSURE(LastAggregation.TransTypeId);
        Y_ENSURE(LastOid);
        Y_ENSURE(LastTransFunc);
        auto transFuncIdsPtr = ProcByName.FindPtr(LastTransFunc);
        if (!transFuncIdsPtr) {
            // e.g. variadic ordered_set_transition_multi
            return false;
        }

        for (const auto id : *transFuncIdsPtr) {
            auto procPtr = Procs.FindPtr(id);
            Y_ENSURE(procPtr);
            if (procPtr->ArgTypes.size() >= 1 && procPtr->ArgTypes[0] == LastAggregation.TransTypeId) {
                Y_ENSURE(!LastAggregation.TransFuncId);
                LastAggregation.TransFuncId = id;
            }
        }

        Y_ENSURE(LastAggregation.TransFuncId);

        // oid format: name(arg1,arg2...)
        auto pos1 = LastOid.find('(');
        if (pos1 != TString::npos) {
            LastAggregation.Name = LastOid.substr(0, pos1);
            auto pos = pos1 + 1;
            for (;;) {
                auto nextPos = Min(LastOid.find(',', pos), LastOid.find(')', pos));
                Y_ENSURE(nextPos != TString::npos);
                if (pos == nextPos) {
                    break;
                }

                auto arg = LastOid.substr(pos, nextPos - pos);
                auto argTypeId = TypeByName.FindPtr(arg);
                Y_ENSURE(argTypeId);
                LastAggregation.ArgTypes.push_back(*argTypeId);
                pos = nextPos;
                if (LastOid[pos] == ')') {
                    break;
                } else {
                    ++pos;
                }
            }
        } else {
            // no signature in oid, use transfunc
            LastAggregation.Name = LastOid;
            auto procPtr = Procs.FindPtr(LastAggregation.TransFuncId);
            Y_ENSURE(procPtr);
            LastAggregation.ArgTypes = procPtr->ArgTypes;
            Y_ENSURE(LastAggregation.ArgTypes.size() >= 1);
            Y_ENSURE(LastAggregation.ArgTypes[0] == LastAggregation.TransTypeId);
            LastAggregation.ArgTypes.erase(LastAggregation.ArgTypes.begin());
        }

        Y_ENSURE(!LastAggregation.Name.empty());
        if (!ResolveFunc(LastFinalFunc, LastAggregation.FinalFuncId, 1)) {
            return false;
        }

        if (!ResolveFunc(LastCombineFunc, LastAggregation.CombineFuncId, 2)) {
            return false;
        }

        if (!ResolveFunc(LastSerializeFunc, LastAggregation.SerializeFuncId, 1)) {
            return false;
        }

        if (!ResolveFunc(LastDeserializeFunc, LastAggregation.DeserializeFuncId, 0)) {
            return false;
        }

        return true;
    }

    bool ResolveFunc(const TString& name, ui32& funcId, ui32 stateArgsCount) {
        if (name) {
            auto funcIdsPtr = ProcByName.FindPtr(name);
            if (!funcIdsPtr) {
                return false;
            }

            if (!stateArgsCount) {
                Y_ENSURE(funcIdsPtr->size() == 1);
            }

            for (const auto id : *funcIdsPtr) {
                auto procPtr = Procs.FindPtr(id);
                Y_ENSURE(procPtr);
                bool found = true;
                if (stateArgsCount > 0 && procPtr->ArgTypes.size() == stateArgsCount) {
                    for (ui32 i = 0; i < stateArgsCount; ++i) {
                        if (procPtr->ArgTypes[i] != LastAggregation.TransTypeId) {
                            found = false;
                            break;
                        }
                    }
                }

                if (found) {
                    Y_ENSURE(!funcId);
                    funcId = id;
                }
            }

            Y_ENSURE(funcId);
        }

        return true;
    }

private:
    TAggregations& Aggregations;
    const THashMap<TString, ui32>& TypeByName;
    const THashMap<TString, TVector<ui32>>& ProcByName;
    const TProcs& Procs;
    TAggregateDesc LastAggregation;
    bool IsSupported = true;
    TString LastOid;
    TString LastTransFunc;
    TString LastFinalFunc;
    TString LastCombineFunc;
    TString LastSerializeFunc;
    TString LastDeserializeFunc;
};

TOperators ParseOperators(const TString& dat, const THashMap<TString, ui32>& typeByName,
    const THashMap<TString, TVector<ui32>>& procByName) {
    TOperators ret;
    TOperatorsParser parser(ret, typeByName, procByName);
    parser.Do(dat);
    return ret;
}

TAggregations ParseAggregations(const TString& dat, const THashMap<TString, ui32>& typeByName,
    const THashMap<TString, TVector<ui32>>& procByName, const TProcs& procs) {
    TAggregations ret;
    TAggregationsParser parser(ret, typeByName, procByName, procs);
    parser.Do(dat);
    return ret;
}

TProcs ParseProcs(const TString& dat, const THashMap<TString, ui32>& typeByName) {
    TProcs ret;
    TProcsParser parser(ret, typeByName);
    parser.Do(dat);
    return ret;
}

TTypes ParseTypes(const TString& dat, THashMap<ui32, TLazyTypeInfo>& lazyInfos) {
    TTypes ret;
    TTypesParser parser(ret, lazyInfos);
    parser.Do(dat);
    return ret;
}

TCasts ParseCasts(const TString& dat, const THashMap<TString, ui32>& typeByName,
    const THashMap<TString, TVector<ui32>>& procByName, const TProcs& procs) {
    TCasts ret;
    TCastsParser parser(ret, typeByName, procByName, procs);
    parser.Do(dat);
    return ret;
}

struct TCatalog {
    TCatalog() {
        TString typeData;
        Y_ENSURE(NResource::FindExact("pg_type.dat", &typeData));
        TString opData;
        Y_ENSURE(NResource::FindExact("pg_operator.dat", &opData));
        TString procData;
        Y_ENSURE(NResource::FindExact("pg_proc.dat", &procData));
        TString castData;
        Y_ENSURE(NResource::FindExact("pg_cast.dat", &castData));
        TString aggData;
        Y_ENSURE(NResource::FindExact("pg_aggregate.dat", &aggData));
        THashMap<ui32, TLazyTypeInfo> lazyTypeInfos;
        Types = ParseTypes(typeData, lazyTypeInfos);
        for (const auto& [k, v] : Types) {
            if (k == v.TypeId) {
                Y_ENSURE(TypeByName.insert(std::make_pair(v.Name, k)).second);
            }

            if (k == v.ArrayTypeId) {
                Y_ENSURE(TypeByName.insert(std::make_pair("_" + v.Name, k)).second);
            }
        }

        for (const auto& [k, v]: lazyTypeInfos) {
            if (!v.ElementType) {
                continue;
            }

            auto elemTypePtr = TypeByName.FindPtr(v.ElementType);
            Y_ENSURE(elemTypePtr);
            auto typePtr = Types.FindPtr(k);
            Y_ENSURE(typePtr);
            typePtr->ElementTypeId = *elemTypePtr;
        }

        Procs = ParseProcs(procData, TypeByName);

        for (const auto& [k, v]: Procs) {
            ProcByName[v.Name].push_back(k);
        }

        const ui32 cstringId = 2275;
        const ui32 byteaId = 17;
        const ui32 internalId = 2281;
        for (const auto&[k, v] : lazyTypeInfos) {
            auto typePtr = Types.FindPtr(k);
            Y_ENSURE(typePtr);

            auto inFuncIdPtr = ProcByName.FindPtr(v.InFunc);
            Y_ENSURE(inFuncIdPtr);
            Y_ENSURE(inFuncIdPtr->size() == 1);
            auto inFuncPtr = Procs.FindPtr(inFuncIdPtr->at(0));
            Y_ENSURE(inFuncPtr);
            Y_ENSURE(inFuncPtr->ArgTypes.size() >= 1); // may have mods
            Y_ENSURE(inFuncPtr->ArgTypes[0] == cstringId);
            typePtr->InFuncId = inFuncIdPtr->at(0);

            auto outFuncIdPtr = ProcByName.FindPtr(v.OutFunc);
            Y_ENSURE(outFuncIdPtr);
            Y_ENSURE(outFuncIdPtr->size() == 1);
            auto outFuncPtr = Procs.FindPtr(outFuncIdPtr->at(0));
            Y_ENSURE(outFuncPtr);
            Y_ENSURE(outFuncPtr->ArgTypes.size() == 1);
            Y_ENSURE(outFuncPtr->ResultType == cstringId);
            typePtr->OutFuncId = outFuncIdPtr->at(0);

            if (v.ReceiveFunc != "-") {
                auto receiveFuncIdPtr = ProcByName.FindPtr(v.ReceiveFunc);
                Y_ENSURE(receiveFuncIdPtr);
                Y_ENSURE(receiveFuncIdPtr->size() == 1);
                auto receiveFuncPtr = Procs.FindPtr(receiveFuncIdPtr->at(0));
                Y_ENSURE(receiveFuncPtr);
                Y_ENSURE(receiveFuncPtr->ArgTypes.size() >= 1);
                Y_ENSURE(receiveFuncPtr->ArgTypes[0] == internalId); // mutable StringInfo
                typePtr->ReceiveFuncId = receiveFuncIdPtr->at(0);
            }

            if (v.SendFunc != "-") {
                auto sendFuncIdPtr = ProcByName.FindPtr(v.SendFunc);
                Y_ENSURE(sendFuncIdPtr);
                Y_ENSURE(sendFuncIdPtr->size() == 1);
                auto sendFuncPtr = Procs.FindPtr(sendFuncIdPtr->at(0));
                Y_ENSURE(sendFuncPtr);
                Y_ENSURE(sendFuncPtr->ArgTypes.size() == 1);
                Y_ENSURE(sendFuncPtr->ResultType == byteaId);
                typePtr->SendFuncId = sendFuncIdPtr->at(0);
            }
        }

        Casts = ParseCasts(castData, TypeByName, ProcByName, Procs);
        for (const auto&[k, v] : Casts) {
            Y_ENSURE(CastsByDir.insert(std::make_pair(std::make_pair(v.SourceId, v.TargetId), k)).second);
        }

        Operators = ParseOperators(opData, TypeByName, ProcByName);
        for (const auto&[k, v] : Operators) {
            OperatorsByName[v.Name].push_back(k);
        }

        Aggregations = ParseAggregations(aggData, TypeByName, ProcByName, Procs);
        for (const auto&[k, v] : Aggregations) {
            AggregationsByName[v.Name].push_back(k);
        }
    }

    static const TCatalog& Instance() {
        return *Singleton<TCatalog>();
    }

    TOperators Operators;
    TProcs Procs;
    TTypes Types;
    TCasts Casts;
    TAggregations Aggregations;
    THashMap<TString, TVector<ui32>> ProcByName;
    THashMap<TString, ui32> TypeByName;
    THashMap<std::pair<ui32, ui32>, ui32> CastsByDir;
    THashMap<TString, TVector<ui32>> OperatorsByName;
    THashMap<TString, TVector<ui32>> AggregationsByName;
};

bool ValidateArgs(const TVector<ui32>& descArgTypeIds, const TVector<ui32>& argTypeIds) {
    if (argTypeIds.size() != descArgTypeIds.size()) {
        return false;
    }

    bool found = true;
    for (size_t i = 0; i < argTypeIds.size(); ++i) {
        if (argTypeIds[i] == 0) {
            continue; // NULL
        }

        if (argTypeIds[i] != descArgTypeIds[i]) {
            found = false;
            break;
        }
    }

    return found;
}

bool ValidateProcArgs(const TProcDesc& d, const TVector<ui32>& argTypeIds) {
    return ValidateArgs(d.ArgTypes, argTypeIds);
}

const TProcDesc& LookupProc(ui32 procId, const TVector<ui32>& argTypeIds) {
    const auto& catalog = TCatalog::Instance();
    auto procPtr = catalog.Procs.FindPtr(procId);
    if (!procPtr) {
        throw yexception() << "No such proc: " << procId;
    }

    if (!ValidateProcArgs(*procPtr, argTypeIds)) {
        throw yexception() << "Unable to find an overload for proc with oid " << procId << " with given argument types";
    }

    return *procPtr;
}

const TProcDesc& LookupProc(const TString& name, const TVector<ui32>& argTypeIds) {
    const auto& catalog = TCatalog::Instance();
    auto procIdPtr = catalog.ProcByName.FindPtr(name);
    if (!procIdPtr) {
        throw yexception() << "No such proc: " << name;
    }

    for (const auto& id : *procIdPtr) {
        const auto& d = catalog.Procs.FindPtr(id);
        Y_ENSURE(d);
        if (!ValidateProcArgs(*d, argTypeIds)) {
            continue;
        }

        return *d;
    }

    throw yexception() << "Unable to find an overload for proc " << name << " with given argument types";
}

const TProcDesc& LookupProc(ui32 procId) {
    const auto& catalog = TCatalog::Instance();
    auto procPtr = catalog.Procs.FindPtr(procId);
    if (!procPtr) {
        throw yexception() << "No such proc: " << procId;
    }

    return *procPtr;
}

bool HasType(const TStringBuf& name) {
    const auto& catalog = TCatalog::Instance();
    return catalog.TypeByName.contains(name);
}

const TTypeDesc& LookupType(const TString& name) {
    const auto& catalog = TCatalog::Instance();
    auto typeIdPtr = catalog.TypeByName.FindPtr(name);
    if (!typeIdPtr) {
        throw yexception() << "No such type: " << name;
    }

    auto typePtr = catalog.Types.FindPtr(*typeIdPtr);
    Y_ENSURE(typePtr);
    return *typePtr;
}

const TTypeDesc& LookupType(ui32 typeId) {
    const auto& catalog = TCatalog::Instance();
    auto typePtr = catalog.Types.FindPtr(typeId);
    if (!typePtr) {
        throw yexception() << "No such type: " << typeId;
    }

    return *typePtr;
}

bool HasCast(ui32 sourceId, ui32 targetId) {
    const auto& catalog = TCatalog::Instance();
    return catalog.CastsByDir.contains(std::make_pair(sourceId, targetId));
}

const TCastDesc& LookupCast(ui32 sourceId, ui32 targetId) {
    const auto& catalog = TCatalog::Instance();
    auto castByDirPtr = catalog.CastsByDir.FindPtr(std::make_pair(sourceId, targetId));
    if (!castByDirPtr) {
        throw yexception() << "No such cast";
    }

    auto castPtr = catalog.Casts.FindPtr(*castByDirPtr);
    Y_ENSURE(castPtr);
    return *castPtr;
}

bool ValidateOperArgs(const TOperDesc& d, const TVector<ui32>& argTypeIds) {
    ui32 size = d.Kind == EOperKind::Binary ? 2 : 1;
    if (argTypeIds.size() != size) {
        return false;
    }

    bool found = true;
    for (size_t i = 0; i < argTypeIds.size(); ++i) {
        if (argTypeIds[i] == 0) {
            continue; // NULL
        }

        ui32 expectedArgType;
        if (d.Kind == EOperKind::RightUnary || (d.Kind == EOperKind::Binary && i == 0)) {
            expectedArgType = d.LeftType;
        } else {
            expectedArgType = d.RightType;
        }

        if (argTypeIds[i] != expectedArgType) {
            found = false;
            break;
        }
    }

    return found;
}

const TOperDesc& LookupOper(const TString& name, const TVector<ui32>& argTypeIds) {
    const auto& catalog = TCatalog::Instance();
    auto operIdPtr = catalog.OperatorsByName.FindPtr(name);
    if (!operIdPtr) {
        throw yexception() << "No such operator: " << name;
    }

    for (const auto& id : *operIdPtr) {
        const auto& d = catalog.Operators.FindPtr(id);
        Y_ENSURE(d);
        if (!ValidateOperArgs(*d, argTypeIds)) {
            continue;
        }

        return *d;
    }

    throw yexception() << "Unable to find an overload for operator " << name << " with given argument types";
}

const TOperDesc& LookupOper(ui32 operId, const TVector<ui32>& argTypeIds) {
    const auto& catalog = TCatalog::Instance();
    auto operPtr = catalog.Operators.FindPtr(operId);
    if (!operPtr) {
        throw yexception() << "No such oper: " << operId;
    }

    if (!ValidateOperArgs(*operPtr, argTypeIds)) {
        throw yexception() << "Unable to find an overload for operator with oid " << operId << " with given argument types";
    }

    return *operPtr;
}

const TOperDesc& LookupOper(ui32 operId) {
    const auto& catalog = TCatalog::Instance();
    auto operPtr = catalog.Operators.FindPtr(operId);
    if (!operPtr) {
        throw yexception() << "No such oper: " << operId;
    }

    return *operPtr;
}

bool HasAggregation(const TStringBuf& name) {
    const auto& catalog = TCatalog::Instance();
    return catalog.AggregationsByName.contains(name);
}

bool ValidateAggregateArgs(const TAggregateDesc& d, const TVector<ui32>& argTypeIds) {
    return ValidateArgs(d.ArgTypes, argTypeIds);
}

const TAggregateDesc& LookupAggregation(const TStringBuf& name, const TVector<ui32>& argTypeIds) {
    const auto& catalog = TCatalog::Instance();
    auto aggIdPtr = catalog.AggregationsByName.FindPtr(name);
    if (!aggIdPtr) {
        throw yexception() << "No such aggregate: " << name;
    }

    for (const auto& id : *aggIdPtr) {
        const auto& d = catalog.Aggregations.FindPtr(id);
        Y_ENSURE(d);
        if (!ValidateAggregateArgs(*d, argTypeIds)) {
            continue;
        }

        return *d;
    }

    throw yexception() << "Unable to find an overload for aggregate " << name << " with given argument types";
}

}
