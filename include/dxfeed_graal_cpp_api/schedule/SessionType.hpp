// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp {

/**
 * Defines type of session - what kind of trading activity is allowed (if any),
 * what rules are used, what impact on daily trading statistics it has, etc.
 * The ::NO_TRADING session type is used for non-trading sessions.
 * <p>
 * Some exchanges support all session types defined here, others do not.
 * <p>
 * Some sessions may have zero duration - e.g. indices that post value once a day.
 * Such sessions can be of any appropriate type, trading or non-trading.
 */
struct DXFCPP_EXPORT SessionType {
    /// Non-trading session type is used to mark periods of time during which trading is not allowed.
    static const SessionType NO_TRADING;
    /// Pre-market session type marks extended trading session before regular trading hours.
    static const SessionType PRE_MARKET;
    /// Regular session type marks regular trading hours session.
    static const SessionType REGULAR;
    /// After-market session type marks extended trading session after regular trading hours.
    static const SessionType AFTER_MARKET;

  private:
    std::string name_{};
    bool trading_{};

    SessionType(std::string name, bool trading) noexcept : name_{std::move(name)}, trading_{trading} {
    }

  public:
    SessionType() noexcept = default;
    virtual ~SessionType() noexcept = default;

    const std::string &getName() const & noexcept {
        return name_;
    }

    /**
     * Returns `true` if trading activity is allowed for this type of session.
     * <p>
     * Some sessions may have zero duration - e.g. indices that post value once a day.
     * Such sessions can be of any appropriate type, trading or non-trading.
     */
    bool isTrading() const noexcept {
        return trading_;
    }

    const std::string &toString() const & noexcept {
        return name_;
    }

    bool operator==(const SessionType &sessionType) const noexcept {
        return name_ == sessionType.getName();
    }
};

} // namespace dxfcpp

template <> struct std::hash<dxfcpp::SessionType> {
    std::size_t operator()(const dxfcpp::SessionType &sessionType) const noexcept {
        return std::hash<std::string>{}(sessionType.getName());
    }
};