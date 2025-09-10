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

#include "palimpsest/Value.h"
#include "palimpsest/exceptions/TypeError.h"
#include "palimpsest/json/write.h"
#include "palimpsest/mpack/Writer.h"
#include "palimpsest/mpack/read.h"
#include "palimpsest/mpack/write.h"

//! Main library namespace
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
 * @code
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
 * @code
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
 *     | Value
 *     | Map of (std::string -> Dictionary)
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

  /*! Return the list of keys of the dictionary.
   *
   * @return Vector containing all keys in the dictionary.
   *
   * Example:
   * @code
   * Dictionary dict;
   * dict.set("temperature", 25.5);
   * dict.set("pressure", 101.3);
   * dict("sensors").set("count", 3);
   *
   * std::vector<std::string> keys = dict.keys();
   * for (const std::string& key : keys) {
   *   std::cout << "Key: " << key << std::endl;
   * }
   * @endcode
   */
  std::vector<std::string> keys() const noexcept;

  /*! Return an iterable view of the dictionary's (key, value) pairs.
   *
   * @return Vector of pairs containing key strings and references to
   *     Dictionary values.
   *
   * This function has the same semantics as Python's dict.items(). Each pair
   * contains the key as a string and a reference to the Dictionary at that
   * key. The Dictionary reference can be further used with as<T>() to get
   * typed values.
   *
   * Example:
   * @code
   * Dictionary dict;
   * dict.set("temperature", 25.5);
   * dict.set("pressure", 101.3);
   * dict("sensors").set("count", 3);
   *
   * for (const auto& [key, value_ref] : dict.items()) {
   *   const Dictionary& value = value_ref.get();
   *   std::cout << key << ": " << value << std::endl;
   * }
   * @endcode
   */
  std::vector<std::pair<std::string, std::reference_wrapper<const Dictionary>>>
  items() const noexcept;

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
   *
   * Example:
   * @code
   * Dictionary dict;
   * dict.set("temperature", 25.5);
   * double& temp = dict.get<double>("temperature");
   * temp = 30.0;  // Modifies the value in the dictionary
   * @endcode
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
   *
   * Example:
   * @code
   * const Dictionary dict;
   * const double& temp = dict.get<double>("temperature");
   * @endcode
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
   *
   * Example:
   * @code
   * Dictionary dict;
   * dict.set("count", 42);
   * int value = dict.get<int>("count", 0);     // Returns 42
   * int missing = dict.get<int>("missing", 0); // Returns 0 (default)
   * @endcode
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

  /*! If key is in the dictionary, return its value. If not, insert key with a
   * value of default_value and return default_value.
   *
   * @param[in] key Key to look for or insert.
   * @param[in] default_value Default value to insert and return if key doesn't
   * exist.
   * @return Reference to the value at key (either existing or newly inserted).
   *
   * @throw TypeError if the dictionary is not a map, or if there is already an
   *     object at this key but it is not a value, or it is but its type does
   *     not match T.
   *
   * @note This function has the same semantics as Python's dict.setdefault(key,
   * default). It has the same semantics as @ref insert, except that it does
   * not warn when returning an existing value.
   *
   * Example:
   * @code
   * Dictionary dict;
   * dict.set("temperature", 25.5);
   *
   * double& temp = dict.setdefault<double>("temperature", 20.0);  // 25.5
   * double& pressure = dict.setdefault<double>("pressure", 101.3); // 101.3
   * std::cout << dict.size() << std::endl;  // 2
   * @endcode
   */
  template <typename T>
  T &setdefault(const std::string &key, const T &default_value) {
    if (this->is_value()) {
      throw TypeError(__FILE__, __LINE__,
                      "Cannot insert at key \"" + key +
                          "\" in non-dictionary object of type \"" +
                          value_.type_name() + "\".");
    }
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
    auto &child = this->operator()(key);
    child.value_.allocate<T>();
    new (child.value_.buffer.get()) T(default_value);
    T &ret = child.value_.setup<T>();
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

  /*! Remove a key-value pair from the dictionary and return its value.
   *
   * @param[in] key Key to remove.
   * @return Value that was stored at the key.
   *
   * @throw KeyError if there is no object at this key.
   * @throw TypeError if there is an object at this key but it is not a value,
   *     or it is but its type does not match T.
   *
   * This function has the same semantics as Python's dict.pop(key).
   *
   * Example:
   * @code
   * Dictionary dict;
   * dict.set("temperature", 25.5);
   * dict.set("pressure", 101.3);
   *
   * double temp = dict.pop<double>("temperature");
   * std::cout << "Removed: " << temp << std::endl;  // 25.5
   * std::cout << "Size: " << dict.size() << std::endl;  // 1
   * @endcode
   */
  template <typename T>
  T pop(const std::string &key) {
    const T value = get<T>(key);
    remove(key);
    return value;
  }

  /*! Remove a key-value pair from the dictionary and return its value, or
   * return a default value if the key doesn't exist.
   *
   * @param[in] key Key to remove.
   * @param[in] default_value Default value to return if key doesn't exist.
   * @return Value that was stored at the key, or default_value if key doesn't
   *     exist.
   *
   * @throw TypeError if there is an object at this key but it is not a value,
   *     or it is but its type does not match T.
   *
   * This function has the same semantics as Python's dict.pop(key, default).
   *
   * Example:
   * @code
   * Dictionary dict;
   * dict.set("temperature", 25.5);
   *
   * double temp = dict.pop<double>("temperature", 20.0);  // 25.5
   * double missing = dict.pop<double>("missing", 20.0);   // 20.0
   * std::cout << "Size: " << dict.size() << std::endl;    // 0
   * @endcode
   */
  template <typename T>
  T pop(const std::string &key, const T &default_value) {
    auto it = map_.find(key);
    if (it == map_.end()) {
      return default_value;
    }
    if (it->second->is_map()) {
      throw TypeError(__FILE__, __LINE__,
                      "Object at key \"" + key +
                          "\" is a dictionary, cannot pop a single value "
                          "from it.");
    }
    try {
      const T value = it->second->value_.get_reference<T>();
      remove(key);
      return value;
    } catch (const TypeError &e) {
      throw TypeError(
          __FILE__, __LINE__,
          "Object for key \"" + key +
              "\" does not have the same type as the requested type. Stored " +
              it->second->value_.type_name() + " but requested " +
              typeid(T).name() + ".");
    }
  }

  /*! Remove and return a (key, value) pair from the dictionary.
   *
   * @return Pair containing the key string and the Dictionary value that was
   *     removed. The latter value can be further used with @ref as to recover
   *     a typed reference.
   *
   * @throw KeyError if the dictionary is empty.
   * @throw TypeError if the dictionary is not a map.
   *
   * @note Contrary to Python's dict.popitem(), this function does not
   * guarantee that pairs are returned in LIFO (last-in, first-out) order
   * (although they will likely be in practice).
   *
   * Example:
   * @code
   * Dictionary dict;
   * dict("temperature") = 25.5;
   * dict("pressure") = 101.3;
   *
   * auto [key, value] = dict.popitem();
   * std::cout << "Popped ('" << key << "', " << value.as<double>() << ")\n";
   * std::cout << "Size: " << dict.size() << std::endl;  // 1
   * @endcode
   */
  std::pair<std::string, Dictionary> popitem();

  /*! Remove all entries from the dictionary.
   *
   * Example:
   * @code
   * Dictionary dict;
   * dict.set("temperature", 25.5);
   * dict.set("pressure", 101.3);
   * std::cout << dict.size() << std::endl;  // 2
   * dict.clear();
   * std::cout << dict.size() << std::endl;  // 0
   * @endcode
   */
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
   * @code
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

  /*! Create a deep copy of an existing dictionary.
   *
   * @param[in] other Dictionary to copy.
   * @return New dictionary that is a deep copy of the input.
   *
   * @throw TypeError if deserialized data types cannot be handled.
   *
   * Example:
   * @code
   * Dictionary original;
   * original.set("temperature", 25.5);
   * original("sensors").set("count", 3);
   *
   * Dictionary copy = Dictionary::deepcopy(original);
   * copy.set("temperature", 30.0);  // Does not affect original
   * std::cout << original.get<double>("temperature") << std::endl;  // 25.5
   * @endcode
   */
  static Dictionary deepcopy(const Dictionary &other);

  /*! Create a new dictionary with keys from an iterable container and all
   * values set to the same value.
   *
   * @param[in] keys Container of keys (any iterable with string elements).
   * @param[in] value Value to set for all keys.
   * @return New dictionary with the specified keys and value.
   *
   * This function has the same semantics as Python's dict.fromkeys(iterable,
   * value). All keys will be set to the same value.
   *
   * Example:
   * @code
   * std::vector<std::string> keys = {"name", "age", "city"};
   * Dictionary dict = Dictionary::fromkeys(keys, std::string("unknown"));
   * std::cout << dict.get<std::string>("name") << std::endl;  // "unknown"
   * std::cout << dict.get<std::string>("age") << std::endl;   // "unknown"
   * std::cout << dict.get<std::string>("city") << std::endl;  // "unknown"
   * @endcode
   */
  template <typename Container, typename T>
  static Dictionary fromkeys(const Container &keys, const T &value) {
    Dictionary result;
    for (const auto &key : keys) {
      result(key) = value;
    }
    return result;
  }

  /*! Create a new dictionary with keys from an iterable container and all
   * values set to empty dictionaries.
   *
   * @param[in] keys Container of keys (any iterable with string elements).
   * @return New dictionary with the specified keys and empty dictionary values.
   *
   * This function is similar to Python's dict.fromkeys(iterable) when no value
   * is specified. However, in Python default values are set to None, while in
   * palimpsest default values are set to empty dictionaries (that may become
   * either values or dictionaries).
   *
   * Example:
   * @code
   * std::vector<std::string> keys = {"config", "temperature"};
   * Dictionary dict = Dictionary::fromkeys(keys);
   * sensor_dict("temperature") = 12.1;       // Can become a value
   * sensor_dict("config")("checks") = true;  // Can become a dictionary
   * @endcode
   */
  template <typename Container>
  static Dictionary fromkeys(const Container &keys) {
    Dictionary result;
    for (const auto &key : keys) {
      result(key);  // Create empty dictionary for each key
    }
    return result;
  }

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
   *
   * Example:
   * @code
   * Dictionary dict1;
   * dict1.set("temperature", 25.5);
   * dict1.set("pressure", 101.3);
   *
   * Dictionary dict2;
   * dict2.set("temperature", 30.0);  // This will overwrite dict1's temperature
   * dict2.set("humidity", 65.0);     // This will be added to dict1
   *
   * dict1.update(dict2);
   * std::cout << dict1.get<double>("temperature") << std::endl;  // 30.0
   * std::cout << dict1.get<double>("humidity") << std::endl;     // 65.0
   * @endcode
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
  void deserialize(const char *data, size_t size);

  /*! Compute the difference between this dictionary and another.
   *
   * @param[in] other Dictionary to compare with.
   * @return New dictionary containing key-value pairs that are in this
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
  /*! Update existing values from an MPack node.
   *
   * @param[in] node MPack node.
   *
   * @throw TypeError if a deserialized object's type does not match the type
   *     of an existing entry in the dictionary.
   */
  void deserialize_(mpack_node_t node);

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

#include "palimpsest/internal/fmt_formatter.h"
