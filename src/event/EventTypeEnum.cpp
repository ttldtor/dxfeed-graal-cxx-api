// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

const EventTypeEnum EventTypeEnum::QUOTE{DXFG_EVENT_QUOTE, "QUOTE", true};
const EventTypeEnum EventTypeEnum::PROFILE{DXFG_EVENT_PROFILE, "PROFILE", true};
const EventTypeEnum EventTypeEnum::SUMMARY{DXFG_EVENT_SUMMARY, "SUMMARY", true};
const EventTypeEnum EventTypeEnum::GREEKS{DXFG_EVENT_GREEKS, "GREEKS", true, true, true};
const EventTypeEnum EventTypeEnum::CANDLE{DXFG_EVENT_CANDLE, "CANDLE", true, true, true};

// const EventTypeEnum EventTypeEnum::DAILY_CANDLE{DXFG_EVENT_DAILY_CANDLE, "DAILY_CANDLE", true, true, true};

const EventTypeEnum EventTypeEnum::UNDERLYING{DXFG_EVENT_UNDERLYING, "UNDERLYING", true, true, true};
const EventTypeEnum EventTypeEnum::THEO_PRICE{DXFG_EVENT_THEO_PRICE, "THEO_PRICE", true, true, true};
const EventTypeEnum EventTypeEnum::TRADE{DXFG_EVENT_TRADE, "TRADE", true};
const EventTypeEnum EventTypeEnum::TRADE_ETH{DXFG_EVENT_TRADE_ETH, "TRADE_ETH", true};
const EventTypeEnum EventTypeEnum::CONFIGURATION{DXFG_EVENT_CONFIGURATION, "CONFIGURATION", true};
const EventTypeEnum EventTypeEnum::MESSAGE{DXFG_EVENT_MESSAGE, "MESSAGE", false};
const EventTypeEnum EventTypeEnum::TIME_AND_SALE{DXFG_EVENT_TIME_AND_SALE, "TIME_AND_SALE", false, true, true};

// const EventTypeEnum EventTypeEnum::ORDER_BASE{DXFG_EVENT_ORDER_BASE, "ORDER_BASE", false, true};

const EventTypeEnum EventTypeEnum::ORDER{DXFG_EVENT_ORDER, "ORDER", false, true};
const EventTypeEnum EventTypeEnum::ANALYTIC_ORDER{DXFG_EVENT_ANALYTIC_ORDER, "ANALYTIC_ORDER", false, true};
const EventTypeEnum EventTypeEnum::SPREAD_ORDER{DXFG_EVENT_SPREAD_ORDER, "SPREAD_ORDER", false, true};
const EventTypeEnum EventTypeEnum::SERIES{DXFG_EVENT_SERIES, "SERIES", false, true};
const EventTypeEnum EventTypeEnum::OPTION_SALE{DXFG_EVENT_OPTION_SALE, "OPTION_SALE", false, true};

const std::vector<std::reference_wrapper<const EventTypeEnum>> EventTypeEnum::ALL{
    std::cref(QUOTE),
    std::cref(PROFILE),
    std::cref(SUMMARY),
    std::cref(GREEKS),
    std::cref(CANDLE),
    std::cref(UNDERLYING),
    std::cref(THEO_PRICE),
    std::cref(TRADE),
    std::cref(TRADE_ETH),
    // std::cref(CONFIGURATION),
    // std::cref(MESSAGE),
    std::cref(TIME_AND_SALE),
    std::cref(ORDER),
    std::cref(ANALYTIC_ORDER),
    std::cref(SPREAD_ORDER),
    std::cref(SERIES),
    std::cref(OPTION_SALE),
};

const std::unordered_map<std::string, std::reference_wrapper<const EventTypeEnum>> EventTypeEnum::ALL_BY_NAME{
    {QUOTE.getName(), std::cref(QUOTE)},
    {PROFILE.getName(), std::cref(PROFILE)},
    {SUMMARY.getName(), std::cref(SUMMARY)},
    {GREEKS.getName(), std::cref(GREEKS)},
    {CANDLE.getName(), std::cref(CANDLE)},
    {UNDERLYING.getName(), std::cref(UNDERLYING)},
    {THEO_PRICE.getName(), std::cref(THEO_PRICE)},
    {TRADE.getName(), std::cref(TRADE)},
    {TRADE_ETH.getName(), std::cref(TRADE_ETH)},
    // {CONFIGURATION.getName(), std::cref(CONFIGURATION)},
    // {MESSAGE.getName(), std::cref(MESSAGE)},
    {TIME_AND_SALE.getName(), std::cref(TIME_AND_SALE)},
    {ORDER.getName(), std::cref(ORDER)},
    {ANALYTIC_ORDER.getName(), std::cref(ANALYTIC_ORDER)},
    {SPREAD_ORDER.getName(), std::cref(SPREAD_ORDER)},
    {SERIES.getName(), std::cref(SERIES)},
    {OPTION_SALE.getName(), std::cref(OPTION_SALE)},
};

} // namespace dxfcpp