// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 Stéphane Caron
// Copyright 2024 Inria
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     Configuration and DataStore classes of mc_rtc
 *     Copyright 2015-2020 CNRS-UM LIRMM, CNRS-AIST JRL
 *     SPDX-License-Identifier: BSD-2-Clause
 */

#include "palimpsest/Dictionary.h"

#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "palimpsest/exceptions/KeyError.h"
#include "palimpsest/exceptions/PalimpsestError.h"
#include "palimpsest/exceptions/TypeError.h"
#include "palimpsest/mpack/eigen.h"

namespace palimpsest {

using exceptions::KeyError;
using exceptions::PalimpsestError;
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

Dictionary Dictionary::deepcopy(const Dictionary &other) {
  Dictionary result;
  std::vector<char> buffer;
  size_t size = other.serialize(buffer);
  result.deserialize(buffer.data(), size);
  return result;
}

void Dictionary::deserialize(const char *data, size_t size) {
  mpack_tree_t tree;
  mpack_tree_init_data(&tree, data, size);
  mpack_tree_parse(&tree);
  const auto status = mpack_tree_error(&tree);
  if (status != mpack_ok) {
    spdlog::error("MPack tree error: \"{}\", skipping Dictionary::deserialize",
                  mpack_error_to_string(status));
    return;
  }
  deserialize_(mpack_tree_root(&tree));
  mpack_tree_destroy(&tree);
}

void Dictionary::deserialize_(mpack_node_t node) {
  if (mpack_node_type(node) == mpack_type_nil) {
    return;
  }

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
        it->second->deserialize_(value_node);
      } catch (const TypeError &e) {
        throw TypeError(e, " ← at key \"" + key + "\"");
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
    case mpack_type_array: {
      size_t length = mpack_node_array_length(value);
      if (length == 0) {
        throw TypeError(__FILE__, __LINE__,
                        std::string("Cannot deserialize an empty list "
                                    "(precludes type inference) at key \"") +
                            key + "\"");
      }
      mpack_node_t first_item = mpack_node_array_at(value, 0);
      mpack_type_t array_type = mpack_node_type(first_item);
      if (array_type == mpack_type_double) {
        switch (length) {
          case 2:
            this->insert<Eigen::Vector2d>(key, mpack_node_vector2d(value));
            break;
          case 3:
            this->insert<Eigen::Vector3d>(key, mpack_node_vector3d(value));
            break;
          case 4:
            this->insert<Eigen::Quaterniond>(key,
                                             mpack_node_quaterniond(value));
            break;
          case 9:
            this->insert<Eigen::Matrix3d>(key, mpack_node_matrix3d(value));
            break;
          default:
            this->insert<Eigen::VectorXd>(key, mpack_node_vectorXd(value));
            break;
        }
      } else if (array_type == mpack_type_array) {
        // We only handle lists of Eigen::VectorXd vectors for now
        auto &new_vec_vec =
            this->insert<std::vector<Eigen::VectorXd>>(key, length);
        for (unsigned index = 0; index < length; ++index) {
          mpack_node_t sub_array = mpack_node_array_at(value, index);
          mpack_type_t sub_type = mpack_node_type(sub_array);
          if (sub_type != mpack_type_array) {
            throw TypeError(__FILE__, __LINE__,
                            std::string("Encountered non-array item ") +
                                mpack_type_to_string(sub_type) +
                                " while parsing array of arrays at key \"" +
                                key + "\"");
          }
          unsigned sub_length = mpack_node_array_length(sub_array);
          Eigen::VectorXd &vector = new_vec_vec[index];
          vector.resize(sub_length);
          for (Eigen::Index j = 0; j < sub_length; ++j) {
            vector(j) = mpack_node_double(mpack_node_array_at(sub_array, j));
          }
        }
      } else {
        throw TypeError(__FILE__, __LINE__,
                        std::string("Unsupported array of ") +
                            mpack_type_to_string(array_type) +
                            " elements encountered at key \"" + key + "\"");
      }
      break;
    }
    case mpack_type_map:
      this->operator()(key).deserialize_(value);
      break;
    case mpack_type_bin:
    case mpack_type_nil:
    default:
      throw TypeError(__FILE__, __LINE__,
                      std::string("Cannot insert values of type ") +
                          mpack_type_to_string(mpack_node_type(value)));
  }
}

Dictionary Dictionary::difference(const Dictionary &other) const {
  Dictionary result;

  // If this is empty, no differences
  if (this->is_empty()) {
    return result;
  }

  // If this is a value
  if (this->is_value()) {
    // If other is also a value, compare their serialized data
    if (other.is_value()) {
      std::vector<char> this_buffer, other_buffer;
      size_t this_size = this->serialize(this_buffer);
      size_t other_size = other.serialize(other_buffer);

      // If sizes differ or data differs, we need to return this value
      if (this_size != other_size ||
          std::memcmp(this_buffer.data(), other_buffer.data(), this_size) !=
              0) {
        // Use a temporary key to insert this value, then extract it
        std::vector<char> buffer;
        size_t size = this->serialize(buffer);

        mpack_tree_t tree;
        mpack_tree_init_data(&tree, buffer.data(), size);
        mpack_tree_parse(&tree);
        if (mpack_tree_error(&tree) == mpack_ok) {
          result.insert_at_key_("temp", mpack_tree_root(&tree));
          // Move the value from temp key to result
          Dictionary temp_result = std::move(result("temp"));
          result.clear();
          result = std::move(temp_result);
        }
        mpack_tree_destroy(&tree);
      }
    } else {
      // Other is not a value (empty or map), so this value is a difference
      std::vector<char> buffer;
      size_t size = this->serialize(buffer);

      mpack_tree_t tree;
      mpack_tree_init_data(&tree, buffer.data(), size);
      mpack_tree_parse(&tree);
      if (mpack_tree_error(&tree) == mpack_ok) {
        result.insert_at_key_("temp", mpack_tree_root(&tree));
        // Move the value from temp key to result
        Dictionary temp_result = std::move(result("temp"));
        result.clear();
        result = std::move(temp_result);
      }
      mpack_tree_destroy(&tree);
    }
    return result;
  }

  // If this is a map
  assert(this->is_map());

  for (const auto &key_child : map_) {
    const std::string &key = key_child.first;
    const Dictionary &this_child = *key_child.second;

    // Check if the key exists in other
    auto other_it = other.map_.find(key);
    if (other_it == other.map_.end()) {
      // Key doesn't exist in other, so include entire subtree as difference
      if (this_child.is_value()) {
        // For values, use insert_at_key_ to handle the serialized data properly
        std::vector<char> child_buffer;
        size_t child_size = this_child.serialize(child_buffer);
        mpack_tree_t tree;
        mpack_tree_init_data(&tree, child_buffer.data(), child_size);
        mpack_tree_parse(&tree);
        if (mpack_tree_error(&tree) == mpack_ok) {
          result.insert_at_key_(key, mpack_tree_root(&tree));
        }
        mpack_tree_destroy(&tree);
      } else {
        std::vector<char> child_buffer;
        size_t child_size = this_child.serialize(child_buffer);
        result(key).deserialize(child_buffer.data(), child_size);
      }
    } else {
      // Key exists in other, recursively compute difference
      const Dictionary &other_child = *other_it->second;
      Dictionary child_diff = this_child.difference(other_child);

      // Only include this key if there are actual differences
      if (!child_diff.is_empty()) {
        if (child_diff.is_value()) {
          // For values, use insert_at_key_
          std::vector<char> diff_buffer;
          size_t diff_size = child_diff.serialize(diff_buffer);
          mpack_tree_t tree;
          mpack_tree_init_data(&tree, diff_buffer.data(), diff_size);
          mpack_tree_parse(&tree);
          if (mpack_tree_error(&tree) == mpack_ok) {
            result.insert_at_key_(key, mpack_tree_root(&tree));
          }
          mpack_tree_destroy(&tree);
        } else {
          std::vector<char> diff_buffer;
          size_t diff_size = child_diff.serialize(diff_buffer);
          result(key).deserialize(diff_buffer.data(), diff_size);
        }
      }
    }
  }

  return result;
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
  deserialize(buffer.data(), size);
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

void Dictionary::update(const Dictionary &other) {
  throw PalimpsestError(__FILE__, __LINE__,
                        "Dictionary::update is not implemented yet");
}

}  // namespace palimpsest
