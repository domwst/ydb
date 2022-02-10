UNITTEST_FOR(ydb/core/client/minikql_result_lib)

OWNER(g:kikimr)

FORK_SUBTESTS()

TIMEOUT(300) 

SIZE(MEDIUM) 

SRCS(
    converter_ut.cpp
    objects_ut.cpp
)

PEERDIR(
    library/cpp/testing/unittest
    ydb/core/testlib
)

YQL_LAST_ABI_VERSION()

REQUIREMENTS(network:full ram:13)

END()
