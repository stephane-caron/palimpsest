// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Inria
/*
 * This example showcases using palimpsest dictionaries like Python ones.
 */

#include <palimpsest/Dictionary.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>
#include <string>

using palimpsest::Dictionary;

inline void print_title(const std::string& title) {
  static std::string title_sep =
      "--------------------------------------------------";
  std::cout << std::endl
            << title_sep << " " << title << " " << title_sep << std::endl
            << std::endl;
}

int main() {
  Dictionary dict;

  // Setting values
  dict("name") = "example";
  dict("temperature") = 25.5;
  dict("count") = 42u;
  dict("active") = true;

  // Nested dictionaries
  auto& config = dict("config");
  config("timeout") = 30.0;
  config("retries") = 3u;
  config("debug") = false;

  std::cout << ">>> dict\n" << dict << "\n";

  // dict.keys() - Get all keys at the top level
  std::cout << ">>> dict.keys()\n[";
  bool first = true;
  for (const auto& key : dict.keys()) {
    if (!first) std::cout << ", ";
    std::cout << "'" << key << "'";
    first = false;
  }
  std::cout << "]\n";

  // dict.get(key, default) - Get value with default fallback
  print_title("dict.get");
  std::string name = dict.get<std::string>("name", "unknown");
  std::string missing = dict.get<std::string>("missing", "default_value");
  double timeout = dict.get<double>("nonexistent", 10.0);

  std::cout << ">>> dict.get('name', 'unknown')\n'" << name << "'\n";
  std::cout << ">>> dict.get('missing', 'default_value')\n'" << missing
            << "'\n";
  std::cout << ">>> dict.get('nonexistent', 10.0)\n" << timeout << "\n";

  // dict.update(other) - Update with another dictionary
  print_title("dict.update");
  Dictionary updates;
  updates("temperature") = 28.0;  // Update existing
  updates("humidity") = 65.0;     // Add new
  updates("location") = "Paris";  // Add new

  std::cout << ">>> updates\n" << updates << "\n";
  std::cout << ">>> dict.update(updates)\n";
  dict.update(updates);
  std::cout << ">>> dict\n" << dict << "\n";

  // Accessing nested dictionary keys
  std::cout << ">>> dict['config'].keys()\n[";
  first = true;
  for (const auto& key : dict("config").keys()) {
    if (!first) std::cout << ", ";
    std::cout << "'" << key << "'";
    first = false;
  }
  std::cout << "]\n";

  // dict.clear() - Clear all contents
  print_title("dict.clear");
  Dictionary temp_dict;
  temp_dict("a") = 1;
  temp_dict("b") = 2;
  std::cout << ">>> temp_dict\n" << temp_dict << "\n";
  std::cout << ">>> temp_dict.clear()\n";
  temp_dict.clear();
  std::cout << ">>> temp_dict\n" << temp_dict << "\n";

  // Demonstrate that cleared dictionary has no keys
  std::cout << ">>> temp_dict.keys()\n[";
  first = true;
  for (const auto& key : temp_dict.keys()) {
    if (!first) std::cout << ", ";
    std::cout << "'" << key << "'";
    first = false;
  }
  std::cout << "]\n";

  // dict.pop(key) - Remove and return value (like Python dict.pop)
  print_title("dict.pop");
  std::cout << ">>> dict\n" << dict << "\n";
  std::cout << ">>> dict.pop<double>('humidity')\n";
  double humidity = dict.pop<double>("humidity");
  std::cout << humidity << "\n";
  std::cout << ">>> dict\n" << dict << "\n";

  // dict.pop(key, default) - Remove and return value, or default if missing
  std::cout << ">>> dict.pop<std::string>('missing_key', 'not_found')\n";
  std::string missing_result =
      dict.pop<std::string>("missing_key", "not_found");
  std::cout << "'" << missing_result << "'\n";

  std::cout << ">>> dict\n" << dict << "\n";
  std::cout << ">>> dict.pop<std::string>('location')\n";
  std::string location = dict.pop<std::string>("location");
  std::cout << "'" << location << "'\n";
  std::cout << ">>> dict\n" << dict << "\n";

  return EXIT_SUCCESS;
}
