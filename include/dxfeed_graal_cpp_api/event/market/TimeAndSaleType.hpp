// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "../../internal/Common.hpp"
#include "../../internal/Enum.hpp"

namespace dxfcpp {

/**
 * Type of a time and sale event.
 */
struct TimeAndSaleType : Enum<TimeAndSaleType, std::uint32_t> {
    using Enum::Enum;

    /**
     * Represents new time and sale event.
     */
    static const TimeAndSaleType NEW;

    /**
     * Represents correction time and sale event.
     */
    static const TimeAndSaleType CORRECTION;

    /**
     * Represents cancel time and sale event.
     */
    static const TimeAndSaleType CANCEL;
};

template <>
const std::unordered_map<TimeAndSaleType::CodeType, std::reference_wrapper<const TimeAndSaleType>>
    TimeAndSaleType::ParentType::ALL;
} // namespace dxfcpp