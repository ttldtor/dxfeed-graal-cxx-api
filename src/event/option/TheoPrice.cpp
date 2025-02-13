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

const EventTypeEnum &TheoPrice::TYPE = EventTypeEnum::THEO_PRICE;

void TheoPrice::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalTheoPrice = static_cast<dxfg_theo_price_t *>(graalNative);

    data_ = {
        .eventFlags = graalTheoPrice->event_flags,
        .index = graalTheoPrice->index,
        .price = graalTheoPrice->price,
        .underlyingPrice = graalTheoPrice->underlying_price,
        .delta = graalTheoPrice->delta,
        .gamma = graalTheoPrice->gamma,
        .dividend = graalTheoPrice->dividend,
        .interest = graalTheoPrice->interest,
    };
}

void TheoPrice::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    auto graalTheoPrice = static_cast<dxfg_theo_price_t *>(graalNative);

    graalTheoPrice->event_flags = data_.eventFlags;
    graalTheoPrice->index = data_.index;
    graalTheoPrice->price = data_.price;
    graalTheoPrice->underlying_price = data_.underlyingPrice;
    graalTheoPrice->delta = data_.delta;
    graalTheoPrice->gamma = data_.gamma;
    graalTheoPrice->dividend = data_.dividend;
    graalTheoPrice->interest = data_.interest;
}

std::shared_ptr<TheoPrice> TheoPrice::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_THEO_PRICE) {
        return {};
    }

    try {
        auto theoPrice = std::make_shared<TheoPrice>();

        theoPrice->fillData(graalNative);

        return theoPrice;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string TheoPrice::toString() const noexcept {
    return fmt::format(
        "TheoPrice{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, price={}, underlyingPrice={}, "
        "delta={}, gamma={}, dividend={}, interest={}}}",
        MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
        getEventFlagsMask().getMask(), formatTimeStampWithMillis(getTime()), getSequence(),
        dxfcpp::toString(getPrice()), dxfcpp::toString(getUnderlyingPrice()), dxfcpp::toString(getDelta()),
        dxfcpp::toString(getGamma()), dxfcpp::toString(getDividend()), dxfcpp::toString(getInterest()));
}

void *TheoPrice::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalTheoPrice = new (std::nothrow)
        dxfg_theo_price_t{.market_event = {.event_type = {.clazz = dxfg_event_clazz_t::DXFG_EVENT_THEO_PRICE}}};

    if (!graalTheoPrice) {
        // TODO: error handling

        return nullptr;
    }

    fillGraalData(static_cast<void *>(graalTheoPrice));

    return static_cast<void *>(graalTheoPrice);
}

void TheoPrice::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_THEO_PRICE) {
        return;
    }

    auto graalTheoPrice = static_cast<dxfg_theo_price_t *>(graalNative);

    MarketEvent::freeGraalData(graalNative);

    delete graalTheoPrice;
}

} // namespace dxfcpp