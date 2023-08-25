// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &Profile::TYPE = EventTypeEnum::PROFILE;

void Profile::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalProfile = static_cast<dxfg_profile_t *>(graalNative);

    data_ = {
        .description = dxfcpp::toString(graalProfile->description),
        .statusReason = dxfcpp::toString(graalProfile->status_reason),
        .haltStartTime = graalProfile->halt_start_time,
        .haltEndTime = graalProfile->halt_end_time,
        .highLimitPrice = graalProfile->high_limit_price,
        .lowLimitPrice = graalProfile->low_limit_price,
        .high52WeekPrice = graalProfile->high_52_week_price,
        .low52WeekPrice = graalProfile->low_52_week_price,
        .beta = graalProfile->beta,
        .earningsPerShare = graalProfile->earnings_per_share,
        .dividendFrequency = graalProfile->dividend_frequency,
        .exDividendAmount = graalProfile->ex_dividend_amount,
        .exDividendDayId = graalProfile->ex_dividend_day_id,
        .shares = graalProfile->shares,
        .freeFloat = graalProfile->free_float,
        .flags = graalProfile->flags,
    };
}

void Profile::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    auto graalProfile = static_cast<dxfg_profile_t *>(graalNative);

    graalProfile->description = createCString(data_.description);
    graalProfile->status_reason = createCString(data_.statusReason);
    graalProfile->halt_start_time = data_.haltStartTime;
    graalProfile->halt_end_time = data_.haltEndTime;
    graalProfile->high_limit_price = data_.highLimitPrice;
    graalProfile->low_limit_price = data_.lowLimitPrice;
    graalProfile->high_52_week_price = data_.high52WeekPrice;
    graalProfile->low_52_week_price = data_.low52WeekPrice;
    graalProfile->beta = data_.beta;
    graalProfile->earnings_per_share = data_.earningsPerShare;
    graalProfile->dividend_frequency = data_.dividendFrequency;
    graalProfile->ex_dividend_amount = data_.exDividendAmount;
    graalProfile->ex_dividend_day_id = data_.exDividendDayId;
    graalProfile->shares = data_.shares;
    graalProfile->free_float = data_.freeFloat;
    graalProfile->flags = data_.flags;
}

void Profile::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    auto graalProfile = static_cast<dxfg_profile_t *>(graalNative);

    delete[] graalProfile->description;
    delete[] graalProfile->status_reason;
}

std::shared_ptr<Profile> Profile::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_PROFILE) {
        return {};
    }

    try {
        auto profile = std::make_shared<Profile>();

        profile->fillData(graalNative);

        return profile;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string Profile::toString() const noexcept {
    return fmt::format("Profile{{{}, eventTime={}, description='{}', SSR={}, status={}, statusReason='{}', "
                       "haltStartTime={}, haltEndTime={}, highLimitPrice={}, lowLimitPrice={}, high52WeekPrice={}, "
                       "low52WeekPrice={}, beta={}, earningsPerShare={}, dividendFrequency={}, "
                       "exDividendAmount={}, exDividendDay={}, shares={}, freeFloat={}}}",
                       MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
                       getDescription(), getShortSaleRestriction().toString(), getTradingStatus().toString(),
                       getStatusReason(), formatTimeStamp(getHaltStartTime()), formatTimeStamp(getHaltEndTime()),
                       dxfcpp::toString(getHighLimitPrice()), dxfcpp::toString(getLowLimitPrice()),
                       dxfcpp::toString(getHigh52WeekPrice()), dxfcpp::toString(getLow52WeekPrice()),
                       dxfcpp::toString(getBeta()), dxfcpp::toString(getEarningsPerShare()),
                       dxfcpp::toString(getDividendFrequency()), dxfcpp::toString(getExDividendAmount()),
                       day_util::getYearMonthDayByDayId(getExDividendDayId()), dxfcpp::toString(getShares()),
                       dxfcpp::toString(getFreeFloat()));
}

void *Profile::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalProfile = new (std::nothrow)
        dxfg_profile_t{.market_event = {.event_type = {.clazz = dxfg_event_clazz_t::DXFG_EVENT_PROFILE}}};

    if (!graalProfile) {
        // TODO: error handling

        return nullptr;
    }

    fillGraalData(static_cast<void *>(graalProfile));

    return static_cast<void *>(graalProfile);
}

void Profile::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_PROFILE) {
        return;
    }

    auto graalProfile = static_cast<dxfg_profile_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalProfile;
}

} // namespace dxfcpp