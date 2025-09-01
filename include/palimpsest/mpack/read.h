// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 Stéphane Caron

#pragma once

#include <mpack.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>

#include "palimpsest/exceptions/TypeError.h"

namespace palimpsest {

using exceptions::TypeError;

namespace mpack {

/*! Read a value from MessagePack.
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <typename T>
void read(const mpack_node_t node, T& value) {
  throw TypeError(
      __FILE__, __LINE__,
      std::string("No known deserialization function for typeid \"") +
          typeid(T).name() + "\"");
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, bool& value) {
  if (mpack_node_type(node) != mpack_type_bool) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting bool, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_bool(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, int8_t& value) {
  if (mpack_node_type(node) != mpack_type_int &&
      mpack_node_type(node) != mpack_type_uint) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting int8_t, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_i8(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, int16_t& value) {
  if (mpack_node_type(node) != mpack_type_int &&
      mpack_node_type(node) != mpack_type_uint) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting int16_t, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_i16(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, int32_t& value) {
  if (mpack_node_type(node) != mpack_type_int &&
      mpack_node_type(node) != mpack_type_uint) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting int32_t, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_i32(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, int64_t& value) {
  if (mpack_node_type(node) != mpack_type_int &&
      mpack_node_type(node) != mpack_type_uint) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting int64_t, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_i64(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, uint8_t& value) {
  if (mpack_node_type(node) != mpack_type_uint) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting uint8_t, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_u8(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, uint16_t& value) {
  if (mpack_node_type(node) != mpack_type_uint) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting uint16_t, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_u16(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, uint32_t& value) {
  if (mpack_node_type(node) != mpack_type_uint) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting uint32_t, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_u32(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, uint64_t& value) {
  if (mpack_node_type(node) != mpack_type_uint) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting uint64_t, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_u64(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, float& value) {
  switch (mpack_node_type(node)) {
    case mpack_type_int:
    case mpack_type_uint:
    case mpack_type_float:
    case mpack_type_double:
      break;
    default:
      throw TypeError(
          __FILE__, __LINE__,
          std::string("Expecting float, but deserialized node has type ") +
              mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_float(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, double& value) {
  switch (mpack_node_type(node)) {
    case mpack_type_int:
    case mpack_type_uint:
    case mpack_type_float:
    case mpack_type_double:
      break;
    default:
      throw TypeError(
          __FILE__, __LINE__,
          std::string("Expecting double, but deserialized node has type ") +
              mpack_type_to_string(mpack_node_type(node)));
  }
  value = mpack_node_double(node);
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, std::string& value) {
  if (mpack_node_type(node) != mpack_type_str) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting std::string, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  value = std::string{mpack_node_str(node), mpack_node_strlen(node)};
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, Eigen::Vector2d& value) {
  if (mpack_node_type(node) != mpack_type_array) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting an array, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  assert(mpack_node_array_length(node) == 2);
  read<double>(mpack_node_array_at(node, 0), value.x());
  read<double>(mpack_node_array_at(node, 1), value.y());
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, Eigen::Vector3d& value) {
  if (mpack_node_type(node) != mpack_type_array) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting an array, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  assert(mpack_node_array_length(node) == 3);
  read<double>(mpack_node_array_at(node, 0), value.x());
  read<double>(mpack_node_array_at(node, 1), value.y());
  read<double>(mpack_node_array_at(node, 2), value.z());
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, Eigen::VectorXd& value) {
  if (mpack_node_type(node) != mpack_type_array) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting an array, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  const unsigned length = mpack_node_array_length(node);
  assert(length == value.size());
  for (size_t i = 0; i < length; ++i) {
    read<double>(mpack_node_array_at(node, i), value(i));
  }
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, Eigen::Quaterniond& value) {
  if (mpack_node_type(node) != mpack_type_array) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting an array, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  assert(mpack_node_array_length(node) == 4);
  read<double>(mpack_node_array_at(node, 0), value.w());
  read<double>(mpack_node_array_at(node, 1), value.x());
  read<double>(mpack_node_array_at(node, 2), value.y());
  read<double>(mpack_node_array_at(node, 3), value.z());
}

/*! Specialization of read<T>(node, value)
 *
 * @param[in] node MPack node to read the value from.
 * @param[out] value Reference to write the value to.
 *
 * @throw TypeError if there is no deserialization for type T.
 */
template <>
inline void read(const mpack_node_t node, Eigen::Matrix3d& value) {
  if (mpack_node_type(node) != mpack_type_array) {
    throw TypeError(
        __FILE__, __LINE__,
        std::string("Expecting an array, but deserialized node has type ") +
            mpack_type_to_string(mpack_node_type(node)));
  }
  assert(mpack_node_array_length(node) == 9);
  for (Eigen::Index i = 0; i < 3; ++i) {
    for (Eigen::Index j = 0; j < 3; ++j) {
      read<double>(mpack_node_array_at(node, 3 * i + j), value(i, j));
    }
  }
}

}  // namespace mpack

}  // namespace palimpsest
