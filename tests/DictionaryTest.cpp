// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 Stéphane Caron
// Copyright 2024 Inria
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     Configuration and DataStore classes of mc_rtc
 *     Copyright 2015-2020 CNRS-UM LIRMM, CNRS-AIST JRL
 *     SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "cppcodec/base64_rfc4648.hpp"
#include "palimpsest/Dictionary.h"
#include "palimpsest/exceptions/KeyError.h"
#include "palimpsest/exceptions/TypeError.h"
#include "palimpsest/mpack/Writer.h"

using base64 = cppcodec::base64_rfc4648;

// This is only used in EigenOverloadOperatorNew but we need to overload the
// Eigen::aligned_allocator for this type.
struct Overloaded {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

// We cheat here since we know our code is going to use
// Eigen::aligned_allocator in such cases.
namespace Eigen {

template <>
struct aligned_allocator<Overloaded> : public std::allocator<Overloaded> {
  static bool used;
  Overloaded *allocate(size_t n, const void *hint = nullptr) {
    used = true;
    return std::allocator<Overloaded>::allocate(n, hint);
  }

  void deallocate(Overloaded *p, std::size_t n) {
    used = false;
    return std::allocator<Overloaded>::deallocate(p, n);
  }
};

bool aligned_allocator<Overloaded>::used = false;

}  // namespace Eigen

/*! Custom serializable type.
 *
 * A serializable type can be any type T for which a json::write<T>()
 * specialization is implemented.
 */
struct Serializable {
  int a;
  std::string b;
};

namespace palimpsest::json {

/*! Write custom serializable type as JSON.
 *
 * @param[out] stream Output stream.
 * @param[in] s Instance of the custom Serializable type defined above.
 */
template <>
void write(std::ostream &stream, const Serializable &s) {
  stream << "{\"a\": " << s.a << ", \"b\": \"" << s.b << "\"}";
}

}  // namespace palimpsest::json

namespace palimpsest {

using exceptions::KeyError;
using exceptions::TypeError;

class DictionaryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Eigen::VectorXd longer_vector(42);
    longer_vector.setOnes();

    // Insert one value for each type
    dict_.insert<bool>("sure", true);
    dict_.insert<int>("foo", 12);
    dict_.insert<int8_t>("int8_t", -8);
    dict_.insert<int16_t>("int16_t", -16);
    dict_.insert<int32_t>("int32_t", -32);
    dict_.insert<int64_t>("int64_t", -64);
    dict_.insert<uint8_t>("uint8_t", 8u);
    dict_.insert<uint16_t>("uint16_t", 16u);
    dict_.insert<uint32_t>("uint32_t", 32u);
    dict_.insert<uint64_t>("uint64_t", 64u);
    dict_.insert<float>("huns", 1.f / 9.f);
    dict_.insert<double>("blah", 12.12);
    dict_.insert<std::string>("bar", "de ligne");
    dict_.insert<Eigen::Vector2d>("tiny", 6., 4.);
    dict_.insert<Eigen::Vector3d>("position", 1., 2., 3.);
    dict_.insert<Eigen::Quaterniond>("orientation", 0., 1., 0., 0.);
    dict_.insert<Eigen::Matrix3d>("inertia", 12. * Eigen::Matrix3d::Identity());
    dict_.insert<Eigen::VectorXd>("longer_vector", longer_vector);
  }

  Dictionary dict_;
};

TEST(Dictionary, TestDictionary) {
  Dictionary dict;
  dict.insert<std::vector<double>>("data", 4, 42.0);
  auto &data = dict.get<std::vector<double>>("data");
  ASSERT_EQ(data.size(), 4);
  ASSERT_NEAR(data[0], 42, 1e-10);
  ASSERT_NEAR(data[1], 42, 1e-10);
  ASSERT_NEAR(data[2], 42, 1e-10);
  ASSERT_NEAR(data[3], 42, 1e-10);
  ASSERT_THROW(dict.get<double>("data"), TypeError);
  ASSERT_THROW(dict.get<std::vector<int>>("data"), TypeError);
  ASSERT_THROW(dict.get<std::vector<double>>("inexistent"), KeyError);

  data.resize(100);
  auto &data2 = dict.get<std::vector<double>>("data");
  ASSERT_TRUE(data2.size() == data.size());
  const auto &constData = dict.get<std::vector<double>>("data");
  ASSERT_TRUE(data2.size() == constData.size());

  struct CustomType {
    int answer;
    std::string name;
  };

  // Test insert
  dict.insert<CustomType>("key", CustomType{42, "Test"});
  ASSERT_EQ(dict.get<CustomType>("key").answer, 42);
  ASSERT_EQ(dict.get<CustomType>("key").name, "Test");

  // Remove object
  dict.remove("key");
  ASSERT_FALSE(dict.has("key"));
  ASSERT_TRUE(dict.has("data"));

  // Try to remove when it's not there
  dict.remove("key");
  ASSERT_FALSE(dict.has("key"));
  ASSERT_TRUE(dict.has("data"));

  // Test getters
  dict.insert<double>("assign", 42);
  double value = 0;
  dict.get("assign", /* default_value = */ value);  // no effect
  ASSERT_DOUBLE_EQ(value, 0);
  ASSERT_EQ(dict.get("assign_inexistent", 12), 12);
  ASSERT_FALSE(dict.get("has_feature", false));
}

TEST(Dictionary, GettingChildCreatesBranch) {
  Dictionary dict;
  ASSERT_THROW(dict("Roger")("le").get<double>("tavernier"), KeyError);
  ASSERT_TRUE(dict.has("Roger"));
  ASSERT_TRUE(dict("Roger").has("le"));
  ASSERT_FALSE(dict("Roger")("le").has("tavernier"));
}

TEST(Dictionary, TestDictionaryInheritance) {
  Dictionary dict;

  struct A {
    explicit A(const std::string &name) : name_(name) {}
    virtual std::string hello() const { return "A::Hello " + name_; }
    void printHello() const { std::cout << hello() << std::endl; }
    std::string type() const { return "A"; }
    std::string name_;
  };

  struct B : public A {
    explicit B(const std::string &name) : A(name) {}
    std::string hello() const override { return "B::Hello " + name_; }
    std::string type() const { return "B"; }
  };

  // Creating an inherited object and checking virtual inheritance
  dict.insert<B, A>("b", "World");
  auto &a = dict.get<A>("b");
  // Checking member function
  ASSERT_EQ(a.type(), "A");
  // Checking virtual function
  ASSERT_EQ(a.hello(), "B::Hello World");
  auto &b = dict.get<B>("b");
  ASSERT_EQ(b.type(), "B");
  ASSERT_EQ(b.hello(), "B::Hello World");

  // Extra check on A alone
  dict.insert<A>("a", "Parent");
  auto &a_of_a = dict.get<A>("a");
  ASSERT_EQ(a_of_a.hello(), "A::Hello Parent");
}

TEST(Dictionary, TestRemove) {
  constexpr int kBeforeConstructor = 0;
  constexpr int kAfterConstructor = 1;
  constexpr int kAfterDestructor = 2;

  struct TokenHolder {
    TokenHolder(const std::string &token, int &int_ref)
        : token(token), int_ref(int_ref) {
      int_ref = kAfterConstructor;
    }
    ~TokenHolder() { int_ref = kAfterDestructor; }
    std::string token;
    int &int_ref;
  };

  int stage = kBeforeConstructor;
  Dictionary dict;
  ASSERT_EQ(stage, kBeforeConstructor);
  dict.insert<TokenHolder>("some_key", "token_value", stage);
  {
    ASSERT_EQ(stage, kAfterConstructor);
    auto &object = dict.get<TokenHolder>("some_key");
    ASSERT_EQ(object.token, "token_value");
    ASSERT_TRUE(dict.has("some_key"));
    dict.remove("some_key");
    ASSERT_EQ(stage, kAfterDestructor);
    ASSERT_TRUE(!dict.has("some_key"));
  }
}

// EIGEN_MAKE_ALIGNED_OPERATOR_NEW is no-op in C++17 from 3.4.0
#if !EIGEN_VERSION_AT_LEAST(3, 4, 0)
TEST(Dictionary, EigenOverloadOperatorNew) {
  Dictionary dict;
  ASSERT_FALSE(Eigen::aligned_allocator<Overloaded>::used);
  dict.insert<Overloaded>("overloaded");
  ASSERT_TRUE(Eigen::aligned_allocator<Overloaded>::used);
  dict.remove("overloaded");
  ASSERT_FALSE(Eigen::aligned_allocator<Overloaded>::used);
}
#endif

TEST(Dictionary, Strings) {
  Dictionary dict;
  dict("foo") = "bar";
  dict("bar") = "foo";
  ASSERT_EQ(dict("foo").as<std::string>(), "bar");
  ASSERT_EQ(dict("bar").as<std::string>(), "foo");
  ASSERT_THROW(dict("foo").as<const char *>(), TypeError);
  ASSERT_THROW(dict("bar").as<const char *>(), TypeError);
}

TEST(Dictionary, Quaternions) {
  Dictionary dict;
  dict.insert<Eigen::Quaterniond>("orientation");
  auto &orientation = dict.get<Eigen::Quaterniond>("orientation");

  orientation = {1., 0., 0., 0.};
  ASSERT_DOUBLE_EQ(dict.get<Eigen::Quaterniond>("orientation").w(), 1.);
  ASSERT_DOUBLE_EQ(dict.get<Eigen::Quaterniond>("orientation").x(), 0.);
  ASSERT_DOUBLE_EQ(dict.get<Eigen::Quaterniond>("orientation").y(), 0.);
  ASSERT_DOUBLE_EQ(dict.get<Eigen::Quaterniond>("orientation").z(), 0.);

  orientation = {0., 0., 12., 0.};
  orientation.normalize();
  ASSERT_DOUBLE_EQ(dict.get<Eigen::Quaterniond>("orientation").w(), 0.);
  ASSERT_DOUBLE_EQ(dict.get<Eigen::Quaterniond>("orientation").x(), 0.);
  ASSERT_DOUBLE_EQ(dict.get<Eigen::Quaterniond>("orientation").y(), 1.);
  ASSERT_DOUBLE_EQ(dict.get<Eigen::Quaterniond>("orientation").z(), 0.);
}

TEST(Dictionary, WriteEmptyJSON) {
  Dictionary dict;
  std::ostringstream oss;
  oss << dict;
  ASSERT_EQ(oss.str(), "{}");
}

TEST(Dictionary, WriteJSON) {
  Dictionary dict;
  dict.insert<int>("test", 1);
  std::ostringstream oss;
  oss << dict;
  ASSERT_EQ(oss.str(), "{\"test\": 1}");
}

TEST(Dictionary, WriteMoreJSON) {
  Dictionary dict;
  dict.insert<int>("test", 1);
  dict.insert<int>("nice", 2);
  std::ostringstream oss;
  oss << dict;
  std::string case_1 = "{\"test\": 1, \"nice\": 2}";
  std::string case_2 = "{\"nice\": 2, \"test\": 1}";
  ASSERT_TRUE(oss.str() == case_1 || oss.str() == case_2);
}

TEST(Dictionary, WriteCustomTypeJSON) {
  Dictionary dict;
  dict.insert<Serializable>("foo", Serializable{1, "bar"});
  std::ostringstream oss;
  oss << dict;
  ASSERT_EQ(oss.str(), "{\"foo\": {\"a\": 1, \"b\": \"bar\"}}");
}

TEST(Dictionary, WriteUnknownTypeJSON) {
  struct UnknownType {
    std::string mystery;
  };
  Dictionary dict;
  dict.insert<UnknownType>("unknown", UnknownType{"???"});
  std::ostringstream oss;
  oss << dict;
  ASSERT_TRUE(oss.str().find("<typeid:") != std::string::npos);
}

TEST_F(DictionaryTest, Serialization) {
  auto &position = dict_.get<Eigen::Vector3d>("position");
  auto &orientation = dict_.get<Eigen::Quaterniond>("orientation");
  auto &inertia = dict_.get<Eigen::Matrix3d>("inertia");
  auto &longer_vector = dict_.get<Eigen::VectorXd>("longer_vector");

  // Serialize to buffer
  std::vector<char> buffer;
  size_t size = dict_.serialize(buffer);
  ASSERT_GT(buffer.size(), 0);
  ASSERT_LT(size, buffer.size());  // size is usually < MPACK_BUFFER_SIZE

  Dictionary checker;
  checker.insert<bool>("sure", false);
  checker.insert<int>("foo", 0);
  checker.insert<int8_t>("int8_t", 0);
  checker.insert<int16_t>("int16_t", 0);
  checker.insert<int32_t>("int32_t", 0);
  checker.insert<int64_t>("int64_t", 0);
  checker.insert<uint8_t>("uint8_t", 0u);
  checker.insert<uint16_t>("uint16_t", 0u);
  checker.insert<uint32_t>("uint32_t", 0u);
  checker.insert<uint64_t>("uint64_t", 0u);
  checker.insert<float>("huns", 0.f);
  checker.insert<double>("blah", 0.);
  checker.insert<std::string>("bar", "");
  checker.insert<Eigen::Vector2d>("tiny", 0., 0.);
  checker.insert<Eigen::Vector3d>("position", 0., 0., 0.);
  checker.insert<Eigen::Quaterniond>("orientation", 0., 0., 0., 0.);
  checker.insert<Eigen::Matrix3d>("inertia", Eigen::Matrix3d::Zero());
  checker.insert<Eigen::VectorXd>("longer_vector", Eigen::VectorXd(42));

  // Deserialize and check that recover all serialized values
  checker.deserialize(buffer.data(), size);
  ASSERT_TRUE(checker("sure"));
  ASSERT_EQ(checker("foo").as<int>(), 12);
  ASSERT_EQ(checker("int8_t").as<int8_t>(), -8);
  ASSERT_EQ(checker("int16_t").as<int16_t>(), -16);
  ASSERT_EQ(checker("int32_t").as<int32_t>(), -32);
  ASSERT_EQ(checker("int64_t").as<int64_t>(), -64);
  ASSERT_EQ(checker("uint8_t").as<uint8_t>(), 8u);
  ASSERT_EQ(checker("uint16_t").as<uint16_t>(), 16u);
  ASSERT_EQ(checker("uint32_t").as<uint32_t>(), 32u);
  ASSERT_EQ(checker("uint64_t").as<uint64_t>(), 64u);
  ASSERT_FLOAT_EQ(checker("huns"), 1.f / 9.f);
  ASSERT_DOUBLE_EQ(checker("blah"), 12.12);
  ASSERT_EQ(checker("bar").as<std::string>(), "de ligne");
  ASSERT_TRUE(
      checker("tiny").as<Eigen::Vector2d>().isApprox(Eigen::Vector2d(6., 4.)));
  ASSERT_TRUE(checker("position").as<Eigen::Vector3d>().isApprox(position));
  ASSERT_TRUE(
      checker("orientation").as<Eigen::Quaterniond>().isApprox(orientation));
  ASSERT_TRUE(checker("inertia").as<Eigen::Matrix3d>().isApprox(inertia));
  ASSERT_TRUE(
      checker("longer_vector").as<Eigen::VectorXd>().isApprox(longer_vector));

  // Insert child dictionaries
  dict_("child_1").insert<Eigen::Vector3d>("position", -1, 1, 0.);
  dict_("child_1").insert<Eigen::Quaterniond>("orientation", 12., -1., 4., 5.);
  dict_("child_2").insert<Eigen::Vector3d>("position", -1, 1, 0.);

  // Serialize to buffer
  size = dict_.serialize(buffer);
  ASSERT_GT(buffer.size(), 0);
  ASSERT_NE(size, buffer.size());

  // Deserialize and check structure
  checker.clear();
  checker("child_1").insert<Eigen::Vector3d>("position", 0., 0., 0.);
  checker("child_1").insert<Eigen::Quaterniond>("orientation", 0., 0., 1., 0.);
  checker("child_2").insert<Eigen::Vector3d>("position", 0., 0., 0.);
  checker.deserialize(buffer.data(), size);
  ASSERT_TRUE(checker.has("child_1"));
  ASSERT_TRUE(checker.has("child_2"));
  ASSERT_TRUE(
      dict_("child_1")
          .get<Eigen::Vector3d>("position")
          .isApprox(checker("child_1")("position").as<Eigen::Vector3d>()));
  ASSERT_TRUE(
      dict_("child_1")
          .get<Eigen::Quaterniond>("orientation")
          .normalized()  // quaternion may be normalized when deserialized
          .isApprox(checker("child_1")("orientation")
                        .as<Eigen::Quaterniond>()
                        .normalized()));
  ASSERT_TRUE(
      dict_("child_2")
          .get<Eigen::Vector3d>("position")
          .isApprox(checker("child_2")("position").as<Eigen::Vector3d>()));
}

TEST(Dictionary, AsCastThenSerialize) {
  Dictionary dict;
  dict.insert<int>("well", -10);
  ASSERT_EQ(dict("well").as<int>(), dict.get<int>("well"));
  ASSERT_EQ(dict("well").as<int>(), -10);

  // Go straight for a deep quaternion!
  dict("this")("is")("quite")("deep").insert<Eigen::Quaterniond>("quat", 0., 0.,
                                                                 1., 0.);
  Eigen::Quaterniond deep_truth = Eigen::Quaterniond(0., 0., 1., 0.);
  auto &deep_quat =
      dict("this")("is")("quite")("deep")("quat").as<Eigen::Quaterniond>();
  const auto &deep_const_quat =
      dict("this")("is")("quite")("deep")("quat").as<Eigen::Quaterniond>();
  ASSERT_TRUE(deep_quat.isApprox(deep_truth));
  ASSERT_TRUE(deep_const_quat.isApprox(deep_truth));
  ASSERT_TRUE(dict("this")("is")("quite")("deep")("quat")
                  .as<Eigen::Quaterniond>()
                  .isApprox(deep_truth));

  // Serialize to buffer
  std::vector<char> buffer;
  auto size = dict.serialize(buffer);
  ASSERT_GT(buffer.size(), 0);
  ASSERT_NE(size, buffer.size());

  // Deserialize and check structure
  Dictionary checker;
  checker("well") = -10;
  checker("this")("is")("quite")("deep").insert<Eigen::Quaterniond>("quat", 1.,
                                                                    0., 0., 0.);
  checker.deserialize(buffer.data(), size);
  ASSERT_EQ(checker("well").as<int>(), -10);
  ASSERT_TRUE(checker("this")("is")("quite")("deep")("quat")
                  .as<Eigen::Quaterniond>()
                  .isApprox(deep_truth));
}

TEST(Dictionary, KnownMessagePackSize) {
  Dictionary dict;
  dict("id") = 12;
  std::vector<char> buffer;
  size_t size = dict.serialize(buffer);
  ASSERT_EQ(size, 5);
}

TEST(Dictionary, TypeErrorWhenSerializingUnknownType) {
  struct UnknownType {
    std::string mystery;
  };
  Dictionary dict;
  dict.insert<UnknownType>("unknown", UnknownType{"???"});
  std::vector<char> buffer;
  ASSERT_THROW(dict.serialize(buffer), TypeError);
}

TEST(Dictionary, DeserializeUnsignedWhenPossible) {
  // >>> config = {"int": -1, "maybe_uint": 1}
  // >>> print(base64.b64encode(msgpack.packb(config)).decode("utf-8"))
  const std::string packed = "gqNpbnT/qm1heWJlX3VpbnQB";
  std::vector<char> decoded = base64::decode<std::vector<char>>(packed);

  Dictionary dict;
  dict.deserialize(decoded.data(), decoded.size());
  ASSERT_EQ(dict.get<int>("int"), -1);
  ASSERT_EQ(dict.get<unsigned>("maybe_uint"), 1);
  ASSERT_THROW(dict.get<int>("maybe_uint"), TypeError);
}

TEST(Dictionary, DictionaryCannotCastToValue) {
  Dictionary dict;
  dict.insert<int>("well", -10);
  ASSERT_THROW(dict.as<bool>(), std::runtime_error);
}

TEST(Dictionary, CastingToIncorrectTypeThrows) {
  Dictionary dict;
  dict.insert<int>("well", -10);

  // Check as<T>() cast
  ASSERT_NO_THROW(dict("well").as<int>());
  ASSERT_THROW(dict("well").as<bool>(), std::runtime_error);

  // Check get<T>() cast
  ASSERT_NO_THROW(dict.get<int>("well"));
  ASSERT_THROW(dict.get<bool>("well"), std::runtime_error);

  // Check implicit conversion
  int well;
  bool not_well = false;
  ASSERT_NO_THROW(well = dict("well"));
  ASSERT_THROW(not_well = dict("well"), std::runtime_error);
  ASSERT_EQ(well, -10);    // don't warn about unused variable
  ASSERT_FALSE(not_well);  // idem
}

TEST(Dictionary, CannotInsertInsideValues) {
  Dictionary dict;
  dict.insert<int>("well", -10);

  // Cannot insert a value inside a value
  ASSERT_THROW(dict("well").insert<int>("aleph", 0), TypeError);

  // Cannot insert/lookup a dictionary inside a value
  ASSERT_THROW(dict("well")("well..."), std::runtime_error);
}

TEST_F(DictionaryTest, ImplicitConversion) {
  bool sure = dict_("sure");
  bool &ref_sure = dict_("sure");
  const bool &const_ref_sure = dict_("sure");
  ASSERT_EQ(sure, ref_sure);
  ASSERT_EQ(sure, const_ref_sure);
}

TEST_F(DictionaryTest, ReinsertionIsIgnored) {
  ASSERT_TRUE(dict_.get<bool>("sure"));
  dict_.insert<bool>("sure", false);
  ASSERT_TRUE(dict_.get<bool>("sure"));
}

TEST_F(DictionaryTest, ImplicitConversionCopies) {
  bool sure = dict_("sure");
  int foo = dict_("foo");
  int8_t int8 = dict_("int8_t");
  int16_t int16 = dict_("int16_t");
  int32_t int32 = dict_("int32_t");
  int64_t int64 = dict_("int64_t");
  uint8_t uint8 = dict_("uint8_t");
  uint16_t uint16 = dict_("uint16_t");
  uint32_t uint32 = dict_("uint32_t");
  uint64_t uint64 = dict_("uint64_t");
  float huns = dict_("huns");
  double blah = dict_("blah");
  std::string bar = dict_("bar");
  Eigen::Vector2d tiny = dict_("tiny");
  Eigen::Vector3d position = dict_("position");
  Eigen::Quaterniond orientation = dict_("orientation");
  Eigen::Matrix3d inertia = dict_("inertia");
  Eigen::VectorXd longer_vector = dict_("longer_vector");

  // Check that copies all have the right values
  ASSERT_EQ(sure, dict_.get<bool>("sure"));
  ASSERT_EQ(foo, dict_.get<int>("foo"));
  ASSERT_EQ(int8, dict_.get<int8_t>("int8_t"));
  ASSERT_EQ(int16, dict_.get<int16_t>("int16_t"));
  ASSERT_EQ(int32, dict_.get<int32_t>("int32_t"));
  ASSERT_EQ(int64, dict_.get<int64_t>("int64_t"));
  ASSERT_EQ(uint8, dict_.get<uint8_t>("uint8_t"));
  ASSERT_EQ(uint16, dict_.get<uint16_t>("uint16_t"));
  ASSERT_EQ(uint32, dict_.get<uint32_t>("uint32_t"));
  ASSERT_EQ(uint64, dict_.get<uint64_t>("uint64_t"));
  ASSERT_EQ(huns, dict_.get<float>("huns"));
  ASSERT_EQ(blah, dict_.get<double>("blah"));
  ASSERT_EQ(bar, dict_.get<std::string>("bar"));
  ASSERT_TRUE(tiny.isApprox(dict_.get<Eigen::Vector2d>("tiny")));
  ASSERT_TRUE(position.isApprox(dict_.get<Eigen::Vector3d>("position")));
  ASSERT_TRUE(
      orientation.isApprox(dict_.get<Eigen::Quaterniond>("orientation")));
  ASSERT_TRUE(inertia.isApprox(dict_.get<Eigen::Matrix3d>("inertia")));
  ASSERT_TRUE(
      longer_vector.isApprox(dict_.get<Eigen::VectorXd>("longer_vector")));
}

TEST_F(DictionaryTest, ImplicitConversionReferences) {
  bool &sure = dict_("sure");
  int &foo = dict_("foo");
  int8_t &int8 = dict_("int8_t");
  int16_t &int16 = dict_("int16_t");
  int32_t &int32 = dict_("int32_t");
  int64_t &int64 = dict_("int64_t");
  uint8_t &uint8 = dict_("uint8_t");
  uint16_t &uint16 = dict_("uint16_t");
  uint32_t &uint32 = dict_("uint32_t");
  uint64_t &uint64 = dict_("uint64_t");
  float &huns = dict_("huns");
  double &blah = dict_("blah");
  std::string &bar = dict_("bar");
  Eigen::Vector2d &tiny = dict_("tiny");
  Eigen::Vector3d &position = dict_("position");
  Eigen::Quaterniond &orientation = dict_("orientation");
  Eigen::Matrix3d &inertia = dict_("inertia");
  Eigen::VectorXd &longer_vector = dict_("longer_vector");

  // Change them all!
  sure = !sure;
  ++foo;
  --int8;
  --int16;
  --int32;
  --int64;
  ++uint8;
  ++uint16;
  ++uint32;
  ++uint64;
  huns *= huns;
  blah *= blah;
  bar += " ou pas";
  tiny.x() = 7.;
  position.y() = -3.;
  orientation.w() *= 2;
  orientation.normalize();
  inertia /= 3.9321883;
  inertia(0, 1) = 1.;
  inertia(1, 0) = 1.;
  longer_vector.setZero();

  // Check that dictionary values still match
  ASSERT_EQ(sure, dict_.get<bool>("sure"));
  ASSERT_EQ(foo, dict_.get<int>("foo"));
  ASSERT_EQ(int8, dict_.get<int8_t>("int8_t"));
  ASSERT_EQ(int16, dict_.get<int16_t>("int16_t"));
  ASSERT_EQ(int32, dict_.get<int32_t>("int32_t"));
  ASSERT_EQ(int64, dict_.get<int64_t>("int64_t"));
  ASSERT_EQ(uint8, dict_.get<uint8_t>("uint8_t"));
  ASSERT_EQ(uint16, dict_.get<uint16_t>("uint16_t"));
  ASSERT_EQ(uint32, dict_.get<uint32_t>("uint32_t"));
  ASSERT_EQ(uint64, dict_.get<uint64_t>("uint64_t"));
  ASSERT_EQ(huns, dict_.get<float>("huns"));
  ASSERT_EQ(blah, dict_.get<double>("blah"));
  ASSERT_EQ(bar, dict_.get<std::string>("bar"));
  ASSERT_TRUE(tiny.isApprox(dict_.get<Eigen::Vector2d>("tiny")));
  ASSERT_TRUE(position.isApprox(dict_.get<Eigen::Vector3d>("position")));
  ASSERT_TRUE(
      orientation.isApprox(dict_.get<Eigen::Quaterniond>("orientation")));
  ASSERT_TRUE(inertia.isApprox(dict_.get<Eigen::Matrix3d>("inertia")));
  ASSERT_TRUE(
      longer_vector.isApprox(dict_.get<Eigen::VectorXd>("longer_vector")));
}

TEST(Dictionary, ForbiddenCast) {
  Dictionary dict;
  dict.insert<int>("id", 1);
  ASSERT_NO_THROW(dict.get<int>("id"));
  ASSERT_THROW(dict.get<unsigned>("id"), std::runtime_error);
}

TEST(Dictionary, Clear) {
  Dictionary dict;
  // dict("test") = "here goes";  // won't compile, see the doc of operator=
  dict("test") = std::string("here goes");
  dict("nested").insert<double>("key", 1.);
  ASSERT_TRUE(dict.has("test"));
  ASSERT_TRUE(dict.has("nested"));
  ASSERT_TRUE(dict("nested").has("key"));
  ASSERT_DOUBLE_EQ(dict("nested")("key"), 1.);
  dict.clear();
  ASSERT_FALSE(dict.has("test"));
}

TEST(Dictionary, ChecksSignedness) {
  Dictionary foo;
  foo("bar") = 11111;  // int
  ASSERT_NO_THROW(foo("bar").as<int>());
  ASSERT_THROW(foo("bar").as<unsigned>(), std::runtime_error);
}

TEST(Dictionary, ReturnValue) {
  auto return_foo_dict = []() -> Dictionary {
    Dictionary foo;
    foo("bar") = 11111;
    foo("nested")("item") = std::string("unique_pointers_on_the_way");
    return foo;
  };

  Dictionary bar = return_foo_dict();
  ASSERT_EQ(bar("bar").as<int>(), 11111);
  ASSERT_EQ(bar("nested")("item").as<std::string>(),
            std::string("unique_pointers_on_the_way"));
}

TEST(Dictionary, NestedInsert) {
  Dictionary foo;
  foo("bar")("bar") = 12;
  ASSERT_EQ(foo("bar")("bar").as<int>(), 12);
}

TEST(Dictionary, NestedDictionaries) {
  Dictionary dict;
  dict.insert<int>("foo", 0);
  dict.insert<std::string>("bar", "forty-two");

  auto &nested = dict("this")("is")("nested");
  nested.insert<int>("answer", 42);
  ASSERT_EQ(dict("this")("is")("nested").get<int>("answer"),
            nested.get<int>("answer"));

  auto &brother = dict("this")("is")("close");
  brother.insert<int>("huns_invasion", 11111);
  ASSERT_EQ(dict("this")("is")("close").get<int>("huns_invasion"), 11111);
  ASSERT_EQ(dict("this")("is")("nested").get<int>("answer"), 42);
}

TEST(Dictionary, CannotGetValueFromEmpty) {
  Dictionary foo;
  double x = 0.;
  ASSERT_THROW(x = foo, std::runtime_error);
  ASSERT_THROW(x = foo("bar"), std::runtime_error);
  ASSERT_THROW(x = foo("bar")("blah"), std::runtime_error);
  ASSERT_DOUBLE_EQ(x, 0.);  // avoid compiler warning
}

TEST(Dictionary, DeserializeWithSameKeys) {
  std::vector<char> buffer;
  mpack::Writer writer(buffer);
  writer.start_map(2);
  writer.write("compact");
  writer.write(true);
  writer.write("schema");
  writer.write(0u);
  writer.finish_map();
  size_t size = writer.finish();
  ASSERT_GT(buffer.size(), 0);

  Dictionary dict;
  dict("compact") = false;
  dict("schema") = 12u;
  dict.deserialize(buffer.data(), size);

  ASSERT_EQ(dict("compact").as<bool>(), true);
  ASSERT_EQ(dict("schema").as<unsigned>(), 0u);
}

TEST(Dictionary, DeserializationAddsKeyValues) {
  std::vector<char> buffer;
  mpack::Writer writer(buffer);
  writer.start_map(2);
  writer.write("compact");
  writer.write(true);
  writer.write("schema");
  writer.write(0u);
  writer.finish_map();
  size_t size = writer.finish();
  ASSERT_GT(buffer.size(), 0);

  Dictionary incomplete;
  incomplete("compact") = false;
  incomplete("zebra") = 42;
  incomplete.deserialize(buffer.data(), size);

  ASSERT_EQ(incomplete("compact").as<bool>(), true);
  ASSERT_TRUE(incomplete.has("schema"));
  ASSERT_EQ(incomplete("schema").as<unsigned>(), 0u);
  ASSERT_THROW(incomplete("schema")("use_uint_as_a_map") = "foo",
               std::runtime_error);
}

TEST(Dictionary, DeserializeSignedInt) {
  std::vector<char> buffer;
  mpack::Writer writer(buffer);
  writer.start_map(1);
  writer.write("foo");
  writer.write(static_cast<int>(12));
  writer.finish_map();
  size_t size = writer.finish();
  ASSERT_GT(buffer.size(), 0);

  Dictionary dict;
  dict.insert<int>("foo", 0);
  ASSERT_EQ(dict("foo").as<int>(), 0);
  ASSERT_NO_THROW(dict.deserialize(buffer.data(), size));
  ASSERT_EQ(dict("foo").as<int>(), 12);
}

TEST(Dictionary, ListKeys) {
  Dictionary dict;
  dict.insert<int>("foo", 99.1);
  dict.insert<int>("bar", true);
  auto keys = dict.keys();
  ASSERT_TRUE((keys[0] == "foo" && keys[1] == "bar") ||
              (keys[0] == "bar" && keys[1] == "foo"));
}

TEST(Dictionary, TypeErrorOnNonValueCast) {
  Dictionary dict;
  dict("foo")("bar") = 1.;
  const Dictionary &const_dict = dict;
  ASSERT_THROW(dict("foo").as<int>(), TypeError);
  ASSERT_THROW(const_dict("foo").as<int>(), TypeError);
}

TEST(Dictionary, GetChecksBothExistenceAndType) {
  Dictionary dict;
  dict("foo")("bar") = 1.;
  ASSERT_THROW(dict.get<int>("blah"), KeyError);
  ASSERT_THROW(dict.get<int>("foo"), TypeError);
}

TEST(Dictionary, GetWithDefaultValueChecksType) {
  Dictionary dict;
  dict("foo") = std::string("blah");
  dict("bar")("num") = 12;
  ASSERT_THROW(dict.get<int>("foo", 12), TypeError);
  ASSERT_THROW(dict.get<int>("bar", 42), TypeError);
}

TEST(Dictionary, DeserializeFromEmptyBuffer) {
  std::vector<char> buffer;
  mpack::Writer writer(buffer);
  writer.start_map(0);
  writer.finish_map();
  size_t size = writer.finish();

  Dictionary dict;
  ASSERT_NO_THROW(dict.deserialize(buffer.data(), size));  // dict empty
  dict("foo") = std::string("bar");
  ASSERT_NO_THROW(dict.deserialize(buffer.data(), size));  // dict non-empty
}

TEST(Dictionary, WriteAndRead) {
  Dictionary dict;
  dict("foo") = std::string("blah");
  dict("bar")("num") = 12u;

  char tmp_file[] = "/tmp/dictXXXXXX";
  int fd = ::mkstemp(tmp_file);
  dict.write(tmp_file);

  Dictionary check;
  check.read(tmp_file);
  ::close(fd);
  ::unlink(tmp_file);

  ASSERT_EQ(dict.get<std::string>("foo"), check.get<std::string>("foo"));
  ASSERT_EQ(dict("bar").get<unsigned>("num"),
            check("bar").get<unsigned>("num"));
}

TEST(Dictionary, WriteVectorOfStrings) {
  Dictionary dict;
  dict("foo").insert<std::vector<std::string>>("bar", 22, "left_wheel");

  char tmp_file[] = "/tmp/dictXXXXXX";
  int fd = ::mkstemp(tmp_file);
  dict.write(tmp_file);

  Dictionary check;
  ASSERT_THROW(check.read(tmp_file), TypeError);  // not implemented
  ::close(fd);
  ::unlink(tmp_file);

  // if we implement a reader in the future, we can test:
  // ASSERT_EQ(dict("foo").get<std::vector<double>>("bar").size(),
  // check("foo").get<std::vector<double>>("bar").size());
}

#if __APPLE__
/*
 * The following test is disabled on macOS GitHub Actions runners where size_t
 * and uint64_t are two distinct types and palimpsest has no serialization
 * function for size_t. (Defining one would make the overload an error on
 * systems where size_t and uint64_t are the same type.)
 *
 * In general, it is recommended not to serialize size_t in palimpsest
 * dictionaries. They would be deserialized as unsigned integers anyway, as
 * MessagePack only has two types for integers (signed and unsigned).
 *
 * See also https://github.com/stephane-caron/palimpsest/pull/6 and
 * https://github.com/stephane-caron/vulp/issues/96
 */
#else
/* This test checks what happens when serializing size_t integers on platforms
 * where size_t is a generic uintXX_t. */
TEST(Dictionary, SerializeSizeT) {
  Dictionary serialized, deserialized;
  std::vector<char> buffer;

  serialized("logger")("last_size") = static_cast<size_t>(42);
  size_t buffer_size = serialized.serialize(buffer);
  deserialized.deserialize(buffer.data(), buffer_size);

  size_t before = serialized("logger")("last_size").as<size_t>();
  unsigned after = deserialized("logger")("last_size");
  ASSERT_EQ(static_cast<unsigned>(before), after);
}
#endif

TEST(Dictionary, SerializeVectorDouble) {
  Dictionary source;
  source.insert<std::vector<double>>(
      "vector_double", std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0});

  // Serialize to buffer
  std::vector<char> buffer;
  size_t size = source.serialize(buffer);
  ASSERT_GT(buffer.size(), 0);
  ASSERT_LT(size, buffer.size());  // size is usually < MPACK_BUFFER_SIZE

  // Deserialize and check that we recover all serialized values. Note that
  // std::vector<double> is deserialized as Eigen::VectorXd We allow
  // serialization for convenience, but expect Eigen vectors to be used
  // primarily.
  Dictionary deserialized;
  deserialized.deserialize(buffer.data(), size);
  const auto &deserialized_vector_xd =
      deserialized("vector_double").as<Eigen::VectorXd>();
  for (unsigned i = 0; i < 5; ++i) {
    ASSERT_DOUBLE_EQ(deserialized_vector_xd(i), static_cast<double>(i + 1));
  }
}

TEST(Dictionary, SerializeVectorXd) {
  Dictionary source;
  Eigen::VectorXd vec(5);
  vec << 1.0, 2.0, 3.0, 4.0, 5.0;
  source.insert<Eigen::VectorXd>("vector_xd", vec);

  // Serialize to buffer
  std::vector<char> buffer;
  size_t size = source.serialize(buffer);
  ASSERT_GT(buffer.size(), 0);
  ASSERT_LT(size, buffer.size());  // size is usually < MPACK_BUFFER_SIZE

  // Deserialize and check that we recover all serialized values
  Dictionary deserialized;
  deserialized.deserialize(buffer.data(), size);
  const auto &deserialized_vector_xd =
      deserialized("vector_xd").as<Eigen::VectorXd>();
  for (unsigned i = 0; i < 5; ++i) {
    ASSERT_DOUBLE_EQ(deserialized_vector_xd(i), static_cast<double>(i + 1));
  }
}

TEST(Dictionary, SerializeVectorOfVectorXd) {
  Eigen::VectorXd vec(5);
  vec << 1.0, 2.0, 3.0, 4.0, 5.0;

  Dictionary source;
  source.insert<std::vector<Eigen::VectorXd>>(
      "vec_vec", std::vector<Eigen::VectorXd>{vec, vec, vec});
  const auto &serialized_vec_vec =
      source("vec_vec").as<std::vector<Eigen::VectorXd>>();
  ASSERT_EQ(serialized_vec_vec.size(), 3);

  // Serialize to buffer
  std::vector<char> buffer;
  size_t size = source.serialize(buffer);
  ASSERT_GT(buffer.size(), 0);
  ASSERT_LT(size, buffer.size());  // size is usually < MPACK_BUFFER_SIZE

  // Deserialize and check that we recover all serialized values
  Dictionary deserialized;
  deserialized.deserialize(buffer.data(), size);
  const auto &deserialized_vec_vec =
      deserialized("vec_vec").as<std::vector<Eigen::VectorXd>>();
  ASSERT_EQ(deserialized_vec_vec.size(), 3);
  for (unsigned i = 0; i < 3; ++i) {
    for (unsigned j = 0; j < 5; ++j) {
      ASSERT_DOUBLE_EQ(deserialized_vec_vec[i](j), static_cast<double>(j + 1));
    }
  }
}

TEST(Dictionary, OneDimensionalVector) {
  Dictionary source;
  Eigen::VectorXd vec(1);
  vec << 0.42;
  source.insert<Eigen::VectorXd>("foo", vec);

  // Serialize to buffer
  std::vector<char> buffer;
  size_t size = source.serialize(buffer);
  ASSERT_GT(buffer.size(), 0);
  ASSERT_LT(size, buffer.size());

  // Deserialize and check that we recover the serialized vector
  Dictionary deserialized;
  deserialized.deserialize(buffer.data(), size);
  const auto &deserialized_vector = deserialized("foo").as<Eigen::VectorXd>();
  ASSERT_EQ(deserialized_vector.size(), 1);
  ASSERT_DOUBLE_EQ(deserialized_vector(0), 0.42);

  // We should not be able to deserialize it as a double
  ASSERT_THROW(deserialized.get<double>("foo"),
               palimpsest::exceptions::TypeError);

  Dictionary bar;
  bar("foo") = 1.2;  // different from above
  ASSERT_THROW(bar.deserialize(buffer.data(), size),
               palimpsest::exceptions::TypeError);
}

TEST(Dictionary, DeepCopyFromDictionary) {
  Dictionary dict1, dict2;

  // Set up first dictionary
  dict1("name") = std::string("first");
  dict1("value") = 42;

  // Set up second dictionary
  dict2("name") = std::string("second");
  dict2("extra") = 3.14;
  dict2("flag") = true;

  // Test the deepcopy function creates a new dictionary with copied values
  Dictionary dict_copy = Dictionary::deepcopy(dict2);

  // Check that dict_copy contains values from dict2
  ASSERT_EQ(dict_copy.get<std::string>("name"), "second");
  ASSERT_DOUBLE_EQ(dict_copy.get<double>("extra"), 3.14);
  ASSERT_EQ(dict_copy.get<bool>("flag"), true);

  // Verify dict1 is unchanged (deepcopy doesn't modify anything)
  ASSERT_EQ(dict1.get<std::string>("name"), "first");
  ASSERT_EQ(dict1.get<int>("value"), 42);
}

TEST(Dictionary, PopValue) {
  Dictionary dict;
  dict.insert<int>("foo", 42);
  dict.insert<std::string>("bar", "hello");
  dict.insert<double>("pi", 3.14159);

  // Test pop without default - returns value and removes key
  ASSERT_EQ(dict.pop<int>("foo"), 42);
  ASSERT_FALSE(dict.has("foo"));

  // Test pop for string
  ASSERT_EQ(dict.pop<std::string>("bar"), "hello");
  ASSERT_FALSE(dict.has("bar"));

  // Test pop for double
  ASSERT_DOUBLE_EQ(dict.pop<double>("pi"), 3.14159);
  ASSERT_FALSE(dict.has("pi"));

  // Dictionary should now be empty
  ASSERT_TRUE(dict.is_empty());
}

TEST(Dictionary, PopWithDefault) {
  Dictionary dict;
  dict.insert<int>("existing", 42);

  // Test pop with default for existing key - should return value and remove key
  ASSERT_EQ(dict.pop<int>("existing", 999), 42);
  ASSERT_FALSE(dict.has("existing"));

  // Test pop with default for non-existing key - should return default and not
  // throw
  ASSERT_EQ(dict.pop<int>("non_existing", 999), 999);
  ASSERT_FALSE(dict.has("non_existing"));

  // Test pop with default for different types
  ASSERT_DOUBLE_EQ(dict.pop<double>("missing_double", 2.718), 2.718);
  ASSERT_EQ(dict.pop<std::string>("missing_string", "default"), "default");
  ASSERT_EQ(dict.pop<bool>("missing_bool", true), true);
}

TEST(Dictionary, PopNonExistentKey) {
  Dictionary dict;
  dict.insert<int>("foo", 42);

  // Test pop without default on non-existent key should throw KeyError
  ASSERT_THROW(dict.pop<int>("nonexistent"), KeyError);

  // Original key should still exist
  ASSERT_TRUE(dict.has("foo"));
  ASSERT_EQ(dict.get<int>("foo"), 42);
}

TEST(Dictionary, PopWrongType) {
  Dictionary dict;
  dict.insert<int>("number", 42);

  // Test pop with wrong type should throw TypeError
  ASSERT_THROW(dict.pop<std::string>("number"), TypeError);
  ASSERT_THROW(dict.pop<double>("number"), TypeError);

  // Key should still exist after failed pop
  ASSERT_TRUE(dict.has("number"));
  ASSERT_EQ(dict.get<int>("number"), 42);
}

TEST(Dictionary, PopWrongTypeWithDefault) {
  Dictionary dict;
  dict.insert<int>("number", 42);

  // Test pop with wrong type and default should throw TypeError (not return
  // default)
  ASSERT_THROW(dict.pop<std::string>("number", "default"), TypeError);
  ASSERT_THROW(dict.pop<double>("number", 3.14), TypeError);

  // Key should still exist after failed pop
  ASSERT_TRUE(dict.has("number"));
  ASSERT_EQ(dict.get<int>("number"), 42);
}

TEST(Dictionary, PopFromDictionary) {
  Dictionary dict;
  dict("nested").insert<int>("value", 123);

  // Test pop from nested dictionary (should throw TypeError)
  ASSERT_THROW(dict.pop<int>("nested"), TypeError);
  ASSERT_THROW(dict.pop<int>("nested", 999), TypeError);

  // Nested dictionary should still exist
  ASSERT_TRUE(dict.has("nested"));
  ASSERT_TRUE(dict("nested").has("value"));
  ASSERT_EQ(dict("nested").get<int>("value"), 123);
}

TEST(Dictionary, PopEigenTypes) {
  Dictionary dict;
  dict.insert<Eigen::Vector3d>("position", 1.0, 2.0, 3.0);
  dict.insert<Eigen::Quaterniond>("orientation", 1.0, 0.0, 0.0, 0.0);

  // Test pop with Eigen types
  Eigen::Vector3d pos = dict.pop<Eigen::Vector3d>("position");
  ASSERT_DOUBLE_EQ(pos.x(), 1.0);
  ASSERT_DOUBLE_EQ(pos.y(), 2.0);
  ASSERT_DOUBLE_EQ(pos.z(), 3.0);
  ASSERT_FALSE(dict.has("position"));

  // Test pop with default for Eigen types
  Eigen::Vector3d default_pos(0.0, 0.0, 0.0);
  Eigen::Vector3d result =
      dict.pop<Eigen::Vector3d>("missing_pos", default_pos);
  ASSERT_TRUE(result.isApprox(default_pos));

  Eigen::Quaterniond quat = dict.pop<Eigen::Quaterniond>("orientation");
  ASSERT_DOUBLE_EQ(quat.w(), 1.0);
  ASSERT_FALSE(dict.has("orientation"));
}

}  // namespace palimpsest
