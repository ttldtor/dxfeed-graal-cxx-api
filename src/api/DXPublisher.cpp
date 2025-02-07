// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include "dxfeed_graal_cpp_api/api/DXPublisher.hpp"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

std::shared_ptr<DXPublisher> DXPublisher::getInstance() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXPublisher::getInstance()");
    }

    return DXEndpoint::getInstance()->getPublisher();
}

std::shared_ptr<DXPublisher> DXPublisher::create(void *feedHandle) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXPublisher::create(" + dxfcpp::toString(feedHandle) + ")");
    }

    std::shared_ptr<DXPublisher> publisher{new (std::nothrow) DXPublisher{}};

    // TODO: error handling

    if (publisher) {
        publisher->handler_ = JavaObjectHandler<DXPublisher>(feedHandle);
    }

    return publisher;
}

std::string DXPublisher::toString() const noexcept {
    return fmt::format("DXPublisher{{{}}}", handler_.toString());
}

void DXPublisher::publishEventsImpl(void *graalEventsList) const noexcept {
    runIsolatedOrElse(
        [handler = bit_cast<dxfg_publisher_t *>(handler_.get()), graalEventsList](auto threadHandle) {
            return dxfg_DXPublisher_publishEvents(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler,
                                                  bit_cast<dxfg_event_type_list *>(graalEventsList)) == 0;
        },
        false);
}

} // namespace dxfcpp