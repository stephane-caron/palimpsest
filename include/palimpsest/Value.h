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

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>

#include "palimpsest/exceptions/TypeError.h"
#include "palimpsest/internal/Allocator.h"
#include "palimpsest/internal/is_valid_hash.h"
#include "palimpsest/internal/type_name.h"
#include "palimpsest/json/write.h"
#include "palimpsest/mpack/Writer.h"
#include "palimpsest/mpack/read.h"
#include "palimpsest/mpack/write.h"

namespace palimpsest {

using exceptions::TypeError;

/*! Internal wrapper around an object and its type information.
 *
 * @note Values are move-only.
 */
class Value {
 public:
  //! Default constructor.
  Value() = default;

  //! No copy constructor.
  Value(const Value &) = delete;

  //! No copy assignment operator.
  Value &operator=(const Value &) = delete;

  //! Default move constructor.
  Value(Value &&) = default;

  //! Default move assignment operator.
  Value &operator=(Value &&) = default;

  //! Destruct the object and free the internal buffer.
  ~Value() {
    if (this->buffer) {
      destroy_(*this);
    }
  }

  //! Allocate the internal buffer.
  template <typename T>
  void allocate() {
    this->buffer.reset(
        reinterpret_cast<uint8_t *>(internal::Allocator<T>().allocate(1)));
  }

  /*! Update value from an MPack node.
   *
   * @param[in] node MPack tree node.
   * @throw TypeError if the deserialized type does not match.
   */
  void deserialize(mpack_node_t node) { deserialize_(*this, node); }

  /*! Print value to an output stream;
   *
   * @param[out] stream Output stream to print to.
   */
  void print(std::ostream &stream) const { print_(*this, stream); }

  /*! Serialize value to a MessagePack writer.
   *
   * @param[out] writer Writer to serialize to.
   */
  void serialize(mpack::Writer &writer) const {
    serialize_(*this, writer.mpack_writer());
  }

  /*! Allocate object and register internal functions.
   *
   * @return Reference to allocated object.
   */
  template <typename T, typename... ArgsT>
  T &setup() {
    this->type_name = &internal::type_name<T>;
    this->same = &internal::is_valid_hash<T, ArgsT...>;
    deserialize_ = [](Value &self, mpack_node_t node) {
      T *cast_buffer = reinterpret_cast<T *>(self.buffer.get());
      mpack::read<T>(node, *cast_buffer);
    };
    destroy_ = [](Value &self) {
      T *p = reinterpret_cast<T *>(self.buffer.release());
      p->~T();
      internal::Allocator<T>().deallocate(p, 1);
    };
    print_ = [](const Value &self, std::ostream &stream) {
      const T *cast_buffer = reinterpret_cast<const T *>(self.buffer.get());
      json::write<T>(stream, *cast_buffer);
    };
    serialize_ = [](const Value &self, mpack_writer_t *writer) {
      const T *cast_buffer = reinterpret_cast<const T *>(self.buffer.get());
      mpack::write<T>(writer, *cast_buffer);
    };
    return *(reinterpret_cast<T *>(this->buffer.get()));
  }

  /*! Cast value to its object's type after checking that it matches T.
   *
   * @param[out] value Value to cast.
   *
   * @throw TypeError if the object's type does not match T.
   */
  template <typename T>
  T &get_reference() const {
    if (!this->same(typeid(T).hash_code())) {
      std::string cast_type = this->type_name();
      throw TypeError(__FILE__, __LINE__,
                      "Object has type \"" + cast_type +
                          "\" but is being cast to type \"" + typeid(T).name() +
                          "\".");
    }
    return *(reinterpret_cast<T *>(this->buffer.get()));
  }

 public:
  //! Internal buffer that holds the actual object.
  std::unique_ptr<uint8_t[]> buffer = nullptr;

  //! Function returning the name of the object's type.
  const char *(*type_name)();

  //! Function that checks if a given type matches the object's type.
  bool (*same)(std::size_t);

 private:
  //! Function that updates the value from a MessagePack node.
  void (*deserialize_)(Value &, mpack_node_t);

  //! Function that destructs the object and frees the internal buffer.
  void (*destroy_)(Value &);

  //! Function that prints the value to an output stream.
  void (*print_)(const Value &, std::ostream &);

  //! Function that serializes the value to a MessagePack writer.
  void (*serialize_)(const Value &, mpack_writer_t *);
};

}  // namespace palimpsest
