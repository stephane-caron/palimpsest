// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 Stéphane Caron
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     MessagePackBuilder from https://github.com/jrl-umi3218/mc_rtc/
 *     Copyright 2015-2019 CNRS-UM LIRMM, CNRS-AIST JRL
 *     SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <mpack.h>
#include <palimpsest/exceptions/TypeError.h>

#include <string>
#include <vector>

namespace palimpsest::mpack {

/*
 * Templated functions to serialize values to an mpack_writer_t. This allows
 * the compiler to select the appropriate function in mpack::write<T>(value).
 */

/*! Write a value to MPack.
 *
 * @param writer MPack writer.
 * @param value Value to write.
 *
 * This is the non-specialized version of this function. It reports a
 * non-serializable value to MPack as a type hash string.
 */
template <typename T>
void write(mpack_writer_t* writer, const T& value) {
  throw TypeError(__FILE__, __LINE__,
                  std::string("No known serialization function for typeid \"") +
                      typeid(T).name() + "\"");
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const bool& value) {
  mpack_write_bool(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const int8_t& value) {
  mpack_write_i8(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const int16_t& value) {
  mpack_write_i16(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const int32_t& value) {
  mpack_write_i32(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const int64_t& value) {
  mpack_write_i64(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const uint8_t& value) {
  mpack_write_u8(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const uint16_t& value) {
  mpack_write_u16(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const uint32_t& value) {
  mpack_write_u32(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const uint64_t& value) {
  mpack_write_u64(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const float& value) {
  mpack_write_float(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const double& value) {
  mpack_write_double(writer, value);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const std::string& value) {
  mpack_write_str(writer, value.c_str(), static_cast<uint32_t>(value.size()));
}

/*! Write a matrix to MPack.
 *
 * @param writer MPack writer.
 * @param matrix Matrix to write.
 */
template <typename T>
inline void write_matrix(mpack_writer_t* writer, const T& matrix) {
  for (Eigen::Index i = 0; i < matrix.rows(); ++i) {
    for (Eigen::Index j = 0; j < matrix.cols(); ++j) {
      write<double>(writer, matrix(i, j));
    }
  }
}

/*! Write a vector to MPack.
 *
 * @param writer MPack writer.
 * @param vector Vector to write.
 */
template <typename T>
inline void write_vector(mpack_writer_t* writer, const T& vector) {
  for (Eigen::Index i = 0; i < vector.size(); ++i) {
    write<double>(writer, vector(i));
  }
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const Eigen::Vector2d& value) {
  mpack_start_array(writer, 2);
  write_vector(writer, value);
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const Eigen::Vector3d& value) {
  mpack_start_array(writer, 3);
  write_vector(writer, value);
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const Eigen::VectorXd& value) {
  mpack_start_array(writer, static_cast<size_t>(value.size()));
  write_vector(writer, value);
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const Eigen::Quaterniond& value) {
  mpack_start_array(writer, 4);
  write<double>(writer, value.w());
  write<double>(writer, value.x());
  write<double>(writer, value.y());
  write<double>(writer, value.z());
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const Eigen::Matrix3d& value) {
  mpack_start_array(writer, 9);
  write_matrix(writer, value);
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer,
                  const std::vector<std::string>& value) {
  mpack_start_array(writer, value.size());
  for (const std::string& str : value) {
    mpack_write_str(writer, str.c_str(), static_cast<uint32_t>(str.size()));
  }
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer, const std::vector<double>& value) {
  mpack_start_array(writer, value.size());
  for (const double& v : value) {
    write<double>(writer, v);
  }
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer,
                  const std::vector<Eigen::Vector2d>& value) {
  mpack_start_array(writer, value.size());
  for (const Eigen::Vector2d& vec : value) {
    write(writer, vec);
  }
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer,
                  const std::vector<Eigen::Vector3d>& value) {
  mpack_start_array(writer, value.size());
  for (const Eigen::Vector3d& vec : value) {
    write(writer, vec);
  }
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer,
                  const std::vector<Eigen::VectorXd>& value) {
  mpack_start_array(writer, value.size());
  for (const Eigen::VectorXd& vec : value) {
    write(writer, vec);
  }
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer,
                  const std::vector<Eigen::Quaterniond>& value) {
  mpack_start_array(writer, value.size());
  for (const Eigen::Quaterniond& quaternion : value) {
    write(writer, quaternion);
  }
  mpack_finish_array(writer);
}

//! Specialization of write<T>(writer, value)
template <>
inline void write(mpack_writer_t* writer,
                  const std::vector<Eigen::Matrix3d>& value) {
  mpack_start_array(writer, value.size());
  for (const Eigen::Matrix3d& matrix : value) {
    write(writer, matrix);
  }
  mpack_finish_array(writer);
}

}  // namespace palimpsest::mpack
