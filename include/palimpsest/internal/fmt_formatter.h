// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 Stéphane Caron
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     Configuration and DataStore classes of mc_rtc
 *     Copyright 2015-2020 CNRS-UM LIRMM, CNRS-AIST JRL
 *     SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <fmt/format.h>

#include <sstream>

namespace palimpsest {
class Dictionary;
}

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
