# -*- coding: utf-8 -*- 
# Generated by the protocol buffer compiler.  DO NOT EDIT! 
# source: ydb/public/api/protos/ydb_experimental.proto
"""Generated protocol buffer code.""" 
from google.protobuf import descriptor as _descriptor 
from google.protobuf import message as _message 
from google.protobuf import reflection as _reflection 
from google.protobuf import symbol_database as _symbol_database 
# @@protoc_insertion_point(imports) 
 
_sym_db = _symbol_database.Default() 
 
 
from ydb.public.api.protos import ydb_issue_message_pb2 as ydb_dot_public_dot_api_dot_protos_dot_ydb__issue__message__pb2 
from ydb.public.api.protos import ydb_status_codes_pb2 as ydb_dot_public_dot_api_dot_protos_dot_ydb__status__codes__pb2 
from ydb.public.api.protos import ydb_value_pb2 as ydb_dot_public_dot_api_dot_protos_dot_ydb__value__pb2 
 
 
DESCRIPTOR = _descriptor.FileDescriptor( 
  name='ydb/public/api/protos/ydb_experimental.proto',
  package='Ydb.Experimental', 
  syntax='proto3', 
  serialized_options=b'\n\033com.yandex.ydb.experimentalB\022ExperimentalProtos\370\001\001', 
  create_key=_descriptor._internal_create_key, 
  serialized_pb=b'\n,ydb/public/api/protos/ydb_experimental.proto\x12\x10Ydb.Experimental\x1a-ydb/public/api/protos/ydb_issue_message.proto\x1a,ydb/public/api/protos/ydb_status_codes.proto\x1a%ydb/public/api/protos/ydb_value.proto\"\xee\x02\n\x19\x45xecuteStreamQueryRequest\x12\x10\n\x08yql_text\x18\x01 \x01(\t\x12O\n\nparameters\x18\x02 \x03(\x0b\x32;.Ydb.Experimental.ExecuteStreamQueryRequest.ParametersEntry\x12M\n\x0cprofile_mode\x18\x03 \x01(\x0e\x32\x37.Ydb.Experimental.ExecuteStreamQueryRequest.ProfileMode\x12\x0f\n\x07\x65xplain\x18\x04 \x01(\x08\x1a\x42\n\x0fParametersEntry\x12\x0b\n\x03key\x18\x01 \x01(\t\x12\x1e\n\x05value\x18\x02 \x01(\x0b\x32\x0f.Ydb.TypedValue:\x02\x38\x01\"J\n\x0bProfileMode\x12\x1c\n\x18PROFILE_MODE_UNSPECIFIED\x10\x00\x12\x08\n\x04NONE\x10\x01\x12\t\n\x05\x42\x41SIC\x10\x02\x12\x08\n\x04\x46ULL\x10\x03\"\xac\x01\n\x1a\x45xecuteStreamQueryResponse\x12)\n\x06status\x18\x01 \x01(\x0e\x32\x19.Ydb.StatusIds.StatusCode\x12\'\n\x06issues\x18\x02 \x03(\x0b\x32\x17.Ydb.Issue.IssueMessage\x12:\n\x06result\x18\x03 \x01(\x0b\x32*.Ydb.Experimental.ExecuteStreamQueryResult\"\x15\n\x13StreamQueryProgress\"\xae\x01\n\x18\x45xecuteStreamQueryResult\x12$\n\nresult_set\x18\x01 \x01(\x0b\x32\x0e.Ydb.ResultSetH\x00\x12\x11\n\x07profile\x18\x02 \x01(\tH\x00\x12\x39\n\x08progress\x18\x03 \x01(\x0b\x32%.Ydb.Experimental.StreamQueryProgressH\x00\x12\x14\n\nquery_plan\x18\x04 \x01(\tH\x00\x42\x08\n\x06resultB4\n\x1b\x63om.yandex.ydb.experimentalB\x12\x45xperimentalProtos\xf8\x01\x01\x62\x06proto3' 
  , 
  dependencies=[ydb_dot_public_dot_api_dot_protos_dot_ydb__issue__message__pb2.DESCRIPTOR,ydb_dot_public_dot_api_dot_protos_dot_ydb__status__codes__pb2.DESCRIPTOR,ydb_dot_public_dot_api_dot_protos_dot_ydb__value__pb2.DESCRIPTOR,]) 
 
 
 
_EXECUTESTREAMQUERYREQUEST_PROFILEMODE = _descriptor.EnumDescriptor( 
  name='ProfileMode', 
  full_name='Ydb.Experimental.ExecuteStreamQueryRequest.ProfileMode', 
  filename=None, 
  file=DESCRIPTOR, 
  create_key=_descriptor._internal_create_key, 
  values=[ 
    _descriptor.EnumValueDescriptor( 
      name='PROFILE_MODE_UNSPECIFIED', index=0, number=0, 
      serialized_options=None, 
      type=None, 
      create_key=_descriptor._internal_create_key), 
    _descriptor.EnumValueDescriptor( 
      name='NONE', index=1, number=1, 
      serialized_options=None, 
      type=None, 
      create_key=_descriptor._internal_create_key), 
    _descriptor.EnumValueDescriptor( 
      name='BASIC', index=2, number=2, 
      serialized_options=None, 
      type=None, 
      create_key=_descriptor._internal_create_key), 
    _descriptor.EnumValueDescriptor( 
      name='FULL', index=3, number=3, 
      serialized_options=None, 
      type=None, 
      create_key=_descriptor._internal_create_key), 
  ], 
  containing_type=None, 
  serialized_options=None, 
  serialized_start=491, 
  serialized_end=565, 
) 
_sym_db.RegisterEnumDescriptor(_EXECUTESTREAMQUERYREQUEST_PROFILEMODE) 
 
 
_EXECUTESTREAMQUERYREQUEST_PARAMETERSENTRY = _descriptor.Descriptor( 
  name='ParametersEntry', 
  full_name='Ydb.Experimental.ExecuteStreamQueryRequest.ParametersEntry', 
  filename=None, 
  file=DESCRIPTOR, 
  containing_type=None, 
  create_key=_descriptor._internal_create_key, 
  fields=[ 
    _descriptor.FieldDescriptor( 
      name='key', full_name='Ydb.Experimental.ExecuteStreamQueryRequest.ParametersEntry.key', index=0, 
      number=1, type=9, cpp_type=9, label=1, 
      has_default_value=False, default_value=b"".decode('utf-8'), 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
    _descriptor.FieldDescriptor( 
      name='value', full_name='Ydb.Experimental.ExecuteStreamQueryRequest.ParametersEntry.value', index=1, 
      number=2, type=11, cpp_type=10, label=1, 
      has_default_value=False, default_value=None, 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
  ], 
  extensions=[ 
  ], 
  nested_types=[], 
  enum_types=[ 
  ], 
  serialized_options=b'8\001', 
  is_extendable=False, 
  syntax='proto3', 
  extension_ranges=[], 
  oneofs=[ 
  ], 
  serialized_start=423, 
  serialized_end=489, 
) 
 
_EXECUTESTREAMQUERYREQUEST = _descriptor.Descriptor( 
  name='ExecuteStreamQueryRequest', 
  full_name='Ydb.Experimental.ExecuteStreamQueryRequest', 
  filename=None, 
  file=DESCRIPTOR, 
  containing_type=None, 
  create_key=_descriptor._internal_create_key, 
  fields=[ 
    _descriptor.FieldDescriptor( 
      name='yql_text', full_name='Ydb.Experimental.ExecuteStreamQueryRequest.yql_text', index=0, 
      number=1, type=9, cpp_type=9, label=1, 
      has_default_value=False, default_value=b"".decode('utf-8'), 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
    _descriptor.FieldDescriptor( 
      name='parameters', full_name='Ydb.Experimental.ExecuteStreamQueryRequest.parameters', index=1, 
      number=2, type=11, cpp_type=10, label=3, 
      has_default_value=False, default_value=[], 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
    _descriptor.FieldDescriptor( 
      name='profile_mode', full_name='Ydb.Experimental.ExecuteStreamQueryRequest.profile_mode', index=2, 
      number=3, type=14, cpp_type=8, label=1, 
      has_default_value=False, default_value=0, 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
    _descriptor.FieldDescriptor( 
      name='explain', full_name='Ydb.Experimental.ExecuteStreamQueryRequest.explain', index=3, 
      number=4, type=8, cpp_type=7, label=1, 
      has_default_value=False, default_value=False, 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
  ], 
  extensions=[ 
  ], 
  nested_types=[_EXECUTESTREAMQUERYREQUEST_PARAMETERSENTRY, ], 
  enum_types=[ 
    _EXECUTESTREAMQUERYREQUEST_PROFILEMODE, 
  ], 
  serialized_options=None, 
  is_extendable=False, 
  syntax='proto3', 
  extension_ranges=[], 
  oneofs=[ 
  ], 
  serialized_start=199, 
  serialized_end=565, 
) 
 
 
_EXECUTESTREAMQUERYRESPONSE = _descriptor.Descriptor( 
  name='ExecuteStreamQueryResponse', 
  full_name='Ydb.Experimental.ExecuteStreamQueryResponse', 
  filename=None, 
  file=DESCRIPTOR, 
  containing_type=None, 
  create_key=_descriptor._internal_create_key, 
  fields=[ 
    _descriptor.FieldDescriptor( 
      name='status', full_name='Ydb.Experimental.ExecuteStreamQueryResponse.status', index=0, 
      number=1, type=14, cpp_type=8, label=1, 
      has_default_value=False, default_value=0, 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
    _descriptor.FieldDescriptor( 
      name='issues', full_name='Ydb.Experimental.ExecuteStreamQueryResponse.issues', index=1, 
      number=2, type=11, cpp_type=10, label=3, 
      has_default_value=False, default_value=[], 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
    _descriptor.FieldDescriptor( 
      name='result', full_name='Ydb.Experimental.ExecuteStreamQueryResponse.result', index=2, 
      number=3, type=11, cpp_type=10, label=1, 
      has_default_value=False, default_value=None, 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
  ], 
  extensions=[ 
  ], 
  nested_types=[], 
  enum_types=[ 
  ], 
  serialized_options=None, 
  is_extendable=False, 
  syntax='proto3', 
  extension_ranges=[], 
  oneofs=[ 
  ], 
  serialized_start=568, 
  serialized_end=740, 
) 
 
 
_STREAMQUERYPROGRESS = _descriptor.Descriptor( 
  name='StreamQueryProgress', 
  full_name='Ydb.Experimental.StreamQueryProgress', 
  filename=None, 
  file=DESCRIPTOR, 
  containing_type=None, 
  create_key=_descriptor._internal_create_key, 
  fields=[ 
  ], 
  extensions=[ 
  ], 
  nested_types=[], 
  enum_types=[ 
  ], 
  serialized_options=None, 
  is_extendable=False, 
  syntax='proto3', 
  extension_ranges=[], 
  oneofs=[ 
  ], 
  serialized_start=742, 
  serialized_end=763, 
) 
 
 
_EXECUTESTREAMQUERYRESULT = _descriptor.Descriptor( 
  name='ExecuteStreamQueryResult', 
  full_name='Ydb.Experimental.ExecuteStreamQueryResult', 
  filename=None, 
  file=DESCRIPTOR, 
  containing_type=None, 
  create_key=_descriptor._internal_create_key, 
  fields=[ 
    _descriptor.FieldDescriptor( 
      name='result_set', full_name='Ydb.Experimental.ExecuteStreamQueryResult.result_set', index=0, 
      number=1, type=11, cpp_type=10, label=1, 
      has_default_value=False, default_value=None, 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
    _descriptor.FieldDescriptor( 
      name='profile', full_name='Ydb.Experimental.ExecuteStreamQueryResult.profile', index=1, 
      number=2, type=9, cpp_type=9, label=1, 
      has_default_value=False, default_value=b"".decode('utf-8'), 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
    _descriptor.FieldDescriptor( 
      name='progress', full_name='Ydb.Experimental.ExecuteStreamQueryResult.progress', index=2, 
      number=3, type=11, cpp_type=10, label=1, 
      has_default_value=False, default_value=None, 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
    _descriptor.FieldDescriptor( 
      name='query_plan', full_name='Ydb.Experimental.ExecuteStreamQueryResult.query_plan', index=3, 
      number=4, type=9, cpp_type=9, label=1, 
      has_default_value=False, default_value=b"".decode('utf-8'), 
      message_type=None, enum_type=None, containing_type=None, 
      is_extension=False, extension_scope=None, 
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key), 
  ], 
  extensions=[ 
  ], 
  nested_types=[], 
  enum_types=[ 
  ], 
  serialized_options=None, 
  is_extendable=False, 
  syntax='proto3', 
  extension_ranges=[], 
  oneofs=[ 
    _descriptor.OneofDescriptor( 
      name='result', full_name='Ydb.Experimental.ExecuteStreamQueryResult.result', 
      index=0, containing_type=None, 
      create_key=_descriptor._internal_create_key, 
    fields=[]), 
  ], 
  serialized_start=766, 
  serialized_end=940, 
) 
 
_EXECUTESTREAMQUERYREQUEST_PARAMETERSENTRY.fields_by_name['value'].message_type = ydb_dot_public_dot_api_dot_protos_dot_ydb__value__pb2._TYPEDVALUE 
_EXECUTESTREAMQUERYREQUEST_PARAMETERSENTRY.containing_type = _EXECUTESTREAMQUERYREQUEST 
_EXECUTESTREAMQUERYREQUEST.fields_by_name['parameters'].message_type = _EXECUTESTREAMQUERYREQUEST_PARAMETERSENTRY 
_EXECUTESTREAMQUERYREQUEST.fields_by_name['profile_mode'].enum_type = _EXECUTESTREAMQUERYREQUEST_PROFILEMODE 
_EXECUTESTREAMQUERYREQUEST_PROFILEMODE.containing_type = _EXECUTESTREAMQUERYREQUEST 
_EXECUTESTREAMQUERYRESPONSE.fields_by_name['status'].enum_type = ydb_dot_public_dot_api_dot_protos_dot_ydb__status__codes__pb2._STATUSIDS_STATUSCODE 
_EXECUTESTREAMQUERYRESPONSE.fields_by_name['issues'].message_type = ydb_dot_public_dot_api_dot_protos_dot_ydb__issue__message__pb2._ISSUEMESSAGE 
_EXECUTESTREAMQUERYRESPONSE.fields_by_name['result'].message_type = _EXECUTESTREAMQUERYRESULT 
_EXECUTESTREAMQUERYRESULT.fields_by_name['result_set'].message_type = ydb_dot_public_dot_api_dot_protos_dot_ydb__value__pb2._RESULTSET 
_EXECUTESTREAMQUERYRESULT.fields_by_name['progress'].message_type = _STREAMQUERYPROGRESS 
_EXECUTESTREAMQUERYRESULT.oneofs_by_name['result'].fields.append( 
  _EXECUTESTREAMQUERYRESULT.fields_by_name['result_set']) 
_EXECUTESTREAMQUERYRESULT.fields_by_name['result_set'].containing_oneof = _EXECUTESTREAMQUERYRESULT.oneofs_by_name['result'] 
_EXECUTESTREAMQUERYRESULT.oneofs_by_name['result'].fields.append( 
  _EXECUTESTREAMQUERYRESULT.fields_by_name['profile']) 
_EXECUTESTREAMQUERYRESULT.fields_by_name['profile'].containing_oneof = _EXECUTESTREAMQUERYRESULT.oneofs_by_name['result'] 
_EXECUTESTREAMQUERYRESULT.oneofs_by_name['result'].fields.append( 
  _EXECUTESTREAMQUERYRESULT.fields_by_name['progress']) 
_EXECUTESTREAMQUERYRESULT.fields_by_name['progress'].containing_oneof = _EXECUTESTREAMQUERYRESULT.oneofs_by_name['result'] 
_EXECUTESTREAMQUERYRESULT.oneofs_by_name['result'].fields.append( 
  _EXECUTESTREAMQUERYRESULT.fields_by_name['query_plan']) 
_EXECUTESTREAMQUERYRESULT.fields_by_name['query_plan'].containing_oneof = _EXECUTESTREAMQUERYRESULT.oneofs_by_name['result'] 
DESCRIPTOR.message_types_by_name['ExecuteStreamQueryRequest'] = _EXECUTESTREAMQUERYREQUEST 
DESCRIPTOR.message_types_by_name['ExecuteStreamQueryResponse'] = _EXECUTESTREAMQUERYRESPONSE 
DESCRIPTOR.message_types_by_name['StreamQueryProgress'] = _STREAMQUERYPROGRESS 
DESCRIPTOR.message_types_by_name['ExecuteStreamQueryResult'] = _EXECUTESTREAMQUERYRESULT 
_sym_db.RegisterFileDescriptor(DESCRIPTOR) 
 
ExecuteStreamQueryRequest = _reflection.GeneratedProtocolMessageType('ExecuteStreamQueryRequest', (_message.Message,), { 
 
  'ParametersEntry' : _reflection.GeneratedProtocolMessageType('ParametersEntry', (_message.Message,), { 
    'DESCRIPTOR' : _EXECUTESTREAMQUERYREQUEST_PARAMETERSENTRY, 
    '__module__' : 'ydb.public.api.protos.ydb_experimental_pb2'
    # @@protoc_insertion_point(class_scope:Ydb.Experimental.ExecuteStreamQueryRequest.ParametersEntry) 
    }) 
  , 
  'DESCRIPTOR' : _EXECUTESTREAMQUERYREQUEST, 
  '__module__' : 'ydb.public.api.protos.ydb_experimental_pb2'
  # @@protoc_insertion_point(class_scope:Ydb.Experimental.ExecuteStreamQueryRequest) 
  }) 
_sym_db.RegisterMessage(ExecuteStreamQueryRequest) 
_sym_db.RegisterMessage(ExecuteStreamQueryRequest.ParametersEntry) 
 
ExecuteStreamQueryResponse = _reflection.GeneratedProtocolMessageType('ExecuteStreamQueryResponse', (_message.Message,), { 
  'DESCRIPTOR' : _EXECUTESTREAMQUERYRESPONSE, 
  '__module__' : 'ydb.public.api.protos.ydb_experimental_pb2'
  # @@protoc_insertion_point(class_scope:Ydb.Experimental.ExecuteStreamQueryResponse) 
  }) 
_sym_db.RegisterMessage(ExecuteStreamQueryResponse) 
 
StreamQueryProgress = _reflection.GeneratedProtocolMessageType('StreamQueryProgress', (_message.Message,), { 
  'DESCRIPTOR' : _STREAMQUERYPROGRESS, 
  '__module__' : 'ydb.public.api.protos.ydb_experimental_pb2'
  # @@protoc_insertion_point(class_scope:Ydb.Experimental.StreamQueryProgress) 
  }) 
_sym_db.RegisterMessage(StreamQueryProgress) 
 
ExecuteStreamQueryResult = _reflection.GeneratedProtocolMessageType('ExecuteStreamQueryResult', (_message.Message,), { 
  'DESCRIPTOR' : _EXECUTESTREAMQUERYRESULT, 
  '__module__' : 'ydb.public.api.protos.ydb_experimental_pb2'
  # @@protoc_insertion_point(class_scope:Ydb.Experimental.ExecuteStreamQueryResult) 
  }) 
_sym_db.RegisterMessage(ExecuteStreamQueryResult) 
 
 
DESCRIPTOR._options = None 
_EXECUTESTREAMQUERYREQUEST_PARAMETERSENTRY._options = None 
# @@protoc_insertion_point(module_scope) 
