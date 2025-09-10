// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "palimpsest/Dictionary.h"

using palimpsest::Dictionary;

class FmtFormatterTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

TEST_F(FmtFormatterTest, FormatsEmptyDictionary) {
  Dictionary dict;
  std::string formatted = fmt::format("{}", dict);

  // The empty dictionary should format the same as std::cout << dict
  std::ostringstream oss;
  oss << dict;

  EXPECT_EQ(formatted, oss.str());
  EXPECT_FALSE(formatted.empty());
}

TEST_F(FmtFormatterTest, FormatsSimpleValues) {
  Dictionary dict;
  dict("temperature") = 25.5;
  dict("name") = std::string("sensor");
  dict("count") = 42;

  std::string formatted = fmt::format("{}", dict);

  // Compare with ostream output
  std::ostringstream oss;
  oss << dict;

  EXPECT_EQ(formatted, oss.str());
  EXPECT_FALSE(formatted.empty());
}

TEST_F(FmtFormatterTest, FormatsNestedDictionaries) {
  Dictionary dict;
  dict("config")("timeout") = 30;
  dict("config")("retries") = 3;
  dict("sensors")("temperature") = 25.5;
  dict("sensors")("pressure") = 101.3;

  std::string formatted = fmt::format("{}", dict);

  // Compare with ostream output
  std::ostringstream oss;
  oss << dict;

  EXPECT_EQ(formatted, oss.str());
  EXPECT_FALSE(formatted.empty());
}

TEST_F(FmtFormatterTest, FormatsEigenTypes) {
  Dictionary dict;
  Eigen::Vector3d vec(1.0, 2.0, 3.0);
  dict("position") = vec;

  std::string formatted = fmt::format("{}", dict);

  // Compare with ostream output
  std::ostringstream oss;
  oss << dict;

  EXPECT_EQ(formatted, oss.str());
  EXPECT_FALSE(formatted.empty());
}

TEST_F(FmtFormatterTest, FormatsWithCustomFormatString) {
  Dictionary dict;
  dict("value") = 42.5;

  // Test that the formatter works with fmt::format calls
  std::string formatted = fmt::format("Dictionary content: {}", dict);

  std::ostringstream oss;
  oss << "Dictionary content: " << dict;

  EXPECT_EQ(formatted, oss.str());
  EXPECT_THAT(formatted, ::testing::StartsWith("Dictionary content: "));
}
