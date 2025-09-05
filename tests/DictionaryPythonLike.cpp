// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Inria

#include <gtest/gtest.h>

#include <string>

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
