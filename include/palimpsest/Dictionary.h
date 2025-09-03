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

#include <spdlog/spdlog.h>

#include <Eigen/Core>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

/*! Dictionary of values and sub-dictionaries.
 *
 * The grammar here is a subset of e.g. JSON or YAML: a dictionary maps keys to
 * either values (number, string, std::vector, Eigen::MatrixXd, ...) or other
 * dictionaries.
 *
 * This type allows us to store and retrieve C++ objects as follows:
 *
 * @code{cpp}
 * Dictionary dict;
 * // create a vector of 4 double-precision numbers with value 42
 * dict.insert<std::vector<double>>("TheAnswer", 4, 42);
 *
 * auto & answer = dict.get<std::vector<double>>("TheAnswer");
 * answer[3] = 0;  // manipulate the object directly
 *
 * // Get another reference to the object
 * auto &same_answer = dict.get<std::vector<double>>("TheAnswer");
 * same_answer.push_back(0);
 * spdlog::info(answer.size());  // vector now has size 5
 * @endcode
 *
 * When retrieving an object using get<T>, checks are performed to ensure that
 * the value type and T are compatible. Hence, once an object is inserted in
 * the dictionary, it is not meant to change type later on.
 *
 * To handle inheritance, we need to explicitely recall the class hierarchy:
 *
 * @code{cpp}
 * struct A {};
 * struct B : public A {};
 *
 * // Creating an inherited object and checking virtual inheritance
 * dict.insert<B, A>("foo");
 * auto & base = dict.get<A>("foo");
 * auto & derived = dict.get<B>("foo");
 * @endcode
 *
 * @note Dictionaries are move-only.
 *
 * @note We are cheating on the class name a bit: a "dictionary" is actually
 * either empty, or a single value, or a map of key-"dictionary" pairs. If that
 * helps, for OCaml-lovers:
 *
 * @code{pseudocaml}
 * type Dictionary =
 *     | Empty
 *     | Value of Dictionary::Value
 *     | Map of (std::string -> Dictionary) map
 * @endcode
 *
 * This is practical because the root is always an actual dictionary (value
 * insertion in the tree is performed by parents on their children), so that
 * the type we see when declaring ``Dictionary foo;`` is right. The downside is
 * that the type of ``foo("var")`` is still Dictionary; it only becomes its
 * proper value type after an explicit conversion ``foo("bar").as<T>()`` or
 * ``foo.get<T>("bar")``, or an implicit conversion ``T& bar = foo("bar");``.
 */
class Dictionary {
  /*! Internal wrapper around an object and its type information.
   *
   * @note Dictionary values are move-only.
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
                            "\" but is being cast to type \"" +
                            typeid(T).name() + "\".");
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

 public:
  //! Default constructor
  Dictionary() = default;

  //! No copy constructor
  Dictionary(const Dictionary &) = delete;

  //! No copy assignment operator
  Dictionary &operator=(const Dictionary &) = delete;

  //! Default move constructor
  Dictionary(Dictionary &&) = default;

  //! Default move assignment operator
  Dictionary &operator=(Dictionary &&) = default;

  /*! Default destructor.
   *
   * @note Child dictionaries will be recursively destroyed as they are held by
   * unique pointers in an unordered_map;
   */
  ~Dictionary() = default;

  //! We are a (potentially empty) map if and only if the value is empty.
  bool is_map() const noexcept { return (value_.buffer == nullptr); }

  //! We are empty if and only if we are a dictionary with no element.
  bool is_empty() const noexcept { return (is_map() && map_.size() < 1); }

  //! We are a value if and only if the internal value is non-empty.
  bool is_value() const noexcept { return (value_.buffer != nullptr); }

  /*! Check whether a key is in the dictionary.
   *
   * @param[in] key Key to look for.
   * @return true when the key is in the dictionary.
   */
  bool has(const std::string &key) const noexcept {
    return (map_.find(key) != map_.end());
  }

  //! Return the list of keys of the dictionary.
  std::vector<std::string> keys() const noexcept;

  //! Return the number of keys in the dictionary.
  unsigned size() const noexcept { return map_.size(); }

  /*! Get reference to the internal value.
   *
   * @return Reference to the object.
   *
   * @throw TypeError if the dictionary is not a value, or it is but the stored
   *     value type is not T.
   */
  template <typename T>
  T &as() {
    if (!this->is_value()) {
      throw TypeError(__FILE__, __LINE__, "Object is not a value.");
    }
    return value_.get_reference<T>();
  }

  /*! Const variant of @ref as.
   *
   * @return Reference to the object.
   *
   * @throw TypeError if the stored object type is not T.
   */
  template <typename T>
  const T &as() const {
    if (!this->is_value()) {
      throw TypeError(__FILE__, __LINE__, "Object is not a value.");
    }
    return const_cast<const T &>(value_.get_reference<T>());
  }

  /*! Get reference to the object at a given key.
   *
   * @param[in] key Key to the object.
   * @return Reference to the object.
   *
   * @throw KeyError if there is no object at this key.
   * @throw TypeError if there is an object at this key, but its type is not T.
   */
  template <typename T>
  T &get(const std::string &key) {
    return const_cast<T &>(get_<T>(key));
  }

  /*! Const variant of @ref get.
   *
   * @param[in] key Key to the object.
   * @return Reference to the object.
   *
   * @throw KeyError if there is no object at this key.
   * @throw TypeError if there is an object at this key, but its type is not T.
   */
  template <typename T>
  const T &get(const std::string &key) const {
    return get_<T>(key);
  }

  /*! Get object at a given key if it exists, or a default value otherwise.
   *
   * @param[in] key Key to look for.
   * @param[in] default_value Default value used if there is no value at this
   * key.
   * @return Reference to the object if it exists, default_value otherwise.
   *
   * @throw TypeError if the object at this key is not a value, or it is but
   *     its type does is not T.
   */
  template <typename T>
  const T &get(const std::string &key, const T &default_value) const {
    auto it = map_.find(key);
    if (it != map_.end()) {
      if (it->second->is_map()) {
        throw TypeError(__FILE__, __LINE__,
                        "Object at key \"" + key +
                            "\" is a dictionary, cannot get a single value "
                            "from it. Did you "
                            "mean to use operator()?");
      }
      try {
        return it->second->value_.get_reference<T>();
      } catch (const TypeError &e) {
        throw TypeError(
            __FILE__, __LINE__,
            "Object for key \"" + key +
                "\" does not have the same type as the stored type. Stored " +
                it->second->value_.type_name() + " but requested " +
                typeid(T).name() + ".");
      }
    }
    return default_value;
  }

  /*! Create an object at a given key and return a reference to it. If there is
   * already a value at this key, return the existing object instead.
   *
   * @param[in] key Key to create the object at.
   * @param args Parameters passed to the object's constructor.
   * @return Reference to the constructed object.
   *
   * @throw TypeError if the dictionary is not a map, and therefore we cannot
   *     insert at a given key inside it.
   *
   * @note To STL practitioners: although it is named like e.g.
   * unordered_map::insert, this function behaves like unordered_map::emplace
   * as it forwards its argument to the constructor T::T() called internally.
   * Also it doesn't return an insertion confirmation boolean.
   */
  template <typename T, typename... ArgsT, typename... Args>
  T &insert(const std::string &key, Args &&...args) {
    if (this->is_value()) {
      throw TypeError(__FILE__, __LINE__,
                      "Cannot insert at key \"" + key +
                          "\" in non-dictionary object of type \"" +
                          value_.type_name() + "\".");
    }
    auto &child = this->operator()(key);
    if (!child.is_empty()) {
      spdlog::warn(
          "[Dictionary::insert] Key \"{}\" already exists. Returning existing "
          "value rather than creating a new one.",
          key);
      return get<T>(key);
    }
    child.value_.allocate<T>();
    new (child.value_.buffer.get()) T(std::forward<Args>(args)...);
    T &ret = child.value_.setup<T, ArgsT...>();
    return ret;
  }

  /*! Assign value directly.
   *
   * @param[in] new_value New value to assign.
   *
   * @throw TypeError if the object was already a value of a different type.
   *
   * If the object was a dictionary, all entries are cleared and it becomes a
   * value. If a previous value is already present, it will be assigned (not
   * reallocated), therefore the new value needs to have the same type.
   */
  template <typename T>
  Dictionary &operator=(const T &new_value) {
    if (this->is_map()) {
      clear();
    }
    if (this->is_empty()) {
      become<T>(new_value);
      return *this;
    }
    auto &internal_value = value_.get_reference<T>();
    internal_value = new_value;
    return *this;
  }

  /*! Assignment operator for C-style strings.
   *
   * @param[in] c_string C-style string to assign.
   *
   * @throw TypeError if the object was already a value of a different type.
   *
   * This specialization avoids "invalid array assignment" errors. Note that
   * the string is cast to an std::string.
   */
  Dictionary &operator=(const char *c_string) {
    return operator= <std::string>(std::string(c_string));
  }

  /*! Remove a key-value pair from the dictionary.
   *
   * @param[in] key Key to remove.
   */
  void remove(const std::string &key) noexcept;

  //! Remove all entries from the dictionary.
  void clear() noexcept;

  /*! Return a reference to the dictionary at key, performing an insertion if
   * such a key does not already exist.
   *
   * @param[in] key Key to look at.
   * @return Reference to the new dictionary at this key if there was none, or
   *     to the existing dictionary otherwise.
   *
   * @throw TypeError if the dictionary is not a map, and therefore we cannot
   *     look up a key from it.
   *
   * @note The behavior of this operator is the same as
   * std::unordered_map::operator[]. It differs from that of Python
   * dictionaries, where an exception is throw if the key doesn't exist.
   *
   * @note The reason why we use operator() instead of operator[] is that the
   * class includes user-defined conversion functions to value types, so that
   * we can write:
   *
   * @code{cpp}
   * Eigen::Vector3d& position = dict("position");
   * auto& position = dict("position").as<Eigen::Vector3d>();  // equivalent
   * auto& position = dict.get<Eigen::Vector3d>("position");   // equivalent
   * @endcode
   *
   * With operator[], these conversions would be ambiguous as [] is commutative
   * in C (c_str[int] == *(c_str + int) == int[c_str]).
   */
  Dictionary &operator()(const std::string &key);

  /*! Return a reference to the dictionary at key, performing an insertion if
   * such a key does not already exist.
   *
   * @param[in] key Key to look at.
   * @return Reference to the dictionary at this key.
   *
   * @throw KeyError if there is no object at this key.
   * @throw TypeError if the dictionary is not a map, and therefore we cannot
   *     lookup a key from it.
   *
   * Since we cannot insert a new element in a const object, this const
   * operator will throw if the key is not already in the dictionary. See the
   * documentation for the non-const variant of this operator.
   */
  const Dictionary &operator()(const std::string &key) const;

  /*! Serialize to raw MessagePack data.
   *
   * @param[out] buffer Buffer that will hold the message data.
   * @return Size of the message. Note that it is not the same as
   *     the size of the buffer after execution.
   */
  size_t serialize(std::vector<char> &buffer) const;

  /*! Write MessagePack serialization to a binary file.
   *
   * @param[in] filename Path to the output file.
   */
  void write(const std::string &filename) const;

  /*! Update dictionary from a MessagePack binary file.
   *
   * @param[in] filename Path to the input file.
   */
  void read(const std::string &filename);

  /*! Update dictionary from another dictionary.
   *
   * @param[in] other Dictionary to update from.
   *
   * @throw TypeError if deserialized data types don't match those of the
   *     corresponding objects in the dictionary.
   */
  void update(const Dictionary &other);

  /*! Update dictionary from raw MessagePack data.
   *
   * @param[in] data Buffer to read MessagePack from.
   * @param[in] size Buffer size.
   *
   * @throw TypeError if deserialized data types don't match those of the
   *     corresponding objects in the dictionary.
   */
  void update(const char *data, size_t size);

  /*! Update existing values from an MPack node.
   *
   * @param[in] node MPack node.
   *
   * @throw TypeError if a deserialized object's type does not match the type
   *     of an existing entry in the dictionary.
   */
  void update(mpack_node_t node);

  /*! Compute the difference between this dictionary and another.
   *
   * @param[in] other Dictionary to compare with.
   * @return A new dictionary containing key-value pairs that are in this
   *     dictionary and either not in other, or in other but with different
   *     values.
   *
   * The returned dictionary maintains the nested structure of differences. For
   * nested dictionaries, only the differing sub-keys are included in the
   * result. Values are compared by serializing both objects and comparing the
   * serialized data.
   */
  Dictionary difference(const Dictionary &other) const;

  //! Allow implicit conversion to (bool &).
  operator bool &() { return this->as<bool>(); }

  //! Allow implicit conversion to (const bool &).
  operator const bool &() const { return this->as<bool>(); }

  //! Allow implicit conversion to (int8_t &).
  operator int8_t &() { return this->as<int8_t>(); }

  //! Allow implicit conversion to (const int8_t &).
  operator const int8_t &() const { return this->as<int8_t>(); }

  //! Allow implicit conversion to (int16_t &).
  operator int16_t &() { return this->as<int16_t>(); }

  //! Allow implicit conversion to (const int16_t &).
  operator const int16_t &() const { return this->as<int16_t>(); }

  //! Allow implicit conversion to (int32_t &).
  operator int32_t &() { return this->as<int32_t>(); }

  //! Allow implicit conversion to (const int32_t &).
  operator const int32_t &() const { return this->as<int32_t>(); }

  //! Allow implicit conversion to (int64_t &).
  operator int64_t &() { return this->as<int64_t>(); }

  //! Allow implicit conversion to (const int64_t &).
  operator const int64_t &() const { return this->as<int64_t>(); }

  //! Allow implicit conversion to (uint8_t &).
  operator uint8_t &() { return this->as<uint8_t>(); }

  //! Allow implicit conversion to (const uint8_t &).
  operator const uint8_t &() const { return this->as<uint8_t>(); }

  //! Allow implicit conversion to (uint16_t &).
  operator uint16_t &() { return this->as<uint16_t>(); }

  //! Allow implicit conversion to (const uint16_t &).
  operator const uint16_t &() const { return this->as<uint16_t>(); }

  //! Allow implicit conversion to (uint32_t &).
  operator uint32_t &() { return this->as<uint32_t>(); }

  //! Allow implicit conversion to (const uint32_t &).
  operator const uint32_t &() const { return this->as<uint32_t>(); }

  //! Allow implicit conversion to (uint64_t &).
  operator uint64_t &() { return this->as<uint64_t>(); }

  //! Allow implicit conversion to (const uint64_t &).
  operator const uint64_t &() const { return this->as<uint64_t>(); }

  //! Allow implicit conversion to (float &).
  operator float &() { return this->as<float>(); }

  //! Allow implicit conversion to (const float &).
  operator const float &() const { return this->as<float>(); }

  //! Allow implicit conversion to (double &).
  operator double &() { return this->as<double>(); }

  //! Allow implicit conversion to (const double &).
  operator const double &() const { return this->as<double>(); }

  //! Allow implicit conversion to (std::string &).
  operator std::string &() { return this->as<std::string>(); }

  //! Allow implicit conversion to (const std::string &).
  operator const std::string &() const { return this->as<std::string>(); }

  //! Allow implicit conversion to (Eigen::Vector2d &).
  operator Eigen::Vector2d &() { return this->as<Eigen::Vector2d>(); }

  //! Allow implicit conversion to (const Eigen::Vector2d &).
  operator const Eigen::Vector2d &() const {
    return this->as<Eigen::Vector2d>();
  }

  //! Allow implicit conversion to (Eigen::Vector3d &).
  operator Eigen::Vector3d &() { return this->as<Eigen::Vector3d>(); }

  //! Allow implicit conversion to (const Eigen::Vector3d &).
  operator const Eigen::Vector3d &() const {
    return this->as<Eigen::Vector3d>();
  }

  //! Allow implicit conversion to (Eigen::VectorXd&).
  operator Eigen::VectorXd &() { return this->as<Eigen::VectorXd>(); }

  //! Allow implicit conversion to (const Eigen::VectorXd&).
  operator const Eigen::VectorXd &() const {
    return this->as<Eigen::VectorXd>();
  }

  //! Allow implicit conversion to (Eigen::Quaterniond &).
  operator Eigen::Quaterniond &() { return this->as<Eigen::Quaterniond>(); }

  //! Allow implicit conversion to (const Eigen::Quaterniond &).
  operator const Eigen::Quaterniond &() const {
    return this->as<Eigen::Quaterniond>();
  }

  //! Allow implicit conversion to (Eigen::Matrix3d &).
  operator Eigen::Matrix3d &() { return this->as<Eigen::Matrix3d>(); }

  //! Allow implicit conversion to (const Eigen::Matrix3d &).
  operator const Eigen::Matrix3d &() const {
    return this->as<Eigen::Matrix3d>();
  }

  /*! Output stream operator for printing.
   *
   * @param[out] stream Output stream.
   * @param[in] dict Dictionary to print.
   * @return Updated output stream.
   */
  friend std::ostream &operator<<(std::ostream &stream, const Dictionary &dict);

 protected:
  template <typename T, typename... ArgsT, typename... Args>
  void become(Args &&...args) {
    assert(this->is_empty());
    value_.allocate<T>();
    new (value_.buffer.get()) T(std::forward<Args>(args)...);
    value_.setup<T, ArgsT...>();
  }

 private:
  /*! Get a const reference to the object at a given key.
   *
   * @param[in] key Key to the object.
   * @return Const reference to the object.
   *
   * @throw KeyError if there is no object at this key.
   * @throw TypeError if there is an object at this key, but its type is not T.
   */
  template <typename T>
  const T &get_(const std::string &key) const {
    const auto &child_value = get_child_value_(key);
    try {
      return child_value.get_reference<T>();
    } catch (const TypeError &e) {
      throw TypeError(__FILE__, __LINE__,
                      "Object at key \"" + key + "\" has type \"" +
                          child_value.type_name() +
                          "\", but is being cast to type \"" +
                          typeid(T).name() + "\".");
    }
  }

  /*! Get a const reference to the child value at a given key.
   *
   * @param[in] key Key to the object.
   * @return Const reference to the value wrapper.
   *
   * @throw KeyError if there is no object at this key.
   * @throw TypeError if there is an object at this key but it is not a value.
   */
  const Value &get_child_value_(const std::string &key) const;

  /*! Deserialize an MPack value at a given key.
   *
   * @param[in] key Key to store the deserialized object at.
   * @param[in] value MPack value to deserialize.
   *
   * @throw TypeError if the type of the deserialized object cannot be handled.
   */
  void insert_at_key_(const std::string &key, const mpack_node_t &value);

  /*! Serialize to a MessagePack writer.
   *
   * @param[out] writer Writer to serialize to.
   */
  void serialize_(mpack::Writer &writer) const;

 protected:
  //! Internal value, used if we are a value.
  Value value_;

  //! Key-value map, used if we are a map.
  std::unordered_map<std::string, std::unique_ptr<Dictionary>> map_;
};

}  // namespace palimpsest

namespace fmt {

//! Dictionary formatter.
template <>
struct formatter<palimpsest::Dictionary> : public formatter<string_view> {
  template <typename FormatContext>
  auto format(const palimpsest::Dictionary &dict,
              FormatContext &ctx) -> decltype(ctx.out()) {
    std::ostringstream oss;
    oss << dict;
    return formatter<string_view>::format(oss.str(), ctx);
  }
};

}  // namespace fmt
