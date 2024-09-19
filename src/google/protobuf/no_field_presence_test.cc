// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/cord.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_no_field_presence.pb.h"

namespace google {
namespace protobuf {
namespace {

using ::proto2_nofieldpresence_unittest::ExplicitForeignMessage;
using ::proto2_nofieldpresence_unittest::FOREIGN_BAZ;
using ::proto2_nofieldpresence_unittest::FOREIGN_FOO;
using ::proto2_nofieldpresence_unittest::ForeignMessage;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Not;
using ::testing::StrEq;
using ::testing::UnorderedPointwise;

// Custom gmock matchers to simplify testing for map entries.
//
// "HasKey" in this case means HasField() will return true in reflection.
MATCHER(MapEntryHasKey, "") {
  const Reflection* r = arg.GetReflection();
  const Descriptor* desc = arg.GetDescriptor();
  const FieldDescriptor* key = desc->map_key();

  return r->HasField(arg, key);
}

// "HasValue" in this case means HasField() will return true in reflection.
MATCHER(MapEntryHasValue, "") {
  const Reflection* r = arg.GetReflection();
  const Descriptor* desc = arg.GetDescriptor();
  const FieldDescriptor* key = desc->map_value();

  return r->HasField(arg, key);
}

MATCHER(MapEntryKeyExplicitPresence, "") {
  const Descriptor* desc = arg.GetDescriptor();
  const FieldDescriptor* key = desc->map_key();

  return key->has_presence();
}

MATCHER(MapEntryValueExplicitPresence, "") {
  const Descriptor* desc = arg.GetDescriptor();
  const FieldDescriptor* value = desc->map_value();

  return value->has_presence();
}

// Helper: checks that all fields have default (zero/empty) values.
void CheckDefaultValues(
    const proto2_nofieldpresence_unittest::TestAllTypes& m) {
  EXPECT_EQ(0, m.optional_int32());
  EXPECT_EQ(0, m.optional_int64());
  EXPECT_EQ(0, m.optional_uint32());
  EXPECT_EQ(0, m.optional_uint64());
  EXPECT_EQ(0, m.optional_sint32());
  EXPECT_EQ(0, m.optional_sint64());
  EXPECT_EQ(0, m.optional_fixed32());
  EXPECT_EQ(0, m.optional_fixed64());
  EXPECT_EQ(0, m.optional_sfixed32());
  EXPECT_EQ(0, m.optional_sfixed64());
  EXPECT_EQ(0, m.optional_float());
  EXPECT_EQ(0, m.optional_double());
  EXPECT_EQ(false, m.optional_bool());
  EXPECT_EQ(0, m.optional_string().size());
  EXPECT_EQ(0, m.optional_bytes().size());

  EXPECT_EQ(false, m.has_optional_nested_message());
  // accessor for message fields returns default instance when not present
  EXPECT_EQ(0, m.optional_nested_message().bb());
  EXPECT_EQ(false, m.has_optional_proto2_message());
  // Embedded proto2 messages still have proto2 semantics, e.g. non-zero default
  // values. Here the submessage is not present but its accessor returns the
  // default instance.
  EXPECT_EQ(41, m.optional_proto2_message().default_int32());
  EXPECT_EQ(false, m.has_optional_foreign_message());
  EXPECT_EQ(proto2_nofieldpresence_unittest::TestAllTypes::FOO,
            m.optional_nested_enum());
  EXPECT_EQ(proto2_nofieldpresence_unittest::FOREIGN_FOO,
            m.optional_foreign_enum());


  EXPECT_EQ(0, m.repeated_int32_size());
  EXPECT_EQ(0, m.repeated_int64_size());
  EXPECT_EQ(0, m.repeated_uint32_size());
  EXPECT_EQ(0, m.repeated_uint64_size());
  EXPECT_EQ(0, m.repeated_sint32_size());
  EXPECT_EQ(0, m.repeated_sint64_size());
  EXPECT_EQ(0, m.repeated_fixed32_size());
  EXPECT_EQ(0, m.repeated_fixed64_size());
  EXPECT_EQ(0, m.repeated_sfixed32_size());
  EXPECT_EQ(0, m.repeated_sfixed64_size());
  EXPECT_EQ(0, m.repeated_float_size());
  EXPECT_EQ(0, m.repeated_double_size());
  EXPECT_EQ(0, m.repeated_bool_size());
  EXPECT_EQ(0, m.repeated_string_size());
  EXPECT_EQ(0, m.repeated_bytes_size());
  EXPECT_EQ(0, m.repeated_nested_message_size());
  EXPECT_EQ(0, m.repeated_foreign_message_size());
  EXPECT_EQ(0, m.repeated_proto2_message_size());
  EXPECT_EQ(0, m.repeated_nested_enum_size());
  EXPECT_EQ(0, m.repeated_foreign_enum_size());
  EXPECT_EQ(0, m.repeated_lazy_message_size());
  EXPECT_EQ(proto2_nofieldpresence_unittest::TestAllTypes::ONEOF_FIELD_NOT_SET,
            m.oneof_field_case());
}

void FillValues(proto2_nofieldpresence_unittest::TestAllTypes* m) {
  m->set_optional_int32(100);
  m->set_optional_int64(101);
  m->set_optional_uint32(102);
  m->set_optional_uint64(103);
  m->set_optional_sint32(104);
  m->set_optional_sint64(105);
  m->set_optional_fixed32(106);
  m->set_optional_fixed64(107);
  m->set_optional_sfixed32(108);
  m->set_optional_sfixed64(109);
  m->set_optional_float(110.0);
  m->set_optional_double(111.0);
  m->set_optional_bool(true);
  m->set_optional_string("asdf");
  m->set_optional_bytes("jkl;");
  m->mutable_optional_nested_message()->set_bb(42);
  m->mutable_optional_foreign_message()->set_c(43);
  m->mutable_optional_proto2_message()->set_optional_int32(44);
  m->set_optional_nested_enum(
      proto2_nofieldpresence_unittest::TestAllTypes::BAZ);
  m->set_optional_foreign_enum(proto2_nofieldpresence_unittest::FOREIGN_BAZ);
  m->mutable_optional_lazy_message()->set_bb(45);
  m->add_repeated_int32(100);
  m->add_repeated_int64(101);
  m->add_repeated_uint32(102);
  m->add_repeated_uint64(103);
  m->add_repeated_sint32(104);
  m->add_repeated_sint64(105);
  m->add_repeated_fixed32(106);
  m->add_repeated_fixed64(107);
  m->add_repeated_sfixed32(108);
  m->add_repeated_sfixed64(109);
  m->add_repeated_float(110.0);
  m->add_repeated_double(111.0);
  m->add_repeated_bool(true);
  m->add_repeated_string("asdf");
  m->add_repeated_bytes("jkl;");
  m->add_repeated_nested_message()->set_bb(46);
  m->add_repeated_foreign_message()->set_c(47);
  m->add_repeated_proto2_message()->set_optional_int32(48);
  m->add_repeated_nested_enum(
      proto2_nofieldpresence_unittest::TestAllTypes::BAZ);
  m->add_repeated_foreign_enum(proto2_nofieldpresence_unittest::FOREIGN_BAZ);
  m->add_repeated_lazy_message()->set_bb(49);

  m->set_oneof_uint32(1);
  m->mutable_oneof_nested_message()->set_bb(50);
  m->set_oneof_string("test");  // only this one remains set
}

void CheckNonDefaultValues(
    const proto2_nofieldpresence_unittest::TestAllTypes& m) {
  EXPECT_EQ(100, m.optional_int32());
  EXPECT_EQ(101, m.optional_int64());
  EXPECT_EQ(102, m.optional_uint32());
  EXPECT_EQ(103, m.optional_uint64());
  EXPECT_EQ(104, m.optional_sint32());
  EXPECT_EQ(105, m.optional_sint64());
  EXPECT_EQ(106, m.optional_fixed32());
  EXPECT_EQ(107, m.optional_fixed64());
  EXPECT_EQ(108, m.optional_sfixed32());
  EXPECT_EQ(109, m.optional_sfixed64());
  EXPECT_EQ(110.0, m.optional_float());
  EXPECT_EQ(111.0, m.optional_double());
  EXPECT_EQ(true, m.optional_bool());
  EXPECT_EQ("asdf", m.optional_string());
  EXPECT_EQ("jkl;", m.optional_bytes());
  EXPECT_EQ(true, m.has_optional_nested_message());
  EXPECT_EQ(42, m.optional_nested_message().bb());
  EXPECT_EQ(true, m.has_optional_foreign_message());
  EXPECT_EQ(43, m.optional_foreign_message().c());
  EXPECT_EQ(true, m.has_optional_proto2_message());
  EXPECT_EQ(44, m.optional_proto2_message().optional_int32());
  EXPECT_EQ(proto2_nofieldpresence_unittest::TestAllTypes::BAZ,
            m.optional_nested_enum());
  EXPECT_EQ(proto2_nofieldpresence_unittest::FOREIGN_BAZ,
            m.optional_foreign_enum());
  EXPECT_EQ(true, m.has_optional_lazy_message());
  EXPECT_EQ(45, m.optional_lazy_message().bb());

  EXPECT_EQ(1, m.repeated_int32_size());
  EXPECT_EQ(100, m.repeated_int32(0));
  EXPECT_EQ(1, m.repeated_int64_size());
  EXPECT_EQ(101, m.repeated_int64(0));
  EXPECT_EQ(1, m.repeated_uint32_size());
  EXPECT_EQ(102, m.repeated_uint32(0));
  EXPECT_EQ(1, m.repeated_uint64_size());
  EXPECT_EQ(103, m.repeated_uint64(0));
  EXPECT_EQ(1, m.repeated_sint32_size());
  EXPECT_EQ(104, m.repeated_sint32(0));
  EXPECT_EQ(1, m.repeated_sint64_size());
  EXPECT_EQ(105, m.repeated_sint64(0));
  EXPECT_EQ(1, m.repeated_fixed32_size());
  EXPECT_EQ(106, m.repeated_fixed32(0));
  EXPECT_EQ(1, m.repeated_fixed64_size());
  EXPECT_EQ(107, m.repeated_fixed64(0));
  EXPECT_EQ(1, m.repeated_sfixed32_size());
  EXPECT_EQ(108, m.repeated_sfixed32(0));
  EXPECT_EQ(1, m.repeated_sfixed64_size());
  EXPECT_EQ(109, m.repeated_sfixed64(0));
  EXPECT_EQ(1, m.repeated_float_size());
  EXPECT_EQ(110.0, m.repeated_float(0));
  EXPECT_EQ(1, m.repeated_double_size());
  EXPECT_EQ(111.0, m.repeated_double(0));
  EXPECT_EQ(1, m.repeated_bool_size());
  EXPECT_EQ(true, m.repeated_bool(0));
  EXPECT_EQ(1, m.repeated_string_size());
  EXPECT_EQ("asdf", m.repeated_string(0));
  EXPECT_EQ(1, m.repeated_bytes_size());
  EXPECT_EQ("jkl;", m.repeated_bytes(0));
  EXPECT_EQ(1, m.repeated_nested_message_size());
  EXPECT_EQ(46, m.repeated_nested_message(0).bb());
  EXPECT_EQ(1, m.repeated_foreign_message_size());
  EXPECT_EQ(47, m.repeated_foreign_message(0).c());
  EXPECT_EQ(1, m.repeated_proto2_message_size());
  EXPECT_EQ(48, m.repeated_proto2_message(0).optional_int32());
  EXPECT_EQ(1, m.repeated_nested_enum_size());
  EXPECT_EQ(proto2_nofieldpresence_unittest::TestAllTypes::BAZ,
            m.repeated_nested_enum(0));
  EXPECT_EQ(1, m.repeated_foreign_enum_size());
  EXPECT_EQ(proto2_nofieldpresence_unittest::FOREIGN_BAZ,
            m.repeated_foreign_enum(0));
  EXPECT_EQ(1, m.repeated_lazy_message_size());
  EXPECT_EQ(49, m.repeated_lazy_message(0).bb());

  EXPECT_EQ(proto2_nofieldpresence_unittest::TestAllTypes::kOneofString,
            m.oneof_field_case());
  EXPECT_EQ("test", m.oneof_string());
}

TEST(NoFieldPresenceTest, BasicMessageTest) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  // Check default values, fill all fields, check values. We just want to
  // exercise the basic getters/setter paths here to make sure no
  // field-presence-related changes broke these.
  CheckDefaultValues(message);
  FillValues(&message);
  CheckNonDefaultValues(message);

  // Clear() should be equivalent to getting a freshly-constructed message.
  message.Clear();
  CheckDefaultValues(message);
}

TEST(NoFieldPresenceTest, MessageFieldPresenceTest) {
  // check that presence still works properly for message fields.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  EXPECT_EQ(false, message.has_optional_nested_message());
  // Getter should fetch default instance, and not cause the field to become
  // present.
  EXPECT_EQ(0, message.optional_nested_message().bb());
  EXPECT_EQ(false, message.has_optional_nested_message());
  message.mutable_optional_nested_message()->set_bb(42);
  EXPECT_EQ(true, message.has_optional_nested_message());
  message.clear_optional_nested_message();
  EXPECT_EQ(false, message.has_optional_nested_message());

  // Likewise for a lazy message field.
  EXPECT_EQ(false, message.has_optional_lazy_message());
  // Getter should fetch default instance, and not cause the field to become
  // present.
  EXPECT_EQ(0, message.optional_lazy_message().bb());
  EXPECT_EQ(false, message.has_optional_lazy_message());
  message.mutable_optional_lazy_message()->set_bb(42);
  EXPECT_EQ(true, message.has_optional_lazy_message());
  message.clear_optional_lazy_message();
  EXPECT_EQ(false, message.has_optional_lazy_message());

  // Test field presence of a message field on the default instance.
  EXPECT_EQ(false,
            proto2_nofieldpresence_unittest::TestAllTypes::default_instance()
                .has_optional_nested_message());
}

TEST(NoFieldPresenceTest, ReflectionHasFieldTest) {
  // check that HasField reports true on all scalar fields. Check that it
  // behaves properly for message fields.

  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  // Check initial state: scalars not present (due to need to be consistent with
  // MergeFrom()), message fields not present, oneofs not present.
  for (int i = 0; i < desc->field_count(); i++) {
    const FieldDescriptor* field = desc->field(i);
    if (field->is_repeated()) continue;
    EXPECT_EQ(false, r->HasField(message, field));
  }

  // Test field presence of a message field on the default instance.
  const FieldDescriptor* msg_field =
      desc->FindFieldByName("optional_nested_message");
  EXPECT_EQ(
      false,
      r->HasField(
          proto2_nofieldpresence_unittest::TestAllTypes::default_instance(),
          msg_field));

  // Fill all fields, expect everything to report true (check oneofs below).
  FillValues(&message);
  for (int i = 0; i < desc->field_count(); i++) {
    const FieldDescriptor* field = desc->field(i);
    if (field->is_repeated() || field->containing_oneof()) {
      continue;
    }
    if (field->options().ctype() != FieldOptions::STRING) {
      continue;
    }
    EXPECT_EQ(true, r->HasField(message, field));
  }

  message.Clear();

  // Check zero/empty-means-not-present semantics.
  const FieldDescriptor* field_int32 = desc->FindFieldByName("optional_int32");
  const FieldDescriptor* field_double =
      desc->FindFieldByName("optional_double");
  const FieldDescriptor* field_string =
      desc->FindFieldByName("optional_string");

  EXPECT_EQ(false, r->HasField(message, field_int32));
  EXPECT_EQ(false, r->HasField(message, field_double));
  EXPECT_EQ(false, r->HasField(message, field_string));

  message.set_optional_int32(42);
  EXPECT_EQ(true, r->HasField(message, field_int32));
  message.set_optional_int32(0);
  EXPECT_EQ(false, r->HasField(message, field_int32));

  message.set_optional_double(42.0);
  EXPECT_EQ(true, r->HasField(message, field_double));
  message.set_optional_double(0.0);
  EXPECT_EQ(false, r->HasField(message, field_double));

  message.set_optional_string("test");
  EXPECT_EQ(true, r->HasField(message, field_string));
  message.set_optional_string("");
  EXPECT_EQ(false, r->HasField(message, field_string));
}

// Given a message of type ForeignMessage or ExplicitForeignMessage that's also
// part of a map value, return whether its field |c| is present.
bool MapValueSubMessageHasFieldViaReflection(
    const google::protobuf::Message& map_submessage) {
  const Reflection* r = map_submessage.GetReflection();
  const Descriptor* desc = map_submessage.GetDescriptor();

  // "c" only exists in ForeignMessage or ExplicitForeignMessage, so an
  // assertion is necessary.
  ABSL_CHECK(absl::EndsWith(desc->name(), "ForeignMessage"));
  const FieldDescriptor* field = desc->FindFieldByName("c");

  return r->HasField(map_submessage, field);
}

TEST(NoFieldPresenceTest, GenCodeMapMissingKeyDeathTest) {
  proto2_nofieldpresence_unittest::TestAllTypes message;

  // Trying to find an unset key in a map would crash.
  EXPECT_DEATH(message.map_int32_bytes().at(9), "key not found");
}

TEST(NoFieldPresenceTest, GenCodeMapReflectionMissingKeyDeathTest) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");
  // Trying to get an unset map entry would crash in debug mode.
  EXPECT_DEBUG_DEATH(r->GetRepeatedMessage(message, field_map_int32_bytes, 0),
                     "index < current_size_");
}

TEST(NoFieldPresenceTest, ReflectionEmptyMapTest) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");
  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");
  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");
  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  ASSERT_NE(field_map_int32_bytes, nullptr);
  ASSERT_NE(field_map_int32_foreign_enum, nullptr);
  ASSERT_NE(field_map_int32_foreign_message, nullptr);
  ASSERT_NE(field_map_int32_explicit_foreign_message, nullptr);

  // Maps are treated as repeated fields -- so fieldsize should be zero.
  EXPECT_EQ(0, r->FieldSize(message, field_map_int32_bytes));
  EXPECT_EQ(0, r->FieldSize(message, field_map_int32_foreign_enum));
  EXPECT_EQ(0, r->FieldSize(message, field_map_int32_foreign_message));
  EXPECT_EQ(0, r->FieldSize(message, field_map_int32_explicit_foreign_message));
}

TEST(NoFieldPresenceTest, TestNonZeroMapEntriesStringValuePopulatedInGenCode) {
  // Set nonzero values for key-value pairs and test that.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  (*message.mutable_map_int32_bytes())[9] = "hello";

  EXPECT_EQ(1, message.map_int32_bytes().size());
  // Keys can be found.
  EXPECT_TRUE(message.map_int32_bytes().contains(9));
  // Values are counted properly.
  EXPECT_EQ(1, message.map_int32_bytes().count(9));
  // Value can be retrieved.
  EXPECT_EQ("hello", message.map_int32_bytes().at(9));

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestNonZeroMapEntriesIntValuePopulatedInGenCode) {
  // Set nonzero values for key-value pairs and test that.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  (*message.mutable_map_int32_foreign_enum())[99] = FOREIGN_BAZ;

  ASSERT_NE(0, static_cast<uint32_t>(FOREIGN_BAZ));

  EXPECT_EQ(1, message.map_int32_foreign_enum().size());
  // Keys can be found.
  EXPECT_TRUE(message.map_int32_foreign_enum().contains(99));
  // Values are counted properly.
  EXPECT_EQ(1, message.map_int32_foreign_enum().count(99));
  // Value can be retrieved.
  EXPECT_EQ(FOREIGN_BAZ, message.map_int32_foreign_enum().at(99));

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestNonZeroMapEntriesMessageValuePopulatedInGenCode) {
  // Set nonzero values for key-value pairs and test that.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  (*message.mutable_map_int32_foreign_message())[123].set_c(10101);

  EXPECT_EQ(1, message.map_int32_foreign_message().size());
  // Keys can be found.
  EXPECT_TRUE(message.map_int32_foreign_message().contains(123));
  // Values are counted properly.
  EXPECT_EQ(1, message.map_int32_foreign_message().count(123));
  // Value can be retrieved.
  EXPECT_EQ(10101, message.map_int32_foreign_message().at(123).c());

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest,
     TestNonZeroMapEntriesExplicitMessageValuePopulatedInGenCode) {
  // Set nonzero values for key-value pairs and test that.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  (*message.mutable_map_int32_explicit_foreign_message())[456].set_c(20202);

  EXPECT_EQ(1, message.map_int32_explicit_foreign_message().size());
  // Keys can be found.
  EXPECT_TRUE(message.map_int32_explicit_foreign_message().contains(456));
  // Values are counted properly.
  EXPECT_EQ(1, message.map_int32_explicit_foreign_message().count(456));
  // Value can be retrieved.
  EXPECT_EQ(20202, message.map_int32_explicit_foreign_message().at(456).c());

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestNonZeroStringMapEntriesHaveNoPresence) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_bytes())[9] = "hello";
  const google::protobuf::Message& bytes_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_bytes, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(bytes_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Primitive types inherit presence semantics from the map itself.
  EXPECT_THAT(bytes_map_entry, Not(MapEntryValueExplicitPresence()));
}

TEST(NoFieldPresenceTest, TestNonZeroIntMapEntriesHaveNoPresence) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_foreign_enum())[99] = FOREIGN_BAZ;

  const google::protobuf::Message& enum_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_foreign_enum, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(enum_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Primitive types inherit presence semantics from the map itself.
  EXPECT_THAT(enum_map_entry, Not(MapEntryValueExplicitPresence()));
}

TEST(NoFieldPresenceTest, TestNonZeroImplicitSubMessageMapEntriesHavePresence) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_foreign_message())[123].set_c(10101);

  const google::protobuf::Message& msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_foreign_message, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(msg_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Message types always have presence in proto3.
  EXPECT_THAT(msg_map_entry, MapEntryValueExplicitPresence());
}

TEST(NoFieldPresenceTest, TestNonZeroExplicitSubMessageMapEntriesHavePresence) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_explicit_foreign_message())[456].set_c(20202);

  const google::protobuf::Message& explicit_msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_explicit_foreign_message, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(explicit_msg_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Message types always have presence in proto3.
  EXPECT_THAT(explicit_msg_map_entry, MapEntryValueExplicitPresence());
}

TEST(NoFieldPresenceTest, TestNonZeroStringMapEntriesPopulatedInReflection) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_bytes())[9] = "hello";

  // Map entries show up on reflection.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_bytes));
  const google::protobuf::Message& bytes_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_bytes, /*index=*/0);

  // HasField for both key and value returns true.
  EXPECT_THAT(bytes_map_entry, MapEntryHasKey());
  EXPECT_THAT(bytes_map_entry, MapEntryHasValue());
}

TEST(NoFieldPresenceTest, TestNonZeroIntMapEntriesPopulatedInReflection) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");

  // Set nonzero values for key-value pairs and test that.
  ASSERT_NE(0, static_cast<uint32_t>(FOREIGN_BAZ));
  (*message.mutable_map_int32_foreign_enum())[99] = FOREIGN_BAZ;

  // Map entries show up on reflection.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_enum));
  const google::protobuf::Message& enum_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_foreign_enum, /*index=*/0);

  // HasField for both key and value returns true.
  EXPECT_THAT(enum_map_entry, MapEntryHasKey());
  EXPECT_THAT(enum_map_entry, MapEntryHasValue());
}

TEST(NoFieldPresenceTest,
     TestNonZeroSubMessageMapEntriesPopulatedInReflection) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");

  (*message.mutable_map_int32_foreign_message())[123].set_c(10101);

  // Map entries show up on reflection.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_message));
  const google::protobuf::Message& msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_foreign_message, /*index=*/0);

  // HasField for both key and value returns true.
  EXPECT_THAT(msg_map_entry, MapEntryHasKey());
  EXPECT_THAT(msg_map_entry, MapEntryHasValue());

  // For value types that are messages, further test that the message fields
  // show up on reflection.
  EXPECT_TRUE(MapValueSubMessageHasFieldViaReflection(
      message.map_int32_foreign_message().at(123)));
}

TEST(NoFieldPresenceTest,
     TestNonZeroExplicitSubMessageMapEntriesPopulatedInReflection) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  (*message.mutable_map_int32_explicit_foreign_message())[456].set_c(20202);

  // Map entries show up on reflection.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_explicit_foreign_message));
  const google::protobuf::Message& explicit_msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_explicit_foreign_message, /*index=*/0);

  // HasField for both key and value returns true.
  EXPECT_THAT(explicit_msg_map_entry, MapEntryHasKey());
  EXPECT_THAT(explicit_msg_map_entry, MapEntryHasValue());

  // For value types that are messages, further test that the message fields
  // show up on reflection.
  EXPECT_TRUE(MapValueSubMessageHasFieldViaReflection(
      message.map_int32_explicit_foreign_message().at(456)));
}

TEST(NoFieldPresenceTest, TestEmptyMapEntriesStringValuePopulatedInGenCode) {
  // Set zero values for zero keys and test that.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  (*message.mutable_map_int32_bytes())[0];

  // Zero keys are valid entries in gencode.
  EXPECT_EQ(1, message.map_int32_bytes().size());
  EXPECT_TRUE(message.map_int32_bytes().contains(0));
  EXPECT_EQ(1, message.map_int32_bytes().count(0));
  EXPECT_EQ("", message.map_int32_bytes().at(0));

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestEmptyMapEntriesIntValuePopulatedInGenCode) {
  // Set zero values for zero keys and test that.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  (*message.mutable_map_int32_foreign_enum())[0];

  EXPECT_EQ(1, message.map_int32_foreign_enum().size());
  EXPECT_TRUE(message.map_int32_foreign_enum().contains(0));
  EXPECT_EQ(1, message.map_int32_foreign_enum().count(0));
  EXPECT_EQ(0, message.map_int32_foreign_enum().at(0));

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestEmptyMapEntriesMessageValuePopulatedInGenCode) {
  // Set zero values for zero keys and test that.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  (*message.mutable_map_int32_foreign_message())[0];

  // ==== Gencode behaviour ====
  //
  // Zero keys are valid entries in gencode.
  EXPECT_EQ(1, message.map_int32_foreign_message().size());
  EXPECT_TRUE(message.map_int32_foreign_message().contains(0));
  EXPECT_EQ(1, message.map_int32_foreign_message().count(0));
  EXPECT_EQ(0, message.map_int32_foreign_message().at(0).c());

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest,
     TestEmptyMapEntriesExplicitMessageValuePopulatedInGenCode) {
  // Set zero values for zero keys and test that.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  (*message.mutable_map_int32_explicit_foreign_message())[0];

  // ==== Gencode behaviour ====
  //
  // Zero keys are valid entries in gencode.
  EXPECT_EQ(1, message.map_int32_explicit_foreign_message().size());
  EXPECT_TRUE(message.map_int32_explicit_foreign_message().contains(0));
  EXPECT_EQ(1, message.map_int32_explicit_foreign_message().count(0));
  EXPECT_EQ(0, message.map_int32_explicit_foreign_message().at(0).c());

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestEmptyStringMapEntriesHaveNoPresence) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_bytes())[0];
  const google::protobuf::Message& bytes_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_bytes, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(bytes_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Primitive types inherit presence semantics from the map itself.
  EXPECT_THAT(bytes_map_entry, Not(MapEntryValueExplicitPresence()));
}

TEST(NoFieldPresenceTest, TestEmptyIntMapEntriesHaveNoPresence) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_foreign_enum())[0];
  const google::protobuf::Message& enum_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_foreign_enum, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(enum_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Primitive types inherit presence semantics from the map itself.
  EXPECT_THAT(enum_map_entry, Not(MapEntryValueExplicitPresence()));
}

TEST(NoFieldPresenceTest, TestEmptySubMessageMapEntriesHavePresence) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_foreign_message())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_message));
  const google::protobuf::Message& msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_foreign_message, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(msg_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Message types always have presence in proto3.
  EXPECT_THAT(msg_map_entry, MapEntryValueExplicitPresence());
}

TEST(NoFieldPresenceTest, TestEmptyExplicitSubMessageMapEntriesHavePresence) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_explicit_foreign_message())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_explicit_foreign_message));
  const google::protobuf::Message& explicit_msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_explicit_foreign_message, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(explicit_msg_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Message types always have presence in proto3.
  EXPECT_THAT(explicit_msg_map_entry, MapEntryValueExplicitPresence());
}

TEST(NoFieldPresenceTest, TestEmptyStringMapEntriesPopulatedInReflection) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_bytes())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_bytes));
  const google::protobuf::Message& bytes_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_bytes, /*index=*/0);

  // If map entries are truly "no presence", then they should not return true
  // for HasField!
  // However, the existing behavior is that map entries behave like
  // explicit-presence fields in reflection -- i.e. they must return true for
  // HasField even though they are zero.
  EXPECT_THAT(bytes_map_entry, MapEntryHasKey());
  EXPECT_THAT(bytes_map_entry, MapEntryHasValue());
}

TEST(NoFieldPresenceTest, TestEmptyIntMapEntriesPopulatedInReflection) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_foreign_enum())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_enum));
  const google::protobuf::Message& enum_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_foreign_enum, /*index=*/0);

  // If map entries are truly "no presence", then they should not return true
  // for HasField!
  // However, the existing behavior is that map entries behave like
  // explicit-presence fields in reflection -- i.e. they must return true for
  // HasField even though they are zero.
  EXPECT_THAT(enum_map_entry, MapEntryHasKey());
  EXPECT_THAT(enum_map_entry, MapEntryHasValue());
}

TEST(NoFieldPresenceTest, TestEmptySubMessageMapEntriesPopulatedInReflection) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_foreign_message())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_message));
  const google::protobuf::Message& msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_foreign_message, /*index=*/0);

  // If map entries are truly "no presence", then they should not return true
  // for HasField!
  // However, the existing behavior is that map entries behave like
  // explicit-presence fields in reflection -- i.e. they must return true for
  // HasField even though they are zero.
  EXPECT_THAT(msg_map_entry, MapEntryHasKey());
  EXPECT_THAT(msg_map_entry, MapEntryHasValue());

  // For value types that are messages, further test that the message fields
  // do not show up on reflection.
  EXPECT_FALSE(MapValueSubMessageHasFieldViaReflection(
      message.map_int32_foreign_message().at(0)));
}

TEST(NoFieldPresenceTest,
     TestEmptyExplicitSubMessageMapEntriesPopulatedInReflection) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_explicit_foreign_message())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_explicit_foreign_message));
  const google::protobuf::Message& explicit_msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_explicit_foreign_message, /*index=*/0);

  // If map entries are truly "no presence", then they should not return true
  // for HasField!
  // However, the existing behavior is that map entries behave like
  // explicit-presence fields in reflection -- i.e. they must return true for
  // HasField even though they are zero.
  EXPECT_THAT(explicit_msg_map_entry, MapEntryHasKey());
  EXPECT_THAT(explicit_msg_map_entry, MapEntryHasValue());

  // For value types that are messages, further test that the message fields
  // do not show up on reflection.
  EXPECT_FALSE(MapValueSubMessageHasFieldViaReflection(
      message.map_int32_explicit_foreign_message().at(0)));
}

TEST(NoFieldPresenceTest, ReflectionClearFieldTest) {
  proto2_nofieldpresence_unittest::TestAllTypes message;

  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_int32 = desc->FindFieldByName("optional_int32");
  const FieldDescriptor* field_double =
      desc->FindFieldByName("optional_double");
  const FieldDescriptor* field_string =
      desc->FindFieldByName("optional_string");
  const FieldDescriptor* field_message =
      desc->FindFieldByName("optional_nested_message");
  const FieldDescriptor* field_lazy =
      desc->FindFieldByName("optional_lazy_message");

  message.set_optional_int32(42);
  r->ClearField(&message, field_int32);
  EXPECT_EQ(0, message.optional_int32());

  message.set_optional_double(42.0);
  r->ClearField(&message, field_double);
  EXPECT_EQ(0.0, message.optional_double());

  message.set_optional_string("test");
  r->ClearField(&message, field_string);
  EXPECT_EQ("", message.optional_string());

  message.mutable_optional_nested_message()->set_bb(1234);
  r->ClearField(&message, field_message);
  EXPECT_FALSE(message.has_optional_nested_message());
  EXPECT_EQ(0, message.optional_nested_message().bb());

  message.mutable_optional_lazy_message()->set_bb(42);
  r->ClearField(&message, field_lazy);
  EXPECT_FALSE(message.has_optional_lazy_message());
  EXPECT_EQ(0, message.optional_lazy_message().bb());
}

TEST(NoFieldPresenceTest, HasFieldOneofsTest) {
  // check that HasField behaves properly for oneofs.
  proto2_nofieldpresence_unittest::TestAllTypes message;

  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();
  const FieldDescriptor* desc_oneof_uint32 =
      desc->FindFieldByName("oneof_uint32");
  const FieldDescriptor* desc_oneof_nested_message =
      desc->FindFieldByName("oneof_nested_message");
  const FieldDescriptor* desc_oneof_string =
      desc->FindFieldByName("oneof_string");
  ABSL_CHECK(desc_oneof_uint32 != nullptr);
  ABSL_CHECK(desc_oneof_nested_message != nullptr);
  ABSL_CHECK(desc_oneof_string != nullptr);

  EXPECT_EQ(false, r->HasField(message, desc_oneof_uint32));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_nested_message));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_string));

  message.set_oneof_string("test");
  EXPECT_EQ(false, r->HasField(message, desc_oneof_uint32));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_nested_message));
  EXPECT_EQ(true, r->HasField(message, desc_oneof_string));
  message.mutable_oneof_nested_message()->set_bb(42);
  EXPECT_EQ(false, r->HasField(message, desc_oneof_uint32));
  EXPECT_EQ(true, r->HasField(message, desc_oneof_nested_message));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_string));

  message.Clear();
  EXPECT_EQ(false, r->HasField(message, desc_oneof_uint32));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_nested_message));
  EXPECT_EQ(false, r->HasField(message, desc_oneof_string));
}

TEST(NoFieldPresenceTest, MergeFromIfNonzeroTest) {
  // check that MergeFrom copies if nonzero/nondefault only.
  proto2_nofieldpresence_unittest::TestAllTypes source;
  proto2_nofieldpresence_unittest::TestAllTypes dest;

  dest.set_optional_int32(42);
  dest.set_optional_string("test");
  source.set_optional_int32(0);
  source.set_optional_string("");
  // MergeFrom() copies only if present in serialization, i.e., non-zero.
  dest.MergeFrom(source);
  EXPECT_EQ(42, dest.optional_int32());
  EXPECT_EQ("test", dest.optional_string());

  source.set_optional_int32(84);
  source.set_optional_string("test2");
  dest.MergeFrom(source);
  EXPECT_EQ(84, dest.optional_int32());
  EXPECT_EQ("test2", dest.optional_string());
}

TEST(NoFieldPresenceTest, ExtraZeroesInWireParseTest) {
  // check extra serialized zeroes on the wire are parsed into the object.
  ForeignMessage dest;
  dest.set_c(42);
  ASSERT_EQ(42, dest.c());

  // ExplicitForeignMessage has the same fields as ForeignMessage, but with
  // explicit presence instead of implicit presence.
  ExplicitForeignMessage source;
  source.set_c(0);
  std::string wire = source.SerializeAsString();
  ASSERT_THAT(wire, StrEq(absl::string_view{"\x08\x00", 2}));

  // The "parse" operation clears all fields before merging from wire.
  ASSERT_TRUE(dest.ParseFromString(wire));
  EXPECT_EQ(0, dest.c());
  std::string dest_data;
  EXPECT_TRUE(dest.SerializeToString(&dest_data));
  EXPECT_TRUE(dest_data.empty());
}

TEST(NoFieldPresenceTest, ExtraZeroesInWireMergeTest) {
  // check explicit zeros on the wire are merged into an implicit one.
  ForeignMessage dest;
  dest.set_c(42);
  ASSERT_EQ(42, dest.c());

  // ExplicitForeignMessage has the same fields as ForeignMessage, but with
  // explicit presence instead of implicit presence.
  ExplicitForeignMessage source;
  source.set_c(0);
  std::string wire = source.SerializeAsString();
  ASSERT_THAT(wire, StrEq(absl::string_view{"\x08\x00", 2}));

  // TODO: b/356132170 -- Add conformance tests to ensure this behaviour is
  //                      well-defined.
  // As implemented, the C++ "merge" operation does not distinguish between
  // implicit and explicit fields when reading from the wire.
  ASSERT_TRUE(dest.MergeFromString(wire));
  // If zero is present on the wire, the original value is overwritten, even
  // though this is specified as an "implicit presence" field.
  EXPECT_EQ(0, dest.c());
  std::string dest_data;
  EXPECT_TRUE(dest.SerializeToString(&dest_data));
  EXPECT_TRUE(dest_data.empty());
}

TEST(NoFieldPresenceTest, ExtraZeroesInWireLastWins) {
  // check that, when the same field is present multiple times on the wire, we
  // always take the last one -- even if it is a zero.

  absl::string_view wire{"\x08\x01\x08\x00", /*len=*/4};  // note the null-byte.
  ForeignMessage dest;

  // TODO: b/356132170 -- Add conformance tests to ensure this behaviour is
  //                      well-defined.
  // As implemented, the C++ "merge" operation does not distinguish between
  // implicit and explicit fields when reading from the wire.
  ASSERT_TRUE(dest.MergeFromString(wire));
  // If the same field is present multiple times on the wire, "last one wins".
  // i.e. -- the last seen field content will always overwrite, even if it's
  // zero and the field is implicit presence.
  EXPECT_EQ(0, dest.c());
  std::string dest_data;
  EXPECT_TRUE(dest.SerializeToString(&dest_data));
  EXPECT_TRUE(dest_data.empty());
}

TEST(NoFieldPresenceTest, IsInitializedTest) {
  // Check that IsInitialized works properly.
  proto2_nofieldpresence_unittest::TestProto2Required message;

  EXPECT_EQ(true, message.IsInitialized());
  message.mutable_proto2()->set_a(1);
  EXPECT_EQ(false, message.IsInitialized());
  message.mutable_proto2()->set_b(1);
  EXPECT_EQ(false, message.IsInitialized());
  message.mutable_proto2()->set_c(1);
  EXPECT_EQ(true, message.IsInitialized());
}

// TODO: b/358616816 - `if constexpr` can be used here once C++17 is baseline.
template <typename T>
bool TestSerialize(const MessageLite& message, T* output);

template <>
bool TestSerialize<std::string>(const MessageLite& message,
                                std::string* output) {
  return message.SerializeToString(output);
}

template <>
bool TestSerialize<absl::Cord>(const MessageLite& message, absl::Cord* output) {
  return message.SerializeToCord(output);
}

template <typename T>
class NoFieldPresenceSerializeTest : public testing::Test {
 public:
  T& GetOutputSinkRef() { return value_; }
  std::string GetOutput() { return std::string{value_}; }

 protected:
  // Cargo-culted from:
  // https://google.github.io/googletest/reference/testing.html#TYPED_TEST_SUITE
  T value_;
};

using SerializableOutputTypes = ::testing::Types<std::string, absl::Cord>;

// TODO: b/358616816 - `if constexpr` can be used here once C++17 is baseline.
// https://google.github.io/googletest/reference/testing.html#TYPED_TEST_SUITE
#ifdef __cpp_if_constexpr
// Providing the NameGenerator produces slightly more readable output in the
// test invocation summary (type names are displayed instead of numbers).
class NameGenerator {
 public:
  template <typename T>
  static std::string GetName(int) {
    if constexpr (std::is_same_v<T, std::string>) {
      return "string";
    } else if constexpr (std::is_same_v<T, absl::Cord>) {
      return "Cord";
    } else {
      static_assert(
          std::is_same_v<T, std::string> || std::is_same_v<T, absl::Cord>,
          "unsupported type");
    }
  }
};

TYPED_TEST_SUITE(NoFieldPresenceSerializeTest, SerializableOutputTypes,
                 NameGenerator);
#else
TYPED_TEST_SUITE(NoFieldPresenceSerializeTest, SerializableOutputTypes);
#endif

TYPED_TEST(NoFieldPresenceSerializeTest, DontSerializeDefaultValuesTest) {
  // check that serialized data contains only non-zero numeric fields/non-empty
  // string/byte fields.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  TypeParam& output_sink = this->GetOutputSinkRef();

  // All default values -> no output.
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(0, this->GetOutput().size());

  // Zero values -> still no output.
  message.set_optional_int32(0);
  message.set_optional_int64(0);
  message.set_optional_uint32(0);
  message.set_optional_uint64(0);
  message.set_optional_sint32(0);
  message.set_optional_sint64(0);
  message.set_optional_fixed32(0);
  message.set_optional_fixed64(0);
  message.set_optional_sfixed32(0);
  message.set_optional_sfixed64(0);
  message.set_optional_float(0);
  message.set_optional_double(0);
  message.set_optional_bool(false);
  message.set_optional_string("");
  message.set_optional_bytes("");
  message.set_optional_nested_enum(
      proto2_nofieldpresence_unittest::TestAllTypes::FOO);  // first enum entry
  message.set_optional_foreign_enum(
      proto2_nofieldpresence_unittest::FOREIGN_FOO);  // first enum entry

  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(0, this->GetOutput().size());

  message.set_optional_int32(1);
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(2, this->GetOutput().size());
  EXPECT_EQ("\x08\x01", this->GetOutput());

  message.set_optional_int32(0);
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(0, this->GetOutput().size());
}

TYPED_TEST(NoFieldPresenceSerializeTest, NullMutableSerializesEmpty) {
  // Check that, if mutable_foo() was called, but fields were not modified,
  // nothing is serialized on the wire.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  TypeParam& output_sink = this->GetOutputSinkRef();

  // All default values -> no output.
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_TRUE(this->GetOutput().empty());

  // No-op mutable calls -> no output.
  message.mutable_optional_string();
  message.mutable_optional_bytes();
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_TRUE(this->GetOutput().empty());

  // Assign to nonempty string -> some output.
  *message.mutable_optional_bytes() = "bar";
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_THAT(this->GetOutput().size(),
              Gt(3));  // 3-byte-long string + tag/value + len
}

TYPED_TEST(NoFieldPresenceSerializeTest, SetAllocatedAndReleaseTest) {
  // Check that setting an empty string via set_allocated_foo behaves properly;
  // Check that serializing after release_foo does not generate output for foo.
  proto2_nofieldpresence_unittest::TestAllTypes message;
  TypeParam& output_sink = this->GetOutputSinkRef();

  // All default values -> no output.
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_TRUE(this->GetOutput().empty());

  auto allocated_bytes = std::make_unique<std::string>("test");
  message.set_allocated_optional_bytes(allocated_bytes.release());
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_THAT(this->GetOutput().size(),
              Gt(4));  // 4-byte-long string + tag/value + len

  size_t former_output_size = this->GetOutput().size();

  auto allocated_string = std::make_unique<std::string>("");
  message.set_allocated_optional_string(allocated_string.release());
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  // empty string not serialized.
  EXPECT_EQ(former_output_size, this->GetOutput().size());

  auto bytes_ptr = absl::WrapUnique(message.release_optional_bytes());
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_TRUE(
      this->GetOutput().empty());  // released fields are not serialized.
}

TYPED_TEST(NoFieldPresenceSerializeTest, LazyMessageFieldHasBit) {
  // Check that has-bit interaction with lazy message works (has-bit before and
  // after lazy decode).
  proto2_nofieldpresence_unittest::TestAllTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();
  const FieldDescriptor* field = desc->FindFieldByName("optional_lazy_message");
  ABSL_CHECK(field != nullptr);

  EXPECT_EQ(false, message.has_optional_lazy_message());
  EXPECT_EQ(false, r->HasField(message, field));

  message.mutable_optional_lazy_message()->set_bb(42);
  EXPECT_EQ(true, message.has_optional_lazy_message());
  EXPECT_EQ(true, r->HasField(message, field));

  // Serialize and parse with a new message object so that lazy field on new
  // object is in unparsed state.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  proto2_nofieldpresence_unittest::TestAllTypes message2;
  message2.ParseFromString(this->GetOutput());

  EXPECT_EQ(true, message2.has_optional_lazy_message());
  EXPECT_EQ(true, r->HasField(message2, field));

  // Access field to force lazy parse.
  EXPECT_EQ(42, message.optional_lazy_message().bb());
  EXPECT_EQ(true, message2.has_optional_lazy_message());
  EXPECT_EQ(true, r->HasField(message2, field));
}

TYPED_TEST(NoFieldPresenceSerializeTest, OneofPresence) {
  proto2_nofieldpresence_unittest::TestAllTypes message;
  // oneof fields still have field presence -- ensure that this goes on the wire
  // even though its value is the empty string.
  message.set_oneof_string("");
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  // Tag: 113 --> tag is (113 << 3) | 2 (length delimited) = 906
  // varint: 0x8a 0x07
  // Length: 0x00
  EXPECT_EQ(3, this->GetOutput().size());
  EXPECT_EQ(static_cast<char>(0x8a), this->GetOutput().at(0));
  EXPECT_EQ(static_cast<char>(0x07), this->GetOutput().at(1));
  EXPECT_EQ(static_cast<char>(0x00), this->GetOutput().at(2));

  message.Clear();
  EXPECT_TRUE(message.ParseFromString(this->GetOutput()));
  EXPECT_EQ(proto2_nofieldpresence_unittest::TestAllTypes::kOneofString,
            message.oneof_field_case());

  // Also test int32 and enum fields.
  message.Clear();
  message.set_oneof_uint32(0);  // would not go on wire if ordinary field.
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(3, this->GetOutput().size());
  EXPECT_TRUE(message.ParseFromString(this->GetOutput()));
  EXPECT_EQ(proto2_nofieldpresence_unittest::TestAllTypes::kOneofUint32,
            message.oneof_field_case());

  message.Clear();
  message.set_oneof_enum(
      // FOO is the default value.
      proto2_nofieldpresence_unittest::TestAllTypes::FOO);
  ASSERT_TRUE(TestSerialize(message, &output_sink));
  EXPECT_EQ(3, this->GetOutput().size());
  EXPECT_TRUE(message.ParseFromString(this->GetOutput()));
  EXPECT_EQ(proto2_nofieldpresence_unittest::TestAllTypes::kOneofEnum,
            message.oneof_field_case());

  message.Clear();
  message.set_oneof_string("test");
  message.clear_oneof_string();
  EXPECT_EQ(0, message.ByteSizeLong());
}

TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripNonZeroKeyNonZeroString) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_bytes())[9] = "hello";

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ("hello", rt_msg.map_int32_bytes().at(9));
}

TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripNonZeroKeyNonZeroEnum) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  ASSERT_NE(static_cast<uint32_t>(FOREIGN_BAZ), 0);
  (*msg.mutable_map_int32_foreign_enum())[99] = FOREIGN_BAZ;

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_foreign_enum(),
              UnorderedPointwise(Eq(), msg.map_int32_foreign_enum()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(FOREIGN_BAZ, rt_msg.map_int32_foreign_enum().at(99));
}

TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripNonZeroKeyNonZeroMessage) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_foreign_message())[123].set_c(10101);

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_foreign_message().at(123).c(),
            msg.map_int32_foreign_message().at(123).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(10101, rt_msg.map_int32_foreign_message().at(123).c());
}

TYPED_TEST(NoFieldPresenceSerializeTest,
           MapRoundTripNonZeroKeyNonZeroExplicitSubMessage) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_explicit_foreign_message())[456].set_c(20202);

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_explicit_foreign_message().at(456).c(),
            msg.map_int32_explicit_foreign_message().at(456).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(20202, rt_msg.map_int32_explicit_foreign_message().at(456).c());

  // However, explicit presence messages expose a `has_foo` API.
  // Because map value is nonzero, they're expected to be present.
  EXPECT_TRUE(rt_msg.map_int32_explicit_foreign_message().at(456).has_c());
}

TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripZeroKeyNonZeroString) {
  // Because the map definitions all have int32 keys, testing one of them is
  // sufficient.
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_bytes())[0] = "hello";

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ("hello", rt_msg.map_int32_bytes().at(0));
}

// Note: "zero value" in this case means that the value is zero, but still
// explicitly assigned.
TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripZeroKeyZeroString) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_bytes())[0] = "";

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ("", rt_msg.map_int32_bytes().at(0));
}

TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripZeroKeyZeroEnum) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  ASSERT_EQ(static_cast<uint32_t>(FOREIGN_FOO), 0);
  (*msg.mutable_map_int32_foreign_enum())[0] = FOREIGN_FOO;

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_foreign_enum(),
              UnorderedPointwise(Eq(), msg.map_int32_foreign_enum()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(FOREIGN_FOO, rt_msg.map_int32_foreign_enum().at(0));
}

TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripZeroKeyZeroMessage) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_foreign_message())[0].set_c(0);

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_foreign_message().at(0).c(),
            msg.map_int32_foreign_message().at(0).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(0, rt_msg.map_int32_foreign_message().at(0).c());
}

TYPED_TEST(NoFieldPresenceSerializeTest,
           MapRoundTripZeroKeyZeroExplicitMessage) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_explicit_foreign_message())[0].set_c(0);

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_explicit_foreign_message().at(0).c(),
            msg.map_int32_explicit_foreign_message().at(0).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(0, rt_msg.map_int32_explicit_foreign_message().at(0).c());

  // However, explicit presence messages expose a `has_foo` API.
  // Because fields in an explicit message is explicitly set, they are expected
  // to be present.
  EXPECT_TRUE(rt_msg.map_int32_explicit_foreign_message().at(0).has_c());
}

// Note: "default value" in this case means that there is no explicit assignment
// to any value. Instead, map values are just created with operator[].
TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripZeroKeyDefaultString) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_bytes())[0];

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ("", rt_msg.map_int32_bytes().at(0));
}

// Note: "default value" in this case means that there is no explicit assignment
// to any value. Instead, map values are just created with operator[].
TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripZeroKeyDefaultEnum) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_foreign_enum())[0];

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(FOREIGN_FOO, rt_msg.map_int32_foreign_enum().at(0));
}

// Note: "default value" in this case means that there is no explicit assignment
// to any value. Instead, map values are just created with operator[].
TYPED_TEST(NoFieldPresenceSerializeTest, MapRoundTripZeroKeyDefaultMessage) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_foreign_message())[0];

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_foreign_message().at(0).c(),
            msg.map_int32_foreign_message().at(0).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(0, rt_msg.map_int32_foreign_message().at(0).c());
}

// Note: "default value" in this case means that there is no explicit assignment
// to any value. Instead, map values are just created with operator[].
TYPED_TEST(NoFieldPresenceSerializeTest,
           MapRoundTripZeroKeyDefaultExplicitMessage) {
  proto2_nofieldpresence_unittest::TestAllTypes msg;
  (*msg.mutable_map_int32_explicit_foreign_message())[0];

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  proto2_nofieldpresence_unittest::TestAllTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_explicit_foreign_message().at(0).c(),
            msg.map_int32_explicit_foreign_message().at(0).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(0, rt_msg.map_int32_explicit_foreign_message().at(0).c());

  // However, explicit presence messages expose a `has_foo` API.
  // Because fields in an explicit message is not set, they are not present.
  EXPECT_FALSE(rt_msg.map_int32_explicit_foreign_message().at(0).has_c());
}

}  // namespace
}  // namespace protobuf
}  // namespace google
