# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: calibration.proto

from google.protobuf.internal import enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import nanopb_pb2 as nanopb__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='calibration.proto',
  package='Emulation',
  syntax='proto2',
  serialized_options=None,
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x11\x63\x61libration.proto\x12\tEmulation\x1a\x0cnanopb.proto\"D\n\x12\x43\x61librationRequest\x12.\n\x07request\x18\x01 \x02(\x0e\x32\x1d.Emulation.CalibrationCommand\"s\n\x13\x43\x61librationResponse\x12.\n\x07request\x18\x01 \x02(\x0e\x32\x1d.Emulation.CalibrationCommand\x12,\n\x06result\x18\x02 \x02(\x0e\x32\x1c.Emulation.CalibrationResult*Z\n\x12\x43\x61librationCommand\x12\x0f\n\x0b\x43urrentZero\x10\x01\x12\x1a\n\x16Voltage_EmulatorOutput\x10\x02\x12\x17\n\x13Voltage_DebugOutput\x10\x03*-\n\x11\x43\x61librationResult\x12\x0b\n\x07Success\x10\x01\x12\x0b\n\x07\x46\x61ilure\x10\x02'
  ,
  dependencies=[nanopb__pb2.DESCRIPTOR,])

_CALIBRATIONCOMMAND = _descriptor.EnumDescriptor(
  name='CalibrationCommand',
  full_name='Emulation.CalibrationCommand',
  filename=None,
  file=DESCRIPTOR,
  create_key=_descriptor._internal_create_key,
  values=[
    _descriptor.EnumValueDescriptor(
      name='CurrentZero', index=0, number=1,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='Voltage_EmulatorOutput', index=1, number=2,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='Voltage_DebugOutput', index=2, number=3,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
  ],
  containing_type=None,
  serialized_options=None,
  serialized_start=233,
  serialized_end=323,
)
_sym_db.RegisterEnumDescriptor(_CALIBRATIONCOMMAND)

CalibrationCommand = enum_type_wrapper.EnumTypeWrapper(_CALIBRATIONCOMMAND)
_CALIBRATIONRESULT = _descriptor.EnumDescriptor(
  name='CalibrationResult',
  full_name='Emulation.CalibrationResult',
  filename=None,
  file=DESCRIPTOR,
  create_key=_descriptor._internal_create_key,
  values=[
    _descriptor.EnumValueDescriptor(
      name='Success', index=0, number=1,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='Failure', index=1, number=2,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
  ],
  containing_type=None,
  serialized_options=None,
  serialized_start=325,
  serialized_end=370,
)
_sym_db.RegisterEnumDescriptor(_CALIBRATIONRESULT)

CalibrationResult = enum_type_wrapper.EnumTypeWrapper(_CALIBRATIONRESULT)
CurrentZero = 1
Voltage_EmulatorOutput = 2
Voltage_DebugOutput = 3
Success = 1
Failure = 2



_CALIBRATIONREQUEST = _descriptor.Descriptor(
  name='CalibrationRequest',
  full_name='Emulation.CalibrationRequest',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='request', full_name='Emulation.CalibrationRequest.request', index=0,
      number=1, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
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
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=46,
  serialized_end=114,
)


_CALIBRATIONRESPONSE = _descriptor.Descriptor(
  name='CalibrationResponse',
  full_name='Emulation.CalibrationResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='request', full_name='Emulation.CalibrationResponse.request', index=0,
      number=1, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='result', full_name='Emulation.CalibrationResponse.result', index=1,
      number=2, type=14, cpp_type=8, label=2,
      has_default_value=False, default_value=1,
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
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=116,
  serialized_end=231,
)

_CALIBRATIONREQUEST.fields_by_name['request'].enum_type = _CALIBRATIONCOMMAND
_CALIBRATIONRESPONSE.fields_by_name['request'].enum_type = _CALIBRATIONCOMMAND
_CALIBRATIONRESPONSE.fields_by_name['result'].enum_type = _CALIBRATIONRESULT
DESCRIPTOR.message_types_by_name['CalibrationRequest'] = _CALIBRATIONREQUEST
DESCRIPTOR.message_types_by_name['CalibrationResponse'] = _CALIBRATIONRESPONSE
DESCRIPTOR.enum_types_by_name['CalibrationCommand'] = _CALIBRATIONCOMMAND
DESCRIPTOR.enum_types_by_name['CalibrationResult'] = _CALIBRATIONRESULT
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

CalibrationRequest = _reflection.GeneratedProtocolMessageType('CalibrationRequest', (_message.Message,), {
  'DESCRIPTOR' : _CALIBRATIONREQUEST,
  '__module__' : 'calibration_pb2'
  # @@protoc_insertion_point(class_scope:Emulation.CalibrationRequest)
  })
_sym_db.RegisterMessage(CalibrationRequest)

CalibrationResponse = _reflection.GeneratedProtocolMessageType('CalibrationResponse', (_message.Message,), {
  'DESCRIPTOR' : _CALIBRATIONRESPONSE,
  '__module__' : 'calibration_pb2'
  # @@protoc_insertion_point(class_scope:Emulation.CalibrationResponse)
  })
_sym_db.RegisterMessage(CalibrationResponse)


# @@protoc_insertion_point(module_scope)