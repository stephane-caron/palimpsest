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
#include <vector>

using palimpsest::Dictionary;

inline void print_dict_keys(const Dictionary& dict) {
  std::cout << "[";
  bool first = true;
  for (const auto& key : dict.keys()) {
    if (!first) std::cout << ", ";
    std::cout << "'" << key << "'";
    first = false;
  }
  std::cout << "]";
}

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
  std::cout << ">>> dict.keys()\n";
  print_dict_keys(dict);
  std::cout << std::endl;
  std::cout << ">>> dict['config'].keys()\n";
  print_dict_keys(dict("config"));
  std::cout << std::endl;

  // dict.get(key, default) - Get value with default fallback
  print_title("dict.get");
  std::string name = dict.get<std::string>("name", "unknown");
  std::string missing = dict.get<std::string>("missing", "default_value");
  double timeout = dict.get<double>("nonexistent", 10.1);

  std::cout << ">>> dict.get('name', 'unknown')\n'" << name << "'\n";
  std::cout << ">>> dict.get('missing', 'default_value')\n'" << missing
            << "'\n";
  std::cout << ">>> dict.get('nonexistent', 10.1)\n" << timeout << "\n";

  // dict.clear() - Clear all contents
  print_title("dict.clear");
  Dictionary temp_dict;
  temp_dict("a") = 1;
  temp_dict("b") = 2;
  std::cout << ">>> temp_dict\n" << temp_dict << "\n";
  std::cout << ">>> temp_dict.clear()\n";
  temp_dict.clear();
  std::cout << ">>> temp_dict\n" << temp_dict << "\n";
  std::cout << ">>> temp_dict.keys()\n";
  print_dict_keys(temp_dict);
  std::cout << "\n";

  // dict.fromkeys(keys, value) - Create dictionary from keys with same value
  print_title("dict.fromkeys");
  std::vector<std::string> sensor_names = {"temperature", "pressure",
                                           "humidity"};
  Dictionary sensor_dict = Dictionary::fromkeys(sensor_names, 0.0);
  std::cout << ">>> sensor_names = ['temperature', 'pressure', 'humidity']\n";
  std::cout << ">>> sensor_dict = Dictionary::fromkeys(sensor_names, 0.0)\n";
  std::cout << ">>> sensor_dict\n" << sensor_dict << "\n";

  // dict.items() - Get all key-value pairs (like Python dict.items())
  print_title("dict.items");
  std::cout << ">>> for key, value in dict.items():\n";
  std::cout << "...    print(f\"- {key=}, {value=}\")\n";
  auto items = dict.items();
  for (const auto& [key, value_ref] : dict.items()) {
    const Dictionary& value = value_ref.get();
    std::cout << "- key='" << key << "', value=" << value << std::endl;
  }

  // dict.pop(key) - Remove and return value (like Python dict.pop)
  print_title("dict.pop");
  std::cout << ">>> dict\n" << dict << "\n";
  std::cout << ">>> dict.pop<bool>('active')\n";
  double active = dict.pop<bool>("active");
  std::cout << active << "\n";
  std::cout << ">>> dict\n" << dict << "\n";

  // dict.pop(key, default) - Remove and return value, or default if missing
  std::cout << ">>> dict.pop<std::string>('missing_key', 'not_found')\n";
  std::string missing_result =
      dict.pop<std::string>("missing_key", "not_found");
  std::cout << "'" << missing_result << "'\n";

  std::cout << ">>> dict\n" << dict << "\n";
  std::cout << ">>> dict.pop<std::string>('name')\n";
  std::string popped_name = dict.pop<std::string>("name");
  std::cout << "'" << popped_name << "'\n";
  std::cout << ">>> dict\n" << dict << "\n";

  // dict.setdefault(key, default) - Get value, or insert and return default
  print_title("dict.setdefault");
  std::cout << ">>> dict.setdefault('city', 'Tokyo')\n";
  std::string& city = dict.setdefault<std::string>("city", "Tokyo");
  std::cout << "'" << city << "'\n";
  std::cout << ">>> dict['city']\n'" << dict.get<std::string>("city") << "'\n";

  std::cout << ">>> dict.setdefault('city', 'London')\n";
  std::string& existing_city = dict.setdefault<std::string>("city", "London");
  std::cout << "'" << existing_city << "'\n";
  std::cout << ">>> dict['city']\n'" << dict.get<std::string>("city") << "'\n";

  // dict.update(other) - Update with another dictionary
  print_title("dict.update");
  Dictionary updates;
  updates("temperature") = 28.2;  // Update existing
  updates("humidity") = 65.3;     // Add new
  updates("location") = "Paris";  // Add new

  std::cout << ">>> dict.keys()\n";
  print_dict_keys(dict);
  std::cout << std::endl;
  std::cout << ">>> updates = " << updates << "\n";
  std::cout << ">>> dict.update(updates)\n";
  dict.update(updates);
  std::cout << ">>> dict.keys()\n";
  print_dict_keys(dict);
  std::cout << std::endl;

  return EXIT_SUCCESS;
}
