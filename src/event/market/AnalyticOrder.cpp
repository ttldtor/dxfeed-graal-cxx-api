// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "dxfeed_graal_cpp_api/event/market/AnalyticOrder.hpp"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &AnalyticOrder::TYPE = EventTypeEnum::ANALYTIC_ORDER;

void AnalyticOrder::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillData(graalNative);

    auto graalAnalyticOrder = static_cast<dxfg_analytic_order_t *>(graalNative);

    analyticOrderData_ = {.icebergPeakSize = graalAnalyticOrder->iceberg_peak_size,
                          .icebergHiddenSize = graalAnalyticOrder->iceberg_hidden_size,
                          .icebergExecutedSize = graalAnalyticOrder->iceberg_executed_size,
                          .icebergFlags = graalAnalyticOrder->iceberg_flags};
}

void AnalyticOrder::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillGraalData(graalNative);

    auto graalAnalyticOrder = static_cast<dxfg_analytic_order_t *>(graalNative);

    graalAnalyticOrder->iceberg_peak_size = analyticOrderData_.icebergPeakSize;
    graalAnalyticOrder->iceberg_hidden_size = analyticOrderData_.icebergHiddenSize;
    graalAnalyticOrder->iceberg_executed_size = analyticOrderData_.icebergExecutedSize;
    graalAnalyticOrder->iceberg_flags = analyticOrderData_.icebergFlags;
}

std::shared_ptr<AnalyticOrder> AnalyticOrder::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_ANALYTIC_ORDER) {
        return {};
    }

    try {
        auto analyticOrder = std::make_shared<AnalyticOrder>();

        analyticOrder->fillData(graalNative);

        return analyticOrder;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string AnalyticOrder::toString() const noexcept {
    return fmt::format(
        "AnalyticOrder{{{}, icebergPeakSize={}, icebergHiddenSize={}, icebergExecutedSize={}, icebergType={}}}",
        baseFieldsToString(), dxfcpp::toString(getIcebergPeakSize()), dxfcpp::toString(getIcebergHiddenSize()),
        dxfcpp::toString(getIcebergExecutedSize()), getIcebergType().toString());
}

void *AnalyticOrder::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalAnalyticOrder = new (std::nothrow) dxfg_analytic_order_t{
        .order_base = {
            .order_base = {.market_event = {.event_type = {.clazz = dxfg_event_clazz_t::DXFG_EVENT_ANALYTIC_ORDER}}}}};

    if (!graalAnalyticOrder) {
        // TODO: error handling

        return nullptr;
    }

    fillGraalData(static_cast<void *>(graalAnalyticOrder));

    return static_cast<void *>(graalAnalyticOrder);
}

void AnalyticOrder::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_ANALYTIC_ORDER) {
        return;
    }

    auto graalAnalyticOrder = static_cast<dxfg_analytic_order_t *>(graalNative);

    Order::freeGraalData(graalNative);

    delete graalAnalyticOrder;
}

} // namespace dxfcpp