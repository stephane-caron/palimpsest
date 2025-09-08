// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Inria

#include <gtest/gtest.h>

#include <string>
#include <unordered_map>

#include "palimpsest/Dictionary.h"
#include "palimpsest/exceptions/TypeError.h"

using palimpsest::Dictionary;
using palimpsest::exceptions::TypeError;

class DictionaryPythonLikeTest : public ::testing::Test {
 protected:
  Dictionary dict_;
};

TEST_F(DictionaryPythonLikeTest, SetDefaultNewKey) {
  std::string default_value = "default";
  std::string& result = dict_.setdefault("key", default_value);

  EXPECT_EQ(result, "default");
  EXPECT_EQ(dict_.get<std::string>("key"), "default");
  EXPECT_TRUE(dict_.has("key"));
}

TEST_F(DictionaryPythonLikeTest, SetDefaultExistingKey) {
  dict_("existing") = std::string("original");

  std::string default_value = "default";
  std::string& result = dict_.setdefault("existing", default_value);

  EXPECT_EQ(result, "original");
  EXPECT_EQ(dict_.get<std::string>("existing"), "original");
}

TEST_F(DictionaryPythonLikeTest, SetDefaultNumericTypes) {
  int default_int = 42;
  int& int_result = dict_.setdefault("int_key", default_int);
  EXPECT_EQ(int_result, 42);
  EXPECT_EQ(dict_.get<int>("int_key"), 42);

  double default_double = 3.14;
  double& double_result = dict_.setdefault("double_key", default_double);
  EXPECT_EQ(double_result, 3.14);
  EXPECT_EQ(dict_.get<double>("double_key"), 3.14);
}

TEST_F(DictionaryPythonLikeTest, SetDefaultBoolType) {
  bool default_bool = true;
  bool& bool_result = dict_.setdefault("bool_key", default_bool);
  EXPECT_EQ(bool_result, true);
  EXPECT_EQ(dict_.get<bool>("bool_key"), true);

  bool& existing_bool = dict_.setdefault("bool_key", false);
  EXPECT_EQ(existing_bool, true);
}

TEST_F(DictionaryPythonLikeTest, SetDefaultModifyReturnedReference) {
  std::string default_value = "initial";
  std::string& result = dict_.setdefault("key", default_value);

  result = "modified";

  EXPECT_EQ(dict_.get<std::string>("key"), "modified");
}

TEST_F(DictionaryPythonLikeTest, SetDefaultOnNonDictionary) {
  dict_ = std::string("I am a string value");

  std::string default_value = "default";
  EXPECT_THROW(dict_.setdefault("key", default_value), TypeError);
}

TEST_F(DictionaryPythonLikeTest, SetDefaultKeyWithDictionary) {
  Dictionary& child_dict = dict_("child");
  child_dict("nested") = std::string("value");

  std::string default_value = "default";
  EXPECT_THROW(dict_.setdefault<std::string>("child", default_value),
               TypeError);
}

TEST_F(DictionaryPythonLikeTest, SetDefaultTypeMismatch) {
  dict_("key") = std::string("string_value");

  int default_value = 42;
  EXPECT_THROW(dict_.setdefault("key", default_value), TypeError);
}

TEST_F(DictionaryPythonLikeTest, SetDefaultMultipleCallsConsistency) {
  std::string default_value = "default";

  std::string& result1 = dict_.setdefault("key", default_value);
  std::string& result2 = dict_.setdefault("key", std::string("other_default"));

  EXPECT_EQ(result1, "default");
  EXPECT_EQ(result2, "default");
  EXPECT_EQ(&result1, &result2);  // Should return same reference
}

TEST_F(DictionaryPythonLikeTest, ItemsEmptyDictionary) {
  auto items = dict_.items();

  EXPECT_TRUE(items.empty());
  EXPECT_EQ(items.size(), 0);
}

TEST_F(DictionaryPythonLikeTest, ItemsSingleValue) {
  dict_("name") = std::string("test");

  auto items = dict_.items();

  EXPECT_EQ(items.size(), 1);
  EXPECT_EQ(items[0].first, "name");
  EXPECT_EQ(items[0].second.get().as<std::string>(), "test");
}

TEST_F(DictionaryPythonLikeTest, ItemsMultipleValues) {
  dict_("name") = std::string("Alice");
  dict_("age") = 30;
  dict_("active") = true;

  auto items = dict_.items();

  EXPECT_EQ(items.size(), 3);

  std::unordered_map<std::string, bool> found_keys;
  for (const auto& item : items) {
    const std::string& key = item.first;
    const Dictionary& value = item.second.get();

    found_keys[key] = true;

    if (key == "name") {
      EXPECT_EQ(value.as<std::string>(), "Alice");
    } else if (key == "age") {
      EXPECT_EQ(value.as<int>(), 30);
    } else if (key == "active") {
      EXPECT_EQ(value.as<bool>(), true);
    }
  }

  EXPECT_TRUE(found_keys["name"]);
  EXPECT_TRUE(found_keys["age"]);
  EXPECT_TRUE(found_keys["active"]);
}

TEST_F(DictionaryPythonLikeTest, ItemsNestedDictionaries) {
  auto& config = dict_("config");
  config("timeout") = 30.0;
  config("debug") = false;

  dict_("version") = std::string("1.0");

  auto items = dict_.items();

  EXPECT_EQ(items.size(), 2);

  for (const auto& item : items) {
    const std::string& key = item.first;
    const Dictionary& value = item.second.get();

    if (key == "config") {
      EXPECT_TRUE(value.is_map());
      EXPECT_TRUE(value.has("timeout"));
      EXPECT_TRUE(value.has("debug"));
      EXPECT_EQ(value.get<double>("timeout"), 30.0);
      EXPECT_EQ(value.get<bool>("debug"), false);
    } else if (key == "version") {
      EXPECT_TRUE(value.is_value());
      EXPECT_EQ(value.as<std::string>(), "1.0");
    }
  }
}

TEST_F(DictionaryPythonLikeTest, ItemsIteratorUsage) {
  dict_("x") = 1;
  dict_("y") = 2;
  dict_("z") = 3;

  auto items = dict_.items();

  int sum = 0;
  for (const auto& [key, value_ref] : items) {
    sum += value_ref.get().as<int>();
  }

  EXPECT_EQ(sum, 6);
}

TEST_F(DictionaryPythonLikeTest, ItemsWithDifferentTypes) {
  dict_("string") = std::string("hello");
  dict_("integer") = 42;
  dict_("double") = 3.14;
  dict_("bool") = true;

  auto items = dict_.items();

  EXPECT_EQ(items.size(), 4);

  for (const auto& item : items) {
    const std::string& key = item.first;
    const Dictionary& value = item.second.get();

    if (key == "string") {
      EXPECT_EQ(value.as<std::string>(), "hello");
    } else if (key == "integer") {
      EXPECT_EQ(value.as<int>(), 42);
    } else if (key == "double") {
      EXPECT_EQ(value.as<double>(), 3.14);
    } else if (key == "bool") {
      EXPECT_EQ(value.as<bool>(), true);
    }
  }
}
