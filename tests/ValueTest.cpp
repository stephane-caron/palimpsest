// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Inria

#include <gtest/gtest.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "palimpsest/Value.h"
#include "palimpsest/exceptions/TypeError.h"
#include "palimpsest/mpack/Writer.h"

namespace palimpsest {

using exceptions::TypeError;

/*! Custom type for testing Value class functionality.
 */
struct TestStruct {
  int x;
  double y;
  std::string z;

  TestStruct() : x(0), y(0.0), z("") {}
  TestStruct(int x_, double y_, const std::string& z_) : x(x_), y(y_), z(z_) {}

  bool operator==(const TestStruct& other) const {
    return x == other.x && y == other.y && z == other.z;
  }
};

}  // namespace palimpsest

// Specialize json::write for our test struct
namespace palimpsest::json {

template <>
void write(std::ostream& stream, const TestStruct& s) {
  stream << "{\"x\": " << s.x << ", \"y\": " << s.y << ", \"z\": \"" << s.z
         << "\"}";
}

}  // namespace palimpsest::json

// Specialize mpack::write for our test struct
namespace palimpsest::mpack {

template <>
void write(mpack_writer_t* writer, const TestStruct& s) {
  mpack_start_map(writer, 3);
  mpack_write_cstr(writer, "x");
  mpack_write_int(writer, s.x);
  mpack_write_cstr(writer, "y");
  mpack_write_double(writer, s.y);
  mpack_write_cstr(writer, "z");
  mpack_write_cstr(writer, s.z.c_str());
  mpack_finish_map(writer);
}

template <>
void read(mpack_node_t node, TestStruct& s) {
  if (mpack_node_type(node) != mpack_type_map) {
    mpack_node_flag_error(node, mpack_error_type);
    return;
  }

  mpack_node_t x_node = mpack_node_map_cstr(node, "x");
  mpack_node_t y_node = mpack_node_map_cstr(node, "y");
  mpack_node_t z_node = mpack_node_map_cstr(node, "z");

  s.x = mpack_node_int(x_node);
  s.y = mpack_node_double(y_node);
  s.z = std::string(mpack_node_str(z_node), mpack_node_strlen(z_node));
}

}  // namespace palimpsest::mpack

namespace palimpsest {

class ValueTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Test values for different types
    test_bool_ = true;
    test_int_ = 42;
    test_double_ = 3.14159;
    test_string_ = "hello world";
    test_vector_ = {1.0, 2.0, 3.0, 4.0, 5.0};
    test_struct_ = TestStruct{100, 2.718, "test"};

    // Eigen types
    test_vector2d_ = Eigen::Vector2d(1.0, 2.0);
    test_vector3d_ = Eigen::Vector3d(1.0, 2.0, 3.0);
    test_vectorxd_.resize(5);
    test_vectorxd_ << 1.0, 2.0, 3.0, 4.0, 5.0;
    test_quaterniond_ = Eigen::Quaterniond(1.0, 0.0, 0.0, 0.0);
    test_matrix3d_ = Eigen::Matrix3d::Identity();
  }

  bool test_bool_;
  int test_int_;
  double test_double_;
  std::string test_string_;
  std::vector<double> test_vector_;
  TestStruct test_struct_;

  Eigen::Vector2d test_vector2d_;
  Eigen::Vector3d test_vector3d_;
  Eigen::VectorXd test_vectorxd_;
  Eigen::Quaterniond test_quaterniond_;
  Eigen::Matrix3d test_matrix3d_;
};

TEST_F(ValueTest, DefaultConstruction) {
  Value value;
  EXPECT_EQ(value.buffer, nullptr);
}

TEST_F(ValueTest, MoveConstruction) {
  Value value1;
  value1.allocate<int>();
  new (value1.buffer.get()) int{42};
  value1.setup<int>();

  Value value2(std::move(value1));
  EXPECT_NE(value2.buffer, nullptr);
  EXPECT_EQ(value1.buffer, nullptr);  // Moved from
  EXPECT_EQ(value2.get_reference<int>(), 42);
}

TEST_F(ValueTest, MoveAssignment) {
  Value value1;
  value1.allocate<int>();
  new (value1.buffer.get()) int{42};
  value1.setup<int>();

  Value value2;
  value2 = std::move(value1);

  EXPECT_NE(value2.buffer, nullptr);
  EXPECT_EQ(value1.buffer, nullptr);  // Moved from
  EXPECT_EQ(value2.get_reference<int>(), 42);
}

TEST_F(ValueTest, BasicTypeStorage) {
  // Test bool
  {
    Value value;
    value.allocate<bool>();
    new (value.buffer.get()) bool{test_bool_};
    value.setup<bool>();
    EXPECT_EQ(value.get_reference<bool>(), test_bool_);
  }

  // Test int
  {
    Value value;
    value.allocate<int>();
    new (value.buffer.get()) int{test_int_};
    value.setup<int>();
    EXPECT_EQ(value.get_reference<int>(), test_int_);
  }

  // Test double
  {
    Value value;
    value.allocate<double>();
    new (value.buffer.get()) double{test_double_};
    value.setup<double>();
    EXPECT_DOUBLE_EQ(value.get_reference<double>(), test_double_);
  }

  // Test string
  {
    Value value;
    value.allocate<std::string>();
    new (value.buffer.get()) std::string(test_string_);
    value.setup<std::string>();
    EXPECT_EQ(value.get_reference<std::string>(), test_string_);
  }
}

TEST_F(ValueTest, ComplexTypeStorage) {
  // Test vector<double>
  {
    Value value;
    value.allocate<std::vector<double>>();
    new (value.buffer.get()) std::vector<double>(test_vector_);
    value.setup<std::vector<double>>();

    const auto& stored_vector = value.get_reference<std::vector<double>>();
    EXPECT_EQ(stored_vector.size(), test_vector_.size());
    for (size_t i = 0; i < test_vector_.size(); ++i) {
      EXPECT_DOUBLE_EQ(stored_vector[i], test_vector_[i]);
    }
  }

  // Test custom struct
  {
    Value value;
    value.allocate<TestStruct>();
    new (value.buffer.get()) TestStruct(test_struct_);
    value.setup<TestStruct>();

    const auto& stored_struct = value.get_reference<TestStruct>();
    EXPECT_EQ(stored_struct, test_struct_);
  }
}

TEST_F(ValueTest, EigenTypeStorage) {
  // Test Eigen::Vector2d
  {
    Value value;
    value.allocate<Eigen::Vector2d>();
    new (value.buffer.get()) Eigen::Vector2d(test_vector2d_);
    value.setup<Eigen::Vector2d>();

    const auto& stored = value.get_reference<Eigen::Vector2d>();
    EXPECT_DOUBLE_EQ(stored(0), test_vector2d_(0));
    EXPECT_DOUBLE_EQ(stored(1), test_vector2d_(1));
  }

  // Test Eigen::Vector3d
  {
    Value value;
    value.allocate<Eigen::Vector3d>();
    new (value.buffer.get()) Eigen::Vector3d(test_vector3d_);
    value.setup<Eigen::Vector3d>();

    const auto& stored = value.get_reference<Eigen::Vector3d>();
    EXPECT_TRUE(stored.isApprox(test_vector3d_));
  }

  // Test Eigen::VectorXd
  {
    Value value;
    value.allocate<Eigen::VectorXd>();
    new (value.buffer.get()) Eigen::VectorXd(test_vectorxd_);
    value.setup<Eigen::VectorXd>();

    const auto& stored = value.get_reference<Eigen::VectorXd>();
    EXPECT_TRUE(stored.isApprox(test_vectorxd_));
  }

  // Test Eigen::Quaterniond
  {
    Value value;
    value.allocate<Eigen::Quaterniond>();
    new (value.buffer.get()) Eigen::Quaterniond(test_quaterniond_);
    value.setup<Eigen::Quaterniond>();

    const auto& stored = value.get_reference<Eigen::Quaterniond>();
    EXPECT_TRUE(stored.coeffs().isApprox(test_quaterniond_.coeffs()));
  }

  // Test Eigen::Matrix3d
  {
    Value value;
    value.allocate<Eigen::Matrix3d>();
    new (value.buffer.get()) Eigen::Matrix3d(test_matrix3d_);
    value.setup<Eigen::Matrix3d>();

    const auto& stored = value.get_reference<Eigen::Matrix3d>();
    EXPECT_TRUE(stored.isApprox(test_matrix3d_));
  }
}

TEST_F(ValueTest, TypeMismatchThrowsException) {
  Value value;
  value.allocate<int>();
  new (value.buffer.get()) int{42};
  value.setup<int>();

  // These should throw TypeError
  EXPECT_THROW(value.get_reference<double>(), TypeError);
  EXPECT_THROW(value.get_reference<std::string>(), TypeError);
  EXPECT_THROW(value.get_reference<bool>(), TypeError);
  EXPECT_THROW(value.get_reference<std::vector<double>>(), TypeError);

  // This should work
  EXPECT_NO_THROW(value.get_reference<int>());
}

TEST_F(ValueTest, TypeNameFunction) {
  Value int_value;
  int_value.allocate<int>();
  new (int_value.buffer.get()) int{42};
  int_value.setup<int>();

  EXPECT_NE(int_value.type_name(), nullptr);
  // The exact name might be implementation-dependent, but it should be set
  EXPECT_NE(std::string(int_value.type_name()), "");
}

TEST_F(ValueTest, SerializationAndDeserialization) {
  // Test with a simple integer
  Value original_value;
  original_value.allocate<int>();
  new (original_value.buffer.get()) int{42};
  original_value.setup<int>();

  // Serialize to MessagePack
  std::vector<char> buffer;
  mpack::Writer writer(buffer);
  original_value.serialize(writer);
  writer.finish();

  // Create a tree from the buffer
  mpack_tree_t tree;
  mpack_tree_init_data(&tree, buffer.data(), buffer.size());
  mpack_tree_parse(&tree);
  mpack_node_t root = mpack_tree_root(&tree);

  // Deserialize to a new value
  Value deserialized_value;
  deserialized_value.allocate<int>();
  new (deserialized_value.buffer.get()) int{
      0};  // Initialize with different value
  deserialized_value.setup<int>();
  deserialized_value.deserialize(root);

  // Check that the deserialized value matches
  EXPECT_EQ(deserialized_value.get_reference<int>(), 42);

  mpack_tree_destroy(&tree);
}

TEST_F(ValueTest, SerializationWithCustomStruct) {
  // Test with our custom struct
  Value original_value;
  original_value.allocate<TestStruct>();
  new (original_value.buffer.get()) TestStruct(test_struct_);
  original_value.setup<TestStruct>();

  // Serialize to MessagePack
  std::vector<char> buffer;
  mpack::Writer writer(buffer);
  original_value.serialize(writer);
  writer.finish();

  // Create a tree from the buffer
  mpack_tree_t tree;
  mpack_tree_init_data(&tree, buffer.data(), buffer.size());
  mpack_tree_parse(&tree);
  mpack_node_t root = mpack_tree_root(&tree);

  // Deserialize to a new value
  Value deserialized_value;
  deserialized_value.allocate<TestStruct>();
  new (deserialized_value.buffer.get()) TestStruct();
  deserialized_value.setup<TestStruct>();
  deserialized_value.deserialize(root);

  // Check that the deserialized value matches
  const auto& deserialized_struct =
      deserialized_value.get_reference<TestStruct>();
  EXPECT_EQ(deserialized_struct, test_struct_);

  mpack_tree_destroy(&tree);
}

TEST_F(ValueTest, PrintFunctionality) {
  // Test printing an integer value
  Value int_value;
  int_value.allocate<int>();
  new (int_value.buffer.get()) int{42};
  int_value.setup<int>();

  std::ostringstream stream;
  int_value.print(stream);
  EXPECT_EQ(stream.str(), "42");

  // Test printing a string value
  Value string_value;
  string_value.allocate<std::string>();
  new (string_value.buffer.get()) std::string("hello");
  string_value.setup<std::string>();

  std::ostringstream string_stream;
  string_value.print(string_stream);
  EXPECT_EQ(string_stream.str(), "\"hello\"");

  // Test printing our custom struct
  Value struct_value;
  struct_value.allocate<TestStruct>();
  new (struct_value.buffer.get()) TestStruct(100, 2.718, "test");
  struct_value.setup<TestStruct>();

  std::ostringstream struct_stream;
  struct_value.print(struct_stream);
  EXPECT_EQ(struct_stream.str(), "{\"x\": 100, \"y\": 2.718, \"z\": \"test\"}");
}

TEST_F(ValueTest, ProperDestruction) {
  // This test ensures that the destructor properly calls the destroy function
  // We can't directly test destruction, but we can test that objects are
  // properly constructed and accessed, which implies proper setup

  {
    Value value;
    value.allocate<std::vector<double>>();
    new (value.buffer.get()) std::vector<double>(1000, 42.0);
    value.setup<std::vector<double>>();

    const auto& vec = value.get_reference<std::vector<double>>();
    EXPECT_EQ(vec.size(), 1000);
    EXPECT_DOUBLE_EQ(vec[0], 42.0);
    EXPECT_DOUBLE_EQ(vec[999], 42.0);

    // Value will be destroyed when going out of scope
  }

  // If there were memory leaks or improper destruction,
  // tools like AddressSanitizer or Valgrind would catch them
}

TEST_F(ValueTest, ModifyStoredValue) {
  // Test that we can modify the stored value through the reference
  Value value;
  value.allocate<std::vector<double>>();
  new (value.buffer.get()) std::vector<double>();
  value.setup<std::vector<double>>();

  auto& vec = value.get_reference<std::vector<double>>();
  vec.push_back(1.0);
  vec.push_back(2.0);
  vec.push_back(3.0);

  // Verify the modifications
  const auto& const_vec = value.get_reference<std::vector<double>>();
  EXPECT_EQ(const_vec.size(), 3);
  EXPECT_DOUBLE_EQ(const_vec[0], 1.0);
  EXPECT_DOUBLE_EQ(const_vec[1], 2.0);
  EXPECT_DOUBLE_EQ(const_vec[2], 3.0);
}

TEST_F(ValueTest, CopyConstruction) {
  Value original;
  original.allocate<int>();
  new (original.buffer.get()) int{42};
  original.setup<int>();

  Value copy(original);

  // Both should have valid buffers
  EXPECT_NE(original.buffer, nullptr);
  EXPECT_NE(copy.buffer, nullptr);

  // Buffers should be different (deep copy)
  EXPECT_NE(original.buffer.get(), copy.buffer.get());

  // Values should be the same
  EXPECT_EQ(original.get_reference<int>(), 42);
  EXPECT_EQ(copy.get_reference<int>(), 42);

  // Modify original and verify copy is unchanged
  original.get_reference<int>() = 100;
  EXPECT_EQ(original.get_reference<int>(), 100);
  EXPECT_EQ(copy.get_reference<int>(), 42);
}

TEST_F(ValueTest, CopyAssignment) {
  Value original;
  original.allocate<int>();
  new (original.buffer.get()) int{42};
  original.setup<int>();

  Value copy;
  copy = original;

  // Both should have valid buffers
  EXPECT_NE(original.buffer, nullptr);
  EXPECT_NE(copy.buffer, nullptr);

  // Buffers should be different (deep copy)
  EXPECT_NE(original.buffer.get(), copy.buffer.get());

  // Values should be the same
  EXPECT_EQ(original.get_reference<int>(), 42);
  EXPECT_EQ(copy.get_reference<int>(), 42);

  // Modify original and verify copy is unchanged
  original.get_reference<int>() = 100;
  EXPECT_EQ(original.get_reference<int>(), 100);
  EXPECT_EQ(copy.get_reference<int>(), 42);
}

}  // namespace palimpsest
