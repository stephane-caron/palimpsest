// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Inria

#include <gtest/gtest.h>

#include "palimpsest/Dictionary.h"

namespace palimpsest {

class DictionaryUpdateTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(DictionaryUpdateTest, UpdateEmptyWithEmpty) {
  Dictionary dict;
  Dictionary other;
  dict.update(other);
  EXPECT_TRUE(dict.is_empty());
}

TEST_F(DictionaryUpdateTest, UpdateEmptyWithValue) {
  Dictionary dict;
  Dictionary other;
  other.insert<int>("key", 42);

  dict.update(other);
  EXPECT_TRUE(dict.is_map());
  // value might be deserialized as unsigned due to mpack type handling
  EXPECT_EQ(dict.get<unsigned>("key"), 42u);
}

TEST_F(DictionaryUpdateTest, UpdateValueWithEmpty) {
  Dictionary dict;
  dict.insert<int>("key", 42);

  Dictionary other;  // empty

  dict.update(other);
  EXPECT_TRUE(dict.is_map());
  EXPECT_EQ(dict.get<int>("key"), 42);
}

TEST_F(DictionaryUpdateTest, UpdateMapWithMapNoValues) {
  Dictionary dict;
  dict("section1")("key1") = Dictionary();  // nested empty map
  dict("section2")("key2") = Dictionary();  // nested empty map

  Dictionary other;
  other("section1")("key3") = Dictionary();  // add to existing section
  other("section3")("key4") = Dictionary();  // new section

  dict.update(other);
  EXPECT_TRUE(dict.is_map());
  EXPECT_TRUE(dict("section1").is_map());
  EXPECT_TRUE(dict("section1")("key1").is_empty());
  EXPECT_TRUE(dict("section1")("key3").is_empty());
  EXPECT_TRUE(dict("section2").is_map());
  EXPECT_TRUE(dict("section3").is_map());
  EXPECT_TRUE(dict("section3")("key4").is_empty());
}

TEST_F(DictionaryUpdateTest, UpdateWithSingleValue) {
  Dictionary dict;
  dict.insert<int>("key1", 10);
  dict.insert<std::string>("key2", "hello");

  Dictionary other;
  other = 42.0;  // Make other a single double value

  // This should replace the entire dictionary with the single value
  dict.update(other);
  EXPECT_TRUE(dict.is_value());
  EXPECT_EQ(dict.as<double>(), 42.0);
}

TEST_F(DictionaryUpdateTest, UpdateSingleValueWithMap) {
  Dictionary dict;
  dict = 100;  // Make dict a single int value

  Dictionary other;
  other("key1")("subkey1") = Dictionary();  // nested maps only

  // This should work - converting value to map structure
  dict.update(other);
  EXPECT_TRUE(dict.is_map());
  EXPECT_TRUE(dict("key1").is_map());
  EXPECT_TRUE(dict("key1")("subkey1").is_empty());
}

TEST_F(DictionaryUpdateTest, UpdateOverwriteExistingKeys) {
  Dictionary dict;
  dict("key1")("subkey") = Dictionary();
  dict("key2")("subkey") = Dictionary();

  Dictionary other;
  other("key1")("newkey") = Dictionary();  // Add to existing
  other("key3")("subkey") = Dictionary();  // Add new

  dict.update(other);
  EXPECT_TRUE(dict("key1")("subkey").is_empty());  // Preserved
  EXPECT_TRUE(dict("key1")("newkey").is_empty());  // Added
  EXPECT_TRUE(dict("key2")("subkey").is_empty());  // Preserved
  EXPECT_TRUE(dict("key3")("subkey").is_empty());  // Added
}

TEST_F(DictionaryUpdateTest, UpdateNestedMapsOnly) {
  Dictionary dict;
  dict("nested")("deep")("level") = Dictionary();
  dict("nested")("surface") = Dictionary();
  dict("top") = Dictionary();

  Dictionary other;
  other("nested")("deep")("newlevel") = Dictionary();  // Add to deep
  other("nested")("newsurface") = Dictionary();        // Add to nested
  other("newtop") = Dictionary();                      // Add at top level

  dict.update(other);

  // Check preserved structure
  EXPECT_TRUE(dict("top").is_empty());
  EXPECT_TRUE(dict("nested")("surface").is_empty());
  EXPECT_TRUE(dict("nested")("deep")("level").is_empty());

  // Check added structure
  EXPECT_TRUE(dict("newtop").is_empty());
  EXPECT_TRUE(dict("nested")("newsurface").is_empty());
  EXPECT_TRUE(dict("nested")("deep")("newlevel").is_empty());
}

TEST_F(DictionaryUpdateTest, UpdateReplaceNestedWithValue) {
  Dictionary dict;
  dict("nested")("deep") = Dictionary();
  dict("nested")("surface") = Dictionary();

  Dictionary other;
  other("nested").insert<double>("value", 42.0);  // Replace with value

  dict.update(other);

  EXPECT_TRUE(dict("nested").is_map());
  EXPECT_EQ(dict("nested").get<double>("value"), 42.0);
}

TEST_F(DictionaryUpdateTest, UpdateReplaceValueWithNested) {
  Dictionary dict;
  dict("key") = 100;  // Single value

  Dictionary other;
  other("key")("subkey")("deep") = Dictionary();

  // This should work - converting value to nested map structure
  dict.update(other);

  EXPECT_TRUE(dict("key").is_map());
  EXPECT_TRUE(dict("key")("subkey").is_map());
  EXPECT_TRUE(dict("key")("subkey")("deep").is_empty());
}

TEST_F(DictionaryUpdateTest, UpdateComplexMapStructure) {
  Dictionary dict;
  dict("level1")("level2a")("deep") = Dictionary();
  dict("level1")("level2b") = Dictionary();
  dict("level1")("surface") = Dictionary();
  dict("root") = Dictionary();

  Dictionary other;
  other("level1")("level2a")("newdeep") = Dictionary();  // Add to existing deep
  other("level1")("level2c")("branch") = Dictionary();   // Add new deep branch
  other("newroot") = Dictionary();                       // Add new root

  dict.update(other);

  // Check preserved values
  EXPECT_TRUE(dict("root").is_empty());
  EXPECT_TRUE(dict("level1")("surface").is_empty());
  EXPECT_TRUE(dict("level1")("level2b").is_empty());
  EXPECT_TRUE(dict("level1")("level2a")("deep").is_empty());

  // Check added values
  EXPECT_TRUE(dict("newroot").is_empty());
  EXPECT_TRUE(dict("level1")("level2a")("newdeep").is_empty());
  EXPECT_TRUE(dict("level1")("level2c")("branch").is_empty());
}

TEST_F(DictionaryUpdateTest, UpdateWithEigenTypes) {
  Dictionary dict;
  dict("section") = Dictionary();

  Dictionary other;
  other.insert<Eigen::Vector3d>("vector", Eigen::Vector3d(1.0, 2.0, 3.0));
  other.insert<Eigen::Vector2d>("vector2d", Eigen::Vector2d(4.0, 5.0));

  dict.update(other);
  EXPECT_TRUE(dict.is_map());
  EXPECT_EQ(dict.get<Eigen::Vector3d>("vector"),
            Eigen::Vector3d(1.0, 2.0, 3.0));
  EXPECT_EQ(dict.get<Eigen::Vector2d>("vector2d"), Eigen::Vector2d(4.0, 5.0));
  EXPECT_TRUE(dict("section").is_empty());
}

}  // namespace palimpsest
