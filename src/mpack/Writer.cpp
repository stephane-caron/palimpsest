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

#include "palimpsest/mpack/Writer.h"

#include <mpack.h>

#include <string>
#include <vector>

/*! Intrusive flush function for std::vector.
 *
 * This is an intrusive flush function which modifies the writer's buffer in
 * response to a flush instead of emptying it in order to add more capacity for
 * data. This removes the need to copy data from a fixed buffer into a growable
 * one, improving performance.
 *
 * There are three ways flush can be called:
 * - flushing the buffer during writing (used is zero, count is all data, data
 *   is buffer)
 * - flushing extra data during writing (used is all flushed data, count is
 *   extra data, data is not buffer)
 * - flushing during teardown (used and count are both all flushed data, data
 *   is buffer)
 *
 * In the first two cases, we grow the buffer by at least double, enough
 * to ensure that new data will fit. We ignore the teardown flush.
 *
 * @note Inspired by mpack.c @ version 1.0
 */
static void mpack_std_vector_writer_flush(mpack_writer_t *writer,
                                          const char *data, size_t count) {
  auto &buffer = *static_cast<std::vector<char> *>(writer->context);

  if (data == writer->buffer) {
    if (mpack_writer_buffer_used(writer) == count) {
      // teardown, do nothing
      return;
    }
    // otherwise leave the data in the buffer and just grow
    writer->current = writer->buffer + count;
    count = 0;
  }

  size_t used = mpack_writer_buffer_used(writer);
  size_t size = mpack_writer_buffer_size(writer);

  mpack_log("flush size %i used %i data %p buffer %p\n",
            static_cast<int>(count), static_cast<int>(used), data,
            writer->buffer);

  mpack_assert(
      data == writer->buffer || used + count > size,
      "extra flush for %i but there is %i space left in the buffer! (%i/%i)",
      static_cast<int>(count),
      static_cast<int>(mpack_writer_buffer_left(writer)),
      static_cast<int>(used), static_cast<int>(size));

  // grow to fit the data
  // TODO(gergondet): this really needs to correctly test for overflow
  size_t new_size = size * 2;
  while (new_size < used + count) {
    new_size *= 2;
  }

  mpack_log("flush growing buffer size from %i to %i\n", static_cast<int>(size),
            static_cast<int>(new_size));

  // grow the buffer
  buffer.resize(new_size);
  char *new_buffer = buffer.data();
  if (new_buffer == NULL) {
    mpack_writer_flag_error(writer, mpack_error_memory);
    return;
  }
  writer->current = new_buffer + used;
  writer->buffer = new_buffer;
  writer->end = writer->buffer + new_size;

  // append the extra data
  if (count > 0) {
    mpack_memcpy(writer->current, data, count);
    writer->current += count;
  }

  mpack_log("new buffer %p, used %i\n", new_buffer,
            static_cast<int>(mpack_writer_buffer_used(writer)));
}

namespace palimpsest::mpack {

Writer::Writer(std::vector<char> &buffer) {
  if (buffer.size() == 0) {
    buffer.resize(MPACK_BUFFER_SIZE);
  }
  mpack_writer_init(&writer_, buffer.data(), buffer.size());
  mpack_writer_set_context(&writer_, &buffer);  // used in flush function
  mpack_writer_set_flush(&writer_, mpack_std_vector_writer_flush);
}

Writer::~Writer() {}

void Writer::write(bool b) { mpack_write_bool(&writer_, b); }

void Writer::write(int8_t i) { mpack_write_i8(&writer_, i); }

void Writer::write(int16_t i) { mpack_write_i16(&writer_, i); }

void Writer::write(int32_t i) { mpack_write_i32(&writer_, i); }

void Writer::write(int64_t i) { mpack_write_i64(&writer_, i); }

void Writer::write(uint8_t i) { mpack_write_u8(&writer_, i); }

void Writer::write(uint16_t i) { mpack_write_u16(&writer_, i); }

void Writer::write(uint32_t i) { mpack_write_u32(&writer_, i); }

void Writer::write(uint64_t i) { mpack_write_u64(&writer_, i); }

void Writer::write(float f) { mpack_write_float(&writer_, f); }

void Writer::write(double d) { mpack_write_double(&writer_, d); }

void Writer::write(const std::string &s) {
  mpack_write_str(&writer_, s.c_str(), static_cast<uint32_t>(s.size()));
}

void Writer::write(const char *s) { mpack_write_cstr(&writer_, s); }

namespace {

template <typename T>
inline void write_vector(mpack_writer_t *writer, const T &v) {
  for (Eigen::Index i = 0; i < v.size(); ++i) {
    mpack_write_double(writer, v(i));
  }
}

template <typename T>
inline void write_matrix(mpack_writer_t *writer, const T &m) {
  for (Eigen::Index i = 0; i < m.rows(); ++i) {
    for (Eigen::Index j = 0; j < m.cols(); ++j) {
      mpack_write_double(writer, m(i, j));
    }
  }
}

}  // namespace

void Writer::write(const Eigen::Vector2d &v) {
  start_array(2);
  write_vector(&writer_, v);
  finish_array();
}

void Writer::write(const Eigen::Vector3d &v) {
  start_array(3);
  write_vector(&writer_, v);
  finish_array();
}

void Writer::write(const Eigen::VectorXd &v) {
  start_array(static_cast<size_t>(v.size()));
  write_vector(&writer_, v);
  finish_array();
}

void Writer::write(const Eigen::Quaterniond &q) {
  start_array(4);
  write(q.w());
  write(q.x());
  write(q.y());
  write(q.z());
  finish_array();
}

void Writer::write(const Eigen::Matrix3d &m) {
  start_array(9);
  write_matrix(&writer_, m);
  finish_array();
}

void Writer::start_array(size_t s) {
  mpack_start_array(&writer_, static_cast<uint32_t>(s));
}

void Writer::finish_array() { mpack_finish_array(&writer_); }

void Writer::start_map(size_t s) {
  mpack_start_map(&writer_, static_cast<uint32_t>(s));
}

void Writer::finish_map() { mpack_finish_map(&writer_); }

size_t Writer::finish() {
  if (mpack_writer_destroy(&writer_) != mpack_ok) {
    mpack_log("Failed to write to MessagePack");
    return 0;
  }
  return mpack_writer_buffer_used(&writer_);
}

}  // namespace palimpsest::mpack
