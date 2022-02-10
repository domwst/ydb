PY23_LIBRARY()

OWNER(g:kikimr)

PY_SRCS(
    common/__init__.py
    common/yatest_common.py
    common/composite_assert.py
    common/delayed.py
    common/generators.py
    common/helpers.py
    common/local_db_scheme.py
    common/msgbus_types.py
    common/protobuf.py
    common/protobuf_console.py
    common/protobuf_kv.py
    common/protobuf_ss.py
    common/path_types.py
    common/types.py
    common/wait_for.py
    kv/__init__.py
    kv/helpers.py
    serializability/__init__.py
    serializability/checker.py
    harness/__init__.py
    harness/blockstore.py
    harness/daemon.py
    harness/kikimr_client.py
    harness/kikimr_node_interface.py
    harness/kikimr_monitoring.py
    harness/kikimr_cluster_interface.py
    harness/kikimr_cluster.py
    harness/kikimr_config.py
    harness/kikimr_http_client.py
    harness/kikimr_port_allocator.py
    harness/kikimr_runner.py
    harness/param_constants.py
    harness/util.py
    harness/tls_tools.py
    matchers/__init__.py
    matchers/collection.py
    matchers/datashard_matchers.py
    matchers/response.py
    matchers/response_matchers.py
    matchers/scheme_ops.py
    matchers/tablets.py
    predicates/__init__.py
    predicates/blobstorage.py
    predicates/tx.py
    predicates/hive.py
    predicates/executor.py
    wardens/__init__.py
    wardens/base.py
    wardens/datashard.py
    wardens/disk.py
    wardens/factories.py
    wardens/hive.py
    wardens/logs.py
    wardens/schemeshard.py
    nemesis/__init__.py
    nemesis/nemesis_core.py
    nemesis/nemesis_network.py
    nemesis/nemesis_process_killers.py
    nemesis/nemesis_time_terrorist.py
    nemesis/network/__init__.py
    nemesis/network/client.py
    nemesis/remote_execution.py
    nemesis/safety_warden.py
)

RESOURCE(
    harness/resources/default_profile.txt harness/resources/default_profile.txt
    harness/resources/default_yaml.yml harness/resources/default_yaml.yml
    harness/resources/default_domains.txt harness/resources/default_domains.txt
)

IF (NOT PYTHON3)
    PEERDIR(
        contrib/python/futures
        contrib/python/enum34
    )
ENDIF()

PEERDIR(
    contrib/python/PyHamcrest
    contrib/python/PyYAML
    contrib/python/cryptography
    contrib/python/protobuf
    contrib/python/pytest
    contrib/python/six
    ydb/public/sdk/python/ydb
    library/python/resource
    library/python/svn_version
    library/python/testing/yatest_common
    ydb/core/protos
    ydb/public/api/grpc
    ydb/public/api/grpc/draft
    ydb/public/api/protos
    ydb/tests/library/sqs
)

END()
