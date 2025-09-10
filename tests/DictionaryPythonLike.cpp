// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Inria

#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "palimpsest/Dictionary.h"
#include "palimpsest/exceptions/KeyError.h"
#include "palimpsest/exceptions/TypeError.h"

using palimpsest::Dictionary;
using palimpsest::exceptions::KeyError;
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

TEST_F(DictionaryPythonLikeTest, FromkeysWithValue) {
  std::vector<std::string> keys = {"name", "age", "city"};
  Dictionary dict = Dictionary::fromkeys(keys, std::string("unknown"));

  EXPECT_EQ(dict.size(), 3);
  EXPECT_TRUE(dict.has("name"));
  EXPECT_TRUE(dict.has("age"));
  EXPECT_TRUE(dict.has("city"));

  EXPECT_EQ(dict.get<std::string>("name"), "unknown");
  EXPECT_EQ(dict.get<std::string>("age"), "unknown");
  EXPECT_EQ(dict.get<std::string>("city"), "unknown");
}

TEST_F(DictionaryPythonLikeTest, FromkeysWithoutValue) {
  std::vector<std::string> keys = {"config", "data", "meta"};
  Dictionary dict = Dictionary::fromkeys(keys);

  EXPECT_EQ(dict.size(), 3);
  EXPECT_TRUE(dict.has("config"));
  EXPECT_TRUE(dict.has("data"));
  EXPECT_TRUE(dict.has("meta"));

  EXPECT_TRUE(dict("config").is_empty());
  EXPECT_TRUE(dict("data").is_empty());
  EXPECT_TRUE(dict("meta").is_empty());

  // Should be able to use as nested dictionaries
  dict("config")("timeout") = 30.0;
  EXPECT_EQ(dict("config").get<double>("timeout"), 30.0);
}

TEST_F(DictionaryPythonLikeTest, FromkeysWithNumericValue) {
  std::vector<std::string> keys = {"x", "y", "z"};
  Dictionary dict = Dictionary::fromkeys(keys, 42);

  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(dict.get<int>("x"), 42);
  EXPECT_EQ(dict.get<int>("y"), 42);
  EXPECT_EQ(dict.get<int>("z"), 42);
}

TEST_F(DictionaryPythonLikeTest, FromkeysWithBoolValue) {
  std::vector<std::string> keys = {"enabled", "active", "visible"};
  Dictionary dict = Dictionary::fromkeys(keys, true);

  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(dict.get<bool>("enabled"), true);
  EXPECT_EQ(dict.get<bool>("active"), true);
  EXPECT_EQ(dict.get<bool>("visible"), true);
}

TEST_F(DictionaryPythonLikeTest, FromkeysWithDoubleValue) {
  std::vector<std::string> keys = {"temperature", "pressure", "humidity"};
  Dictionary dict = Dictionary::fromkeys(keys, 25.5);

  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(dict.get<double>("temperature"), 25.5);
  EXPECT_EQ(dict.get<double>("pressure"), 25.5);
  EXPECT_EQ(dict.get<double>("humidity"), 25.5);
}

TEST_F(DictionaryPythonLikeTest, FromkeysEmptyContainer) {
  std::vector<std::string> empty_keys;
  Dictionary dict = Dictionary::fromkeys(empty_keys, std::string("default"));

  EXPECT_EQ(dict.size(), 0);
  EXPECT_TRUE(dict.is_empty());
}

TEST_F(DictionaryPythonLikeTest, FromkeysWithInitializerList) {
  std::vector<std::string> keys = {"a", "b", "c"};
  Dictionary dict = Dictionary::fromkeys(keys, 100);

  EXPECT_EQ(dict.size(), 3);
  EXPECT_EQ(dict.get<int>("a"), 100);
  EXPECT_EQ(dict.get<int>("b"), 100);
  EXPECT_EQ(dict.get<int>("c"), 100);
}

TEST_F(DictionaryPythonLikeTest, FromkeysWithDuplicateKeys) {
  std::vector<std::string> keys = {"key1", "key2", "key1", "key3"};
  Dictionary dict = Dictionary::fromkeys(keys, std::string("value"));

  // Should have only 3 unique keys
  EXPECT_EQ(dict.size(), 3);
  EXPECT_TRUE(dict.has("key1"));
  EXPECT_TRUE(dict.has("key2"));
  EXPECT_TRUE(dict.has("key3"));
  EXPECT_EQ(dict.get<std::string>("key1"), "value");
  EXPECT_EQ(dict.get<std::string>("key2"), "value");
  EXPECT_EQ(dict.get<std::string>("key3"), "value");
}

TEST_F(DictionaryPythonLikeTest, FromkeysModifyValues) {
  std::vector<std::string> keys = {"counter1", "counter2"};
  Dictionary dict = Dictionary::fromkeys(keys, 0);

  // Modify one value
  dict("counter1") = 10;

  // Check that only the modified value changed
  EXPECT_EQ(dict.get<int>("counter1"), 10);
  EXPECT_EQ(dict.get<int>("counter2"), 0);
}

TEST_F(DictionaryPythonLikeTest, PopitemBasicUsage) {
  dict_("temperature") = 25.5;
  dict_("pressure") = 101.3;
  dict_("humidity") = 65.0;

  EXPECT_EQ(dict_.size(), 3);

  // Pop one item
  auto [key, value] = dict_.popitem();

  // Check that the dictionary size decreased
  EXPECT_EQ(dict_.size(), 2);

  // Check that we got a valid key-value pair
  EXPECT_FALSE(key.empty());
  EXPECT_TRUE(value.is_value());

  // Verify the popped key is no longer in the dictionary
  EXPECT_FALSE(dict_.has(key));
}

TEST_F(DictionaryPythonLikeTest, PopitemSpecificValues) {
  dict_("name") = std::string("Alice");
  dict_("age") = 30;
  dict_("active") = true;

  auto [key, value] = dict_.popitem();

  // Check that we can extract the correct typed value
  if (key == "name") {
    EXPECT_EQ(value.as<std::string>(), "Alice");
  } else if (key == "age") {
    EXPECT_EQ(value.as<int>(), 30);
  } else if (key == "active") {
    EXPECT_EQ(value.as<bool>(), true);
  } else {
    FAIL() << "Unexpected key: " << key;
  }

  // Verify the original dictionary no longer has this key
  EXPECT_FALSE(dict_.has(key));
  EXPECT_EQ(dict_.size(), 2);
}

TEST_F(DictionaryPythonLikeTest, PopitemEmptyDictionary) {
  EXPECT_TRUE(dict_.is_empty());
  EXPECT_THROW(dict_.popitem(), KeyError);
}

TEST_F(DictionaryPythonLikeTest, PopitemOnNonDictionary) {
  dict_ = std::string("I am a string value");

  EXPECT_THROW(dict_.popitem(), TypeError);
}

TEST_F(DictionaryPythonLikeTest, PopitemMultipleItems) {
  dict_("x") = 1.0;
  dict_("y") = 2.0;
  dict_("z") = 3.0;

  std::unordered_map<std::string, double> popped_items;

  // Pop all items
  while (dict_.size() > 0) {
    auto [key, value] = dict_.popitem();
    popped_items[key] = value.as<double>();
  }

  // Check that we popped all items correctly
  EXPECT_EQ(popped_items.size(), 3);
  EXPECT_EQ(popped_items["x"], 1.0);
  EXPECT_EQ(popped_items["y"], 2.0);
  EXPECT_EQ(popped_items["z"], 3.0);

  // Dictionary should be empty now
  EXPECT_TRUE(dict_.is_empty());
  EXPECT_EQ(dict_.size(), 0);
}

TEST_F(DictionaryPythonLikeTest, PopitemNestedDictionary) {
  auto& config = dict_("config");
  config("timeout") = 30.0;
  config("debug") = false;

  dict_("version") = std::string("1.0");

  auto [key, value] = dict_.popitem();

  if (key == "config") {
    EXPECT_TRUE(value.is_map());
    EXPECT_TRUE(value.has("timeout"));
    EXPECT_TRUE(value.has("debug"));
    EXPECT_EQ(value.get<double>("timeout"), 30.0);
    EXPECT_EQ(value.get<bool>("debug"), false);
  } else if (key == "version") {
    EXPECT_TRUE(value.is_value());
    EXPECT_EQ(value.as<std::string>(), "1.0");
  } else {
    FAIL() << "Unexpected key: " << key;
  }

  EXPECT_EQ(dict_.size(), 1);
}

TEST_F(DictionaryPythonLikeTest, PopitemReturnedValueIsIndependent) {
  dict_("temperature") = 25.5;

  auto [key, value] = dict_.popitem();
  EXPECT_EQ(key, "temperature");
  EXPECT_EQ(value.as<double>(), 25.5);

  // Modify the returned value
  value = 30.0;
  EXPECT_EQ(value.as<double>(), 30.0);

  // The original dictionary should be empty (value was moved)
  EXPECT_TRUE(dict_.is_empty());
  EXPECT_FALSE(dict_.has("temperature"));
}
