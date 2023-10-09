/*
 * Copyright 2022 Stéphane Caron
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     Configuration and DataStore classes of mc_rtc
 *     Copyright 2015-2020 CNRS-UM LIRMM, CNRS-AIST JRL
 *     License: BSD-2-Clause
 */

#include "palimpsest/Dictionary.h"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "palimpsest/exceptions/KeyError.h"
#include "palimpsest/exceptions/TypeError.h"
#include "palimpsest/mpack/eigen.h"

namespace palimpsest {

using exceptions::KeyError;
using exceptions::TypeError;
using mpack::mpack_node_matrix3d;
using mpack::mpack_node_quaterniond;
using mpack::mpack_node_vector2d;
using mpack::mpack_node_vector3d;
using mpack::mpack_node_vectorXd;

void Dictionary::clear() noexcept {
  assert(this->is_map());
  map_.clear();
}

void Dictionary::update(const char *data, size_t size) {
  mpack_tree_t tree;
  mpack_tree_init_data(&tree, data, size);
  mpack_tree_parse(&tree);
  const auto status = mpack_tree_error(&tree);
  if (status != mpack_ok) {
    spdlog::error("MPack tree error: \"{}\", skipping Dictionary::update",
                  mpack_error_to_string(status));
    return;
  }
  update(mpack_tree_root(&tree));
  mpack_tree_destroy(&tree);
}

void Dictionary::update(mpack_node_t node) {
  if (this->is_value()) {
    value_.deserialize(node);
    return;
  }

  /* Now we have asserted that this->is_map() */
  if (mpack_node_type(node) != mpack_type_map) {
    throw TypeError(__FILE__, __LINE__,
                    std::string("Expecting a map, not ") +
                        mpack_type_to_string(mpack_node_type(node)));
  }

  for (size_t i = 0; i < mpack_node_map_count(node); ++i) {
    const mpack_node_t key_node = mpack_node_map_key_at(node, i);
    const mpack_node_t value_node = mpack_node_map_value_at(node, i);
    const std::string key = {mpack_node_str(key_node),
                             mpack_node_strlen(key_node)};
    auto it = map_.find(key);
    if (it == map_.end()) {
      this->insert_at_key_(key, value_node);
    } else /* (it != map_.end()) */ {
      try {
        it->second->update(value_node);
      } catch (const TypeError &e) {
        throw TypeError(e, "(at key \"" + key + "\") ");
      }
    }
  }
}

void Dictionary::insert_at_key_(const std::string &key,
                                const mpack_node_t &value) {
  switch (mpack_node_type(value)) {
    case mpack_type_bool:
      this->insert<bool>(key, mpack_node_bool(value));
      break;
    case mpack_type_int:
      this->insert<int>(key, mpack_node_int(value));
      break;
    case mpack_type_uint:
      this->insert<unsigned>(key, mpack_node_uint(value));
      break;
    case mpack_type_float:
      this->insert<float>(key, mpack_node_float(value));
      break;
    case mpack_type_double:
      this->insert<double>(key, mpack_node_double(value));
      break;
    case mpack_type_str:
      this->insert<std::string>(
          key, std::string{mpack_node_str(value), mpack_node_strlen(value)});
      break;
    case mpack_type_array:
      switch (mpack_node_array_length(value)) {
        case 2:
          this->insert<Eigen::Vector2d>(key, mpack_node_vector2d(value));
          break;
        case 3:
          this->insert<Eigen::Vector3d>(key, mpack_node_vector3d(value));
          break;
        case 4:
          this->insert<Eigen::Quaterniond>(key, mpack_node_quaterniond(value));
          break;
        case 9:
          this->insert<Eigen::Matrix3d>(key, mpack_node_matrix3d(value));
          break;
        default:
          this->insert<Eigen::VectorXd>(key, mpack_node_vectorXd(value));
          break;
      }
      break;
    case mpack_type_map:
      this->operator()(key).update(value);
      break;
    case mpack_type_bin:
    case mpack_type_nil:
    default:
      throw TypeError(__FILE__, __LINE__,
                      std::string("Cannot insert values of type ") +
                          mpack_type_to_string(mpack_node_type(value)));
  }
}

std::vector<std::string> Dictionary::keys() const noexcept {
  std::vector<std::string> out;
  out.reserve(map_.size());
  for (const auto &key_child : map_) {
    out.push_back(key_child.first);
  }
  return out;
}

void Dictionary::remove(const std::string &key) noexcept {
  auto it = map_.find(key);
  if (it == map_.end()) {
    spdlog::error("[Dictionary::remove] No key to remove at \"{}\"", key);
    return;
  }
  map_.erase(it);
}

Dictionary &Dictionary::operator()(const std::string &key) {
  if (this->is_value()) {
    throw TypeError(__FILE__, __LINE__,
                    "Cannot look up at key \"" + key +
                        "\" in non-dictionary object of type \"" +
                        value_.type_name() + "\".");
  }
  auto [it, _] = map_.try_emplace(key, std::make_unique<Dictionary>());
  return *it->second;
}

const Dictionary &Dictionary::operator()(const std::string &key) const {
  if (this->is_value()) {
    throw TypeError(__FILE__, __LINE__,
                    "Cannot lookup at key \"" + key +
                        "\" in non-dictionary object of type \"" +
                        value_.type_name() + "\".");
  }
  const auto it = map_.find(key);
  if (it == map_.end()) {
    throw KeyError(key, __FILE__, __LINE__,
                   "Since the dictionary is const it cannot be created.");
  }
  return *it->second;
}

void Dictionary::read(const std::string &filename) {
  std::ifstream input;
  input.open(filename, std::ifstream::binary | std::ios::ate);
  std::streamsize size = input.tellg();
  input.seekg(0, std::ios::beg);
  std::vector<char> buffer(size);
  input.read(buffer.data(), size);
  input.close();
  this->update(buffer.data(), size);
}

void Dictionary::write(const std::string &filename) const {
  std::vector<char> buffer;
  size_t size = this->serialize(buffer);

  std::ofstream output;
  output.open(filename, std::ofstream::binary);
  output.write(buffer.data(), static_cast<int>(size));
  output.close();
}

size_t Dictionary::serialize(std::vector<char> &buffer) const {
  mpack::Writer writer(buffer);
  serialize_(writer);
  return writer.finish();
}

void Dictionary::serialize_(mpack::Writer &writer) const {
  if (this->is_value()) {
    value_.serialize(writer);
    return;
  }
  size_t size = map_.size();
  writer.start_map(size);
  for (const auto &key_child : map_) {
    const auto &key = key_child.first;
    const auto &child = *key_child.second;
    writer.write(key);
    child.serialize_(writer);
  }
  writer.finish_map();
}

const Dictionary::Value &Dictionary::get_child_value_(
    const std::string &key) const {
  const auto it = map_.find(key);
  if (it == map_.end()) {
    throw KeyError(key, __FILE__, __LINE__, "");
  } else if (!it->second->is_value()) {
    throw TypeError(__FILE__, __LINE__,
                    "Child at key \"" + key + "\" is not a value");
  }
  return it->second->value_;
}

std::ostream &operator<<(std::ostream &stream, const Dictionary &dict) {
  if (dict.is_empty()) {
    stream << "{}";
  } else if (dict.is_value()) {
    dict.value_.print(stream);
  } else /* (dict.is_map()) */ {
    stream << "{";
    bool is_first = true;
    for (const auto &key_child : dict.map_) {
      const auto &key = key_child.first;
      const auto &child = key_child.second;
      if (is_first) {
        is_first = false;
      } else /* is not first key */ {
        stream << ", ";
      }
      stream << "\"" << key << "\": " << *child;
    }
    stream << "}";
  }
  return stream;
}

}  // namespace palimpsest
