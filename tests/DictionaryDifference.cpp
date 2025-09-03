// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>
#include <palimpsest/Dictionary.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <vector>

namespace palimpsest {

TEST(DictionaryDifference, EmptyDictionaries) {
  Dictionary empty1, empty2;
  Dictionary diff = empty1.difference(empty2);
  ASSERT_TRUE(diff.is_empty());
}

TEST(DictionaryDifference, EmptyVsNonEmpty) {
  Dictionary empty, non_empty;
  non_empty("key") = 42u;

  Dictionary diff = empty.difference(non_empty);
  ASSERT_TRUE(diff.is_empty());

  diff = non_empty.difference(empty);
  ASSERT_FALSE(diff.is_empty());
  ASSERT_EQ(diff.get<unsigned>("key"), 42u);
}

TEST(DictionaryDifference, IdenticalDictionaries) {
  Dictionary dict1, dict2;
  dict1("name") = std::string("test");
  dict1("answer") = 42u;
  dict1("pi") = 3.14159;

  dict2("name") = std::string("test");
  dict2("answer") = 42u;
  dict2("pi") = 3.14159;

  Dictionary diff = dict1.difference(dict2);
  ASSERT_TRUE(diff.is_empty());
}

TEST(DictionaryDifference, DifferentValues) {
  Dictionary dict1, dict2;
  dict1("name") = std::string("original");
  dict1("value") = 42;
  dict1("same") = 100;

  dict2("name") = std::string("modified");
  dict2("value") = 42;  // same
  dict2("same") = 100;  // same

  Dictionary diff = dict1.difference(dict2);
  ASSERT_FALSE(diff.is_empty());
  ASSERT_TRUE(diff.has("name"));
  ASSERT_FALSE(diff.has("value"));
  ASSERT_FALSE(diff.has("same"));
  ASSERT_EQ(diff.get<std::string>("name"), "original");
}

TEST(DictionaryDifference, UniqueKeys) {
  Dictionary dict1, dict2;
  dict1("common") = 42;
  dict1("unique_to_1") = std::string("only in dict1");

  dict2("common") = 42;
  dict2("unique_to_2") = std::string("only in dict2");

  Dictionary diff = dict1.difference(dict2);
  ASSERT_FALSE(diff.is_empty());
  ASSERT_FALSE(diff.has("common"));
  ASSERT_TRUE(diff.has("unique_to_1"));
  ASSERT_FALSE(diff.has("unique_to_2"));
  ASSERT_EQ(diff.get<std::string>("unique_to_1"), "only in dict1");
}

TEST(DictionaryDifference, NestedDictionaries) {
  Dictionary dict1, dict2;

  // Set up nested structure in dict1
  dict1("config")("server")("port") = 8080u;
  dict1("config")("server")("host") = std::string("localhost");
  dict1("config")("database")("type") = std::string("postgres");
  dict1("config")("database")("port") = 5432u;
  dict1("data")("count") = 100;

  // Set up nested structure in dict2 (with some differences)
  dict2("config")("server")("port") = 9090u;                      // different
  dict2("config")("server")("host") = std::string("localhost");   // same
  dict2("config")("database")("type") = std::string("postgres");  // same
  dict2("config")("database")("port") = 5432u;                    // same
  dict2("data")("count") = 100;                                   // same

  Dictionary diff = dict1.difference(dict2);

  // Only the differing nested keys should be present
  ASSERT_FALSE(diff.is_empty());
  ASSERT_TRUE(diff.has("config"));
  ASSERT_TRUE(diff("config").has("server"));
  ASSERT_TRUE(diff("config")("server").has("port"));
  ASSERT_FALSE(diff("config")("server").has("host"));
  ASSERT_FALSE(diff("config").has("database"));
  ASSERT_FALSE(diff.has("data"));

  ASSERT_EQ(diff("config")("server").get<unsigned>("port"), 8080u);
}

TEST(DictionaryDifference, EigenTypes) {
  Dictionary dict1, dict2;

  dict1.insert<Eigen::Vector3d>("position1", 1.0, 2.0, 3.0);
  dict1.insert<Eigen::Vector3d>("position2", 4.0, 5.0, 6.0);

  dict2.insert<Eigen::Vector3d>("position1", 1.0, 2.0, 3.0);  // same
  dict2.insert<Eigen::Vector3d>("position2", 7.0, 8.0, 9.0);  // different

  Dictionary diff = dict1.difference(dict2);

  ASSERT_FALSE(diff.is_empty());
  ASSERT_FALSE(diff.has("position1"));
  ASSERT_TRUE(diff.has("position2"));

  Eigen::Vector3d expected_diff(4.0, 5.0, 6.0);
  ASSERT_TRUE(diff.get<Eigen::Vector3d>("position2").isApprox(expected_diff));
}

TEST(DictionaryDifference, ValueVsNestedDict) {
  Dictionary dict1, dict2;

  dict1("item") = 42u;            // the answer
  dict2("item")("nested") = 42u;  // nested answer

  Dictionary diff = dict1.difference(dict2);

  ASSERT_FALSE(diff.is_empty());
  ASSERT_TRUE(diff.has("item"));
  ASSERT_EQ(diff.get<unsigned>("item"), 42u);
}

TEST(DictionaryDifference, ComplexNestedStructure) {
  Dictionary dict1, dict2;

  // Create a more complex nested structure
  dict1("app")("version") = std::string("1.0.0");
  dict1("app")("config")("debug") = true;
  dict1("app")("config")("timeout") = 30.0f;
  dict1("users").insert<std::vector<double>>("scores", 5, 95.5);
  dict1.insert<Eigen::Matrix3d>("transform", Eigen::Matrix3d::Identity());

  dict2("app")("version") = std::string("1.0.1");                 // different
  dict2("app")("config")("debug") = true;                         // same
  dict2("app")("config")("timeout") = 60.0f;                      // different
  dict2("users").insert<std::vector<double>>("scores", 5, 95.5);  // same
  dict2.insert<Eigen::Matrix3d>(
      "transform", 2.0 * Eigen::Matrix3d::Identity());  // different

  Dictionary diff = dict1.difference(dict2);

  ASSERT_FALSE(diff.is_empty());
  ASSERT_TRUE(diff.has("app"));
  ASSERT_TRUE(diff("app").has("version"));
  ASSERT_TRUE(diff("app").has("config"));
  ASSERT_FALSE(diff("app")("config").has("debug"));
  ASSERT_TRUE(diff("app")("config").has("timeout"));
  ASSERT_FALSE(diff.has("users"));
  ASSERT_TRUE(diff.has("transform"));

  ASSERT_EQ(diff("app").get<std::string>("version"), "1.0.0");
  ASSERT_EQ(diff("app")("config").get<float>("timeout"), 30.0f);
  ASSERT_TRUE(diff.get<Eigen::Matrix3d>("transform")
                  .isApprox(Eigen::Matrix3d::Identity()));

  Dictionary opposite_diff = dict2.difference(dict1);

  ASSERT_FALSE(opposite_diff.is_empty());
  ASSERT_TRUE(opposite_diff.has("app"));
  ASSERT_TRUE(opposite_diff("app").has("version"));
  ASSERT_TRUE(opposite_diff("app").has("config"));
  ASSERT_FALSE(opposite_diff("app")("config").has("debug"));
  ASSERT_TRUE(opposite_diff("app")("config").has("timeout"));
  ASSERT_FALSE(opposite_diff.has("users"));
  ASSERT_TRUE(opposite_diff.has("transform"));

  ASSERT_EQ(opposite_diff("app").get<std::string>("version"), "1.0.1");
  ASSERT_EQ(opposite_diff("app")("config").get<float>("timeout"), 60.0f);
  ASSERT_TRUE(opposite_diff.get<Eigen::Matrix3d>("transform")
                  .isApprox(2.0 * Eigen::Matrix3d::Identity()));
}

}  // namespace palimpsest
