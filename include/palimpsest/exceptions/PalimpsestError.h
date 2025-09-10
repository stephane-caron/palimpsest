// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 Stéphane Caron

#pragma once

#include <string>

//! Exceptions raised by the library.
namespace palimpsest::exceptions {

//! Error with file and line references to the calling code.
class PalimpsestError : public std::runtime_error {
 public:
  /*! Create a new error.
   *
   * @param[in] file Source file of the instruction that threw the error.
   * @param[in] line Line of code in that file where the throw originates from.
   * @param[in] message Error message.
   */
  PalimpsestError(const std::string& file, unsigned line,
                  const std::string& message)
      : std::runtime_error(message) {
    std::ostringstream out;
    out << "[" << file << ":" << line << "] " << message;
    message_ = out.str();
  }

  /*! Copy an existing error, adding to the error message.
   *
   * @param[in] other Existing error.
   * @param[in] extra_message Additional error message.
   */
  PalimpsestError(const PalimpsestError& other,
                  const std::string& extra_message)
      : std::runtime_error(other.message_ + extra_message),
        message_(other.message_ + extra_message) {}

  //! Empty destructor
  ~PalimpsestError() throw() {}

  //! Error message
  const char* what() const throw() { return message_.c_str(); }

 private:
  //! Complete error message
  std::string message_;
};

}  // namespace palimpsest::exceptions
