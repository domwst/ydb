PROTO_LIBRARY() 
 
GRPC() 
 
OWNER(g:kikimr) 
 
IF (OS_WINDOWS) 
    NO_OPTIMIZE_PY_PROTOS() 
ENDIF() 
 
SRCS( 
    minikql.proto 
) 
 
EXCLUDE_TAGS(GO_PROTO) 
 
END() 
