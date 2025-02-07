// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <typeinfo>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

void DXFeedSubscription::attach(std::shared_ptr<DXFeed> feed) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::attach(feed = " + feed->toString() + ")");
    }

    feed->attachSubscription(sharedAs<DXFeedSubscription>());
}

void DXFeedSubscription::detach(std::shared_ptr<DXFeed> feed) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::detach(feed = " + feed->toString() + ")");
    }

    feed->detachSubscription(sharedAs<DXFeedSubscription>());
}

void DXFeedSubscription::addSymbolImpl(void *graalSymbol) const noexcept {
    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get()), graalSymbol](auto threadHandle) {
            return dxfg_DXFeedSubscription_addSymbol(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler,
                                                     bit_cast<dxfg_symbol_t *>(graalSymbol)) == 0;
        },
        false);
}

void DXFeedSubscription::addSymbolsImpl(void *graalSymbolList) const noexcept {
    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get()), graalSymbolList](auto threadHandle) {
            return dxfg_DXFeedSubscription_addSymbols(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler,
                                                      bit_cast<dxfg_symbol_list *>(graalSymbolList)) == 0;
        },
        false);
}

void DXFeedSubscription::removeSymbolImpl(void *graalSymbol) const noexcept {
    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get()), graalSymbol](auto threadHandle) {
            return dxfg_DXFeedSubscription_removeSymbol(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                        handler, bit_cast<dxfg_symbol_t *>(graalSymbol)) == 0;
        },
        false);
}

void DXFeedSubscription::removeSymbolsImpl(void *graalSymbolList) const noexcept {
    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get()), graalSymbolList](auto threadHandle) {
            return dxfg_DXFeedSubscription_removeSymbols(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                         handler, bit_cast<dxfg_symbol_list *>(graalSymbolList)) == 0;
        },
        false);
}

void DXFeedSubscription::setSymbolsImpl(void *graalSymbolList) const noexcept {
    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get()), graalSymbolList](auto threadHandle) {
            return dxfg_DXFeedSubscription_setSymbols(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler,
                                                      bit_cast<dxfg_symbol_list *>(graalSymbolList)) == 0;
        },
        false);
}

std::vector<SymbolWrapper> DXFeedSubscription::getSymbolsImpl() const noexcept {
    dxfg_symbol_list *list = runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_getSymbols(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler);
        },
        nullptr);

    auto result = SymbolWrapper::SymbolListUtils::fromGraalList(bit_cast<void *>(list));

    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get()), list](auto threadHandle) {
            return dxfg_CList_symbol_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), list) == 0;
        },
        false);

    return result;
}

std::vector<SymbolWrapper> DXFeedSubscription::getDecoratedSymbolsImpl() const noexcept {
    dxfg_symbol_list *list = runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_getDecoratedSymbols(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                               handler);
        },
        nullptr);

    auto result = SymbolWrapper::SymbolListUtils::fromGraalList(bit_cast<void *>(list));

    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get()), list](auto threadHandle) {
            return dxfg_CList_symbol_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), list) == 0;
        },
        false);

    return result;
}

void DXFeedSubscription::closeImpl() const noexcept {
    if (!handler_) {
        return;
    }

    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_close(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler) == 0;
        },
        false);
}

void DXFeedSubscription::clearImpl() const noexcept {
    if (!handler_) {
        return;
    }

    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_clear(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler) == 0;
        },
        false);
}

bool DXFeedSubscription::isClosedImpl() const noexcept {
    if (!handler_) {
        return false;
    }

    return runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_isClosed(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), handler) !=
                   0;
        },
        false);
}

DXFeedSubscription::DXFeedSubscription(const EventTypeEnum &eventType) noexcept
    : mtx_{}, eventTypes_{eventType}, handler_{}, eventListenerHandler_{}, onEvent_{} {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXFeedSubscription(eventType = " + eventType.getName() + ")");
    }

    handler_ = JavaObjectHandler<DXFeedSubscription>(runIsolatedOrElse(
        [eventType](auto threadHandle) {
            return dxfg_DXFeedSubscription_new(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                               static_cast<dxfg_event_clazz_t>(eventType.getId()));
        },
        nullptr));
}

JavaObjectHandler<DXFeedSubscription>
DXFeedSubscription::createSubscriptionHandlerFromEventClassList(const std::unique_ptr<EventClassList> &list) noexcept {
    return JavaObjectHandler<DXFeedSubscription>(runIsolatedOrElse(
        [listHandler = bit_cast<dxfg_event_clazz_list_t *>(list->getHandler())](auto threadHandle) {
            return dxfg_DXFeedSubscription_new2(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), listHandler);
        },
        nullptr));
}

void DXFeedSubscription::setEventListenerHandler(Id<DXFeedSubscription> id) noexcept {
    auto onEvents = [](graal_isolatethread_t * /*thread*/, dxfg_event_type_list *graalNativeEvents, void *userData) {
        auto id = Id<DXFeedSubscription>::from(bit_cast<Id<DXFeedSubscription>::ValueType>(userData));
        auto sub = ApiContext::getInstance()->getDxFeedSubscriptionManager()->getEntity(id);

        if (sub) {
            auto &&events = EventMapper::fromGraalList(bit_cast<void *>(graalNativeEvents));

            sub->onEvent_(events);
        }
    };

    eventListenerHandler_ = JavaObjectHandler<DXFeedEventListener>(runIsolatedOrElse(
        [idValue = id.getValue(), onEvents](auto threadHandle) {
            return dxfg_DXFeedEventListener_new(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), onEvents,
                                                bit_cast<void *>(idValue));
        },
        nullptr));

    if (handler_ && eventListenerHandler_) {
        runIsolatedOrElse(
            [handler = bit_cast<dxfg_subscription_t *>(handler_.get()),
             eventListenerHandler =
                 bit_cast<dxfg_feed_event_listener_t *>(eventListenerHandler_.get())](auto threadHandle) {
                return dxfg_DXFeedSubscription_addEventListener(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                                handler, eventListenerHandler) == 0;
            },
            false);
    }
}

std::string DXFeedSubscription::toString() const noexcept {
    return fmt::format("DXFeedSubscription{{{}}}", handler_.toString());
}

} // namespace dxfcpp