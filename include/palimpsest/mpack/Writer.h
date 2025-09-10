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

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <array>
#include <exception>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

//! Internal namespace for writing to MessagePack buffers
namespace palimpsest::mpack {

/*! Write MessagePack (using MPack's Write API) to a vector of bytes.
 *
 * Writers assume they are given ownership of the bytes buffer. In particular,
 * they may resize it dynamically as needed.
 *
 * @note After a message has been fully written, the writer instance must be
 * discarded to create a new message.
 */
class Writer {
 public:
  /*! Constructor.
   *
   * @param buffer Buffer used to store the data. It may grow if needed.
   */
  explicit Writer(std::vector<char> &buffer);

  //! Destructor.
  ~Writer();

  /*! Add data to the MessagePack (basic)
   *
   * These overload set allows to write basic data to the MessagePack
   */

  //! Write a bool.
  void write(bool b);

  //! Write an int8_t.
  void write(int8_t i);

  //! Write an int16_t.
  void write(int16_t i);

  //! Write an int32_t.
  void write(int32_t i);

  //! Write an int64_t.
  void write(int64_t i);

  //! Write a uint8_t.
  void write(uint8_t i);

  //! Write a uint16_t.
  void write(uint16_t i);

  //! Write a uint32_t.
  void write(uint32_t i);

  //! Write a uint64_t.
  void write(uint64_t i);

  //! Write a float.
  void write(float f);

  //! Write a double.
  void write(double d);

  //! Write an std::string.
  void write(const std::string &s);

  /*! Write a C-style string.
   *
   * @param[in] s C-style string.
   */
  void write(const char *s);

  /*! Write an Eigen::Vector2d.
   *
   * @param[in] v Two-dimensional vector.
   *
   * Serializes as an array of size 2.
   */
  void write(const Eigen::Vector2d &v);

  /*! Write an Eigen::Vector3d.
   *
   * @param[in] v Three-dimensional vector.
   *
   * Serializes as an array of size 3.
   */
  void write(const Eigen::Vector3d &v);

  /*! Write an Eigen::VectorXd.
   *
   * @param[in] v Vector.
   *
   * Serializes as an array of size X.
   */
  void write(const Eigen::VectorXd &v);

  /*! Write an Eigen::Quaterniond.
   *
   * @param[in] q Quaternion.
   *
   * Serializes as an array of size X.
   */
  void write(const Eigen::Quaterniond &q);

  /*! Write an Eigen::Matrix3d.
   *
   * @param[in] m Three-by-three matrix.
   *
   * Serializes as an array of size 9.
   */
  void write(const Eigen::Matrix3d &m);

  /*! Add data to the MessagePack (containers).
   *
   * These functions support the serialization of standard containers of
   * serializable objects.
   */

  //! Write an std::vector<T, A>.
  template <typename T, typename A>
  void write(const std::vector<T, A> &v) {
    start_array(v.size());
    for (const auto &value : v) {
      write(value);
    }
    finish_array();
  }

  //! Write an std::array<T, N>.
  template <typename T, std::size_t N>
  void write(const std::array<T, N> &a) {
    start_array(N);
    for (const auto &value : a) {
      write(value);
    }
    finish_array();
  }

  //! Write an std::pair<T1, T2>.
  template <typename T1, typename T2>
  void write(const std::pair<T1, T2> &p) {
    start_array(2);
    write(p.first);
    write(p.second);
    finish_array();
  }

  //! Write an std::map<KeyT, T, C, A>.
  template <typename KeyT, typename T, typename C, typename A>
  void write(const std::map<KeyT, T, C, A> &m) {
    start_map(m.count());
    for (const auto &p : m) {
      write(p.first);
      write(p.second);
    }
    finish_map();
  }

  //! Write an std::set<T, C, A>.
  template <typename T, typename C, typename A>
  void write(const std::set<T, C, A> &s) {
    start_array(s.size());
    for (const auto &value : s) {
      write(value);
    }
    finish_array();
  }

  //! Write an std::tuple<Args...>.
  template <typename... Args>
  void write(const std::tuple<Args...> &t) {
    start_array(sizeof...(Args));
    write_impl<0>(t);
    finish_array();
  }

  /*! Start serializing an array.
   *
   * @param[in] size Size of the array.
   */
  void start_array(size_t size);

  //! Finished serializing an array.
  void finish_array();

  /*! Start serializing a map.
   *
   * @param[in] size Size of the map, the map must then contains 2 * size
   *     elements.
   */
  void start_map(size_t size);

  //! Finished serializing a map.
  void finish_map();

  /*! Finish building the message. Data cannot be appended afterwards.
   *
   * @return Effective size of MessagePack data. Note that ``buffer.size()`` is
   *     likely different.
   */
  size_t finish();

  //! Get pointer to the MPack writer for use with the C API.
  mpack_writer_t *mpack_writer() { return &writer_; }

 private:
  //! Internal MPack writer.
  mpack_writer_t writer_;

  //! Internal write function for tuples.
  template <size_t i, typename... Args,
            typename std::enable_if<i<sizeof...(Args), int>::type = 0> void
                write_impl(const std::tuple<Args...> &t) {
    write(std::get<i>(t));
    write_impl<i + 1>(t);
  }

  //! Internal write function for tuples.
  template <size_t i, typename... Args,
            typename std::enable_if<i >= sizeof...(Args), int>::type = 0>
  void write_impl(const std::tuple<Args...> &) {}
};

}  // namespace palimpsest::mpack
