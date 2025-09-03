// SPDX-License-Identifier: Apache-2.0

#include <palimpsest/Dictionary.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>

using palimpsest::Dictionary;

int main() {
  Dictionary config_v1, config_v2;

  // First configuration dictionary
  config_v1("app")("name") = std::string("MyApp");
  config_v1("app")("version") = std::string("1.0.0");
  config_v1("server")("host") = std::string("localhost");
  config_v1("server")("port") = 8080;
  config_v1("features")("logging") = true;
  config_v1("features")("analytics") = false;

  // Second configuration dictionary
  config_v2("app")("name") = std::string("MyApp");             // same
  config_v2("app")("version") = std::string("2.0.0");          // different
  config_v2("server")("host") = std::string("api.myapp.com");  // different
  config_v2("server")("port") = 8080;                          // same
  config_v2("features")("logging") = true;                     // same
  config_v2("features")("analytics") = true;                   // different

  // Print out both dictionaries
  std::cout << "=== Dictionary difference with palimpsest ===" << std::endl;
  std::cout << std::endl;
  std::cout << "config_v1 dictionary: " << std::endl;
  std::cout << config_v1 << std::endl;
  std::cout << std::endl;
  std::cout << "config_v2 dictionary:" << std::endl;
  std::cout << config_v2 << std::endl;
  std::cout << std::endl;

  // Compute and print difference between config_v2 and config_v1
  Dictionary diff_v2_to_v1 = config_v2.difference(config_v1);
  std::cout << "config_v2.difference(config_v1):" << std::endl;
  if (diff_v2_to_v1.is_empty()) {
    std::cout << "No differences found." << std::endl;
  } else {
    std::cout << diff_v2_to_v1 << std::endl;
  }
  std::cout << std::endl;

  return EXIT_SUCCESS;
}
