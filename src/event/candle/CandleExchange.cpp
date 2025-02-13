// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

const CandleExchange CandleExchange::COMPOSITE{'\0'};
const CandleExchange CandleExchange::DEFAULT = COMPOSITE;

} // namespace dxfcpp