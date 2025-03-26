// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 Stéphane Caron
// Copyright 2025 Inria

#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace palimpsest::json {

/*
 * Internal templated functions to serialize values as JSON to a standard output
 * stream.
 */

/*! Write a value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 *
 * @note This is the non-specialized version of this function.
 */
template <typename T>
void write(std::ostream &stream, const T &value) {
  auto type_name = std::string("<typeid:") + typeid(T).name() + ">";
  stream << type_name;
}

/*! Write a boolean value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const bool &value) {
  stream << (value ? "true" : "false");
}

/*! Write an integral value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const int8_t &value) {
  stream << value;
}

/*! Write an integral value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const int16_t &value) {
  stream << value;
}

/*! Write an integral value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const int32_t &value) {
  stream << value;
}

/*! Write an integral value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const int64_t &value) {
  stream << value;
}

/*! Write an unsigned integral value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const uint8_t &value) {
  stream << value;
}

/*! Write an unsigned integral value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const uint16_t &value) {
  stream << value;
}

/*! Write an unsigned integral value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const uint32_t &value) {
  stream << value;
}

/*! Write an unsigned integral value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const uint64_t &value) {
  stream << value;
}

/*! Write a single-precision floating-point value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const float &value) {
  stream << value;
}

/*! Write a double-precision floating-point value as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const double &value) {
  stream << value;
}

/*! Write a string as JSON to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] value Value to write.
 */
template <>
inline void write(std::ostream &stream, const std::string &value) {
  stream << "\"" << value << "\"";
}

/*! Write a 2D vector as a JSON array to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] vector Vector to write.
 */
template <>
inline void write(std::ostream &stream, const Eigen::Vector2d &vector) {
  stream << "[" << vector.x() << ", " << vector.y() << "]";
}

/*! Write a 3D vector as a JSON array to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] vector Vector to write.
 */
template <>
inline void write(std::ostream &stream, const Eigen::Vector3d &vector) {
  stream << "[" << vector.x() << ", " << vector.y() << ", " << vector.z()
         << "]";
}

/*! Write a vector as a JSON array to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] vector Vector to write.
 */
template <>
inline void write(std::ostream &stream, const Eigen::VectorXd &vector) {
  stream << "[";
  for (Eigen::Index i = 0; i < vector.size(); ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << vector(i);
  }
  stream << "]";
}

/*! Write a quaternion as a JSON array to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] quat Quaternion.
 */
template <>
inline void write(std::ostream &stream, const Eigen::Quaterniond &quat) {
  stream << "[" << quat.w() << ", " << quat.x() << ", " << quat.y() << ", "
         << quat.z() << "]";
}

/*! Write a 3x3 matrix as nested JSON arrays to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] quat Quaternion.
 */
template <>
inline void write(std::ostream &stream, const Eigen::Matrix3d &matrix) {
  stream << "[";
  for (Eigen::Index i = 0; i < matrix.rows(); ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "[";
    for (Eigen::Index j = 0; j < matrix.cols(); ++j) {
      if (j > 0) {
        stream << ", ";
      }
      stream << matrix(i, j);
    }
    stream << "]";
  }
  stream << "]";
}

/*! Write a standard vector as a JSON array to an output stream.
 *
 * @param[out] stream Output stream.
 * @param[in] vector Standard vector to write.
 */
template <typename T>
inline void write(std::ostream &stream, const std::vector<T> &vector) {
  if (vector.empty()) {
    return;
  }
  stream << "[";
  for (const auto &it = vector.begin(); it != vector.end(); ++it) {
    if (it != vector.begin()) {
      stream << ", ";
    }
    stream << *it;
  }
  stream << "]";
}

}  // namespace palimpsest::json
