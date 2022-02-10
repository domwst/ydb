UNITTEST_FOR(ydb/core/blobstorage/vdisk/syncer)

OWNER(g:kikimr)

FORK_SUBTESTS()

TIMEOUT(600) 

SIZE(MEDIUM) 

PEERDIR(
    library/cpp/getopt
    library/cpp/svnversion
    ydb/core/base
    ydb/core/blobstorage
)

SRCS(
    blobstorage_syncer_data_ut.cpp
    blobstorage_syncer_localwriter_ut.cpp
    blobstorage_syncquorum_ut.cpp
)

END()
