OWNER(g:yatest)

PY23_LIBRARY()

PY_SRCS(
    ya.py
    collection.py
    conftests.py
    fixtures.py
)

PEERDIR(
    library/python/filelock
    library/python/find_root
    library/python/testing/filter 
)

IF (PYTHON2)
    PY_SRCS(
        fakeid_py2.py
    )

    PEERDIR(
        contrib/python/faulthandler
    )
ELSE()
    PY_SRCS(
        fakeid_py3.py
    )
ENDIF()

END()
