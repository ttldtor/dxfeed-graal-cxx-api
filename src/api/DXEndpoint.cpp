// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <string>
#include <memory>
#include <utility>

namespace dxfcpp {

const std::string DXEndpoint::NAME_PROPERTY = "name";
const std::string DXEndpoint::DXFEED_PROPERTIES_PROPERTY = "dxfeed.properties";
const std::string DXEndpoint::DXFEED_ADDRESS_PROPERTY = "dxfeed.address";
const std::string DXEndpoint::DXFEED_USER_PROPERTY = "dxfeed.user";
const std::string DXEndpoint::DXFEED_PASSWORD_PROPERTY = "dxfeed.password";
const std::string DXEndpoint::DXFEED_THREAD_POOL_SIZE_PROPERTY = "dxfeed.threadPoolSize";
const std::string DXEndpoint::DXFEED_AGGREGATION_PERIOD_PROPERTY = "dxfeed.aggregationPeriod";
const std::string DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY = "dxfeed.wildcard.enable";
const std::string DXEndpoint::DXPUBLISHER_PROPERTIES_PROPERTY = "dxpublisher.properties";
const std::string DXEndpoint::DXPUBLISHER_ADDRESS_PROPERTY = "dxpublisher.address";
const std::string DXEndpoint::DXPUBLISHER_THREAD_POOL_SIZE_PROPERTY = "dxpublisher.threadPoolSize";
const std::string DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY = "dxendpoint.eventTime";
const std::string DXEndpoint::DXENDPOINT_STORE_EVERYTHING_PROPERTY = "dxendpoint.storeEverything";
const std::string DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY = "dxscheme.nanoTime";
const std::string DXEndpoint::DXSCHEME_ENABLED_PROPERTY_PREFIX = "dxscheme.enabled.";

std::unordered_map<DXEndpoint::Role, std::shared_ptr<DXEndpoint>> DXEndpoint::INSTANCES{};

static dxfg_endpoint_role_t roleToGraalRole(DXEndpoint::Role role) {
    switch (role) {
    case DXEndpoint::Role::FEED:
        return DXFG_ENDPOINT_ROLE_FEED;
    case DXEndpoint::Role::ON_DEMAND_FEED:
        return DXFG_ENDPOINT_ROLE_ON_DEMAND_FEED;
    case DXEndpoint::Role::STREAM_FEED:
        return DXFG_ENDPOINT_ROLE_STREAM_FEED;
    case DXEndpoint::Role::PUBLISHER:
        return DXFG_ENDPOINT_ROLE_PUBLISHER;
    case DXEndpoint::Role::STREAM_PUBLISHER:
        return DXFG_ENDPOINT_ROLE_STREAM_PUBLISHER;
    case DXEndpoint::Role::LOCAL_HUB:
        return DXFG_ENDPOINT_ROLE_LOCAL_HUB;
    }

    return DXFG_ENDPOINT_ROLE_FEED;
}

static DXEndpoint::State graalStateToState(dxfg_endpoint_state_t state) {
    switch (state) {
    case DXFG_ENDPOINT_STATE_NOT_CONNECTED:
        return DXEndpoint::State::NOT_CONNECTED;
    case DXFG_ENDPOINT_STATE_CONNECTING:
        return DXEndpoint::State::CONNECTING;
    case DXFG_ENDPOINT_STATE_CONNECTED:
        return DXEndpoint::State::CONNECTED;
    case DXFG_ENDPOINT_STATE_CLOSED:
        return DXEndpoint::State::CLOSED;
    }

    return DXEndpoint::State::NOT_CONNECTED;
}

std::shared_ptr<DXEndpoint> DXEndpoint::create(void *endpointHandle, DXEndpoint::Role role,
                                               const std::unordered_map<std::string, std::string> &properties) {
    if constexpr (isDebug) {
        debug("DXEndpoint::create{{handle = {}, role = {}, properties[{}]}}()", endpointHandle, roleToString(role),
              properties.size());
    }

    std::shared_ptr<DXEndpoint> endpoint{new (std::nothrow) DXEndpoint{}};

    endpoint->handler_ = handler_utils::JavaObjectHandler<DXEndpoint>(endpointHandle);
    endpoint->role_ = role;
    endpoint->name_ = properties.contains(NAME_PROPERTY) ? properties.at(NAME_PROPERTY) : std::string{};
    endpoint->stateChangeListenerHandler_ =
        handler_utils::JavaObjectHandler<DXEndpointStateChangeListener>(runIsolatedOrElse(
            [endpoint](auto threadHandle) {
                return dxfg_PropertyChangeListener_new(
                    threadHandle,
                    [](graal_isolatethread_t *thread, dxfg_endpoint_state_t oldState, dxfg_endpoint_state_t newState,
                       void *user_data) {
                        bit_cast<DXEndpoint *>(user_data)->onStateChange_(graalStateToState(oldState),
                                                                          graalStateToState(newState));
                    },
                    endpoint.get());
            },
            nullptr));
    endpoint->setStateChangeListenerImpl();

    return endpoint;
}

void DXEndpoint::setStateChangeListenerImpl() {
    if (handler_ && stateChangeListenerHandler_) {
        runIsolatedOrElse(
            [handler = bit_cast<dxfg_endpoint_t *>(handler_.get()),
             stateChangeListenerHandler = bit_cast<dxfg_endpoint_state_change_listener_t *>(
                 stateChangeListenerHandler_.get())](auto threadHandle) {
                // TODO: finalize function

                return dxfg_DXEndpoint_addStateChangeListener(
                           threadHandle, handler, stateChangeListenerHandler, [](auto, auto) {}, nullptr) == 0;
            },
            false);
    }
}

DXEndpoint::State DXEndpoint::getState() const {
    std::lock_guard guard(mtx_);

    return !handler_ ? State::CLOSED
                     : runIsolatedOrElse(
                           [handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                               return graalStateToState(dxfg_DXEndpoint_getState(threadHandle, handler));
                           },
                           State::CLOSED);
}

void DXEndpoint::closeImpl() {
    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_close(threadHandle, handler) == 0; },
                      false);

    // TODO: close the Feed and Publisher
}

std::shared_ptr<DXEndpoint> DXEndpoint::user(const std::string &user) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::user(user = {})", handler_.toString(), user);
    }

    std::lock_guard guard(mtx_);

    if (handler_) {
        runIsolatedOrElse(
            [user = user, handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_user(threadHandle, handler, user.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

std::shared_ptr<DXEndpoint> DXEndpoint::password(const std::string &password) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::password(password = {})", handler_.toString(), password);
    }

    std::lock_guard guard(mtx_);

    if (handler_) {
        runIsolatedOrElse(
            [password = password, handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_password(threadHandle, handler, password.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

std::shared_ptr<DXEndpoint> DXEndpoint::connect(const std::string &address) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::connect(address = {})", handler_.toString(), address);
    }

    std::lock_guard guard(mtx_);

    if (handler_) {
        runIsolatedOrElse(
            [address = address, handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_connect(threadHandle, handler, address.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

void DXEndpoint::reconnect() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::reconnect()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_reconnect(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::disconnect() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::disconnect()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_disconnect(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::disconnectAndClear() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::disconnectAndClear()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_disconnectAndClear(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::awaitNotConnected() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::awaitNotConnected()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_awaitNotConnected(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::awaitProcessed() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::awaitProcessed()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_awaitProcessed(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::closeAndAwaitTermination() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::closeAndAwaitTermination()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse(
        [handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_closeAndAwaitTermination(threadHandle, handler) == 0;
        },
        false);

    // TODO: close the Feed and Publisher
}

std::shared_ptr<DXFeed> DXEndpoint::getFeed() {
    std::lock_guard guard(mtx_);

    if (!feed_) {
        auto feedHandle = !handler_ ? nullptr
                                    : runIsolatedOrElse(
                                          [handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                                              return dxfg_DXEndpoint_getFeed(threadHandle, handler);
                                          },
                                          nullptr);

        feed_ = DXFeed::create(feedHandle);
    }

    return feed_;
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::create() noexcept {
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder::create()");
    }

    auto builder = std::shared_ptr<Builder>(new (std::nothrow) Builder{});

    if (builder) {
        builder->handler_ = handler_utils::JavaObjectHandler<DXEndpoint::Builder>(
            runIsolatedOrElse([](auto threadHandle) { return dxfg_DXEndpoint_newBuilder(threadHandle); }, nullptr));
    }

    return builder;
}

void DXEndpoint::Builder::loadDefaultPropertiesImpl() {
    // The default properties file is only valid for the
    // Feed, OnDemandFeed and Publisher roles.
    auto propertiesFileKey = [this]() -> std::string {
        switch (role_) {
        case Role::FEED:
        case Role::ON_DEMAND_FEED:
            return DXFEED_PROPERTIES_PROPERTY;
        case Role::PUBLISHER:
            return DXPUBLISHER_PROPERTIES_PROPERTY;
        case Role::STREAM_FEED:
        case Role::STREAM_PUBLISHER:
        case Role::LOCAL_HUB:
            break;
        }

        return {};
    }();

    if (propertiesFileKey.empty()) {
        return;
    }

    // If propertiesFileKey was set in system properties,
    // don't try to load the default properties file.
    if (!System::getProperty(propertiesFileKey).empty()) {
        return;
    }

    // If there is no propertiesFileKey in user-set properties,
    // try load default properties file from current runtime directory if file exist.
    if (!properties_.contains(propertiesFileKey) && std::filesystem::exists(propertiesFileKey)) {
        if (!handler_) {
            return;
        }

        // The default property file has the same value as the key.
        runIsolatedOrElse(
            [key = propertiesFileKey, value = propertiesFileKey,
             handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withProperty(threadHandle, handler, key.c_str(), value.c_str()) == 0;
            },
            false);
    }
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withRole(DXEndpoint::Role role) {
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder{{{}}}::withRole(role = {})", handler_.toString(), roleToString(role));
    }

    std::lock_guard guard(mtx_);

    role_ = role;

    if (handler_) {
        runIsolatedOrElse(
            [role = role, handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withRole(threadHandle, handler, roleToGraalRole(role)) == 0;
            },
            false);
    }

    return shared_from_this();
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withProperty(const std::string &key,
                                                                       const std::string &value) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder{{{}}}::withProperty(key = {}, value = {})", handler_.toString(), key, value);
    }

    std::lock_guard guard(mtx_);

    properties_[key] = value;

    if (handler_) {
        runIsolatedOrElse(
            [key = key, value = value,
             handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withProperty(threadHandle, handler, key.c_str(), value.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

bool DXEndpoint::Builder::supportsProperty(const std::string &key) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder{{{}}}::supportsProperty(key = {})", handler_.toString(), key);
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return false;
    }

    return runIsolatedOrElse(
        [key = key, handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_Builder_supportsProperty(threadHandle, handler, key.c_str()) != 0;
        },
        false);
}

std::shared_ptr<DXEndpoint> DXEndpoint::Builder::build() {
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder{{{}}}::build()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    loadDefaultPropertiesImpl();

    auto endpointHandle =
        !handler_
            ? nullptr
            : runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](
                                    auto threadHandle) { return dxfg_DXEndpoint_Builder_build(threadHandle, handler); },
                                nullptr);

    return DXEndpoint::create(endpointHandle, role_, properties_);
}

struct BuilderHandle {};

struct BuilderRegistry {
    static std::unordered_map<BuilderHandle *, std::shared_ptr<DXEndpoint::Builder>> builders;

    static std::shared_ptr<DXEndpoint::Builder> get(BuilderHandle *handle) {
        if (builders.contains(handle)) {
            return builders[handle];
        }

        return {};
    }

    static BuilderHandle *add(std::shared_ptr<DXEndpoint::Builder> builder) noexcept {
        auto handle = new (std::nothrow) BuilderHandle{};

        builders[handle] = std::move(builder);

        return handle;
    }

    static bool remove(BuilderHandle *handle) {
        if (!handle) {
            return false;
        }

        auto result = builders.erase(handle) == 1;

        if (result) {
            delete handle;
        }

        return result;
    }
};

std::unordered_map<BuilderHandle *, std::shared_ptr<DXEndpoint::Builder>> BuilderRegistry::builders{};

struct EndpointWrapperHandle {};

struct EndpointWrapper : std::enable_shared_from_this<EndpointWrapper> {
    std::shared_ptr<dxfcpp::DXEndpoint> endpoint{};
    void *userData{};
    std::unordered_map<dxfc_dxendpoint_state_change_listener, std::size_t> listeners{};

    EndpointWrapper(std::shared_ptr<dxfcpp::DXEndpoint> endpoint, void *userData)
        : endpoint{std::move(endpoint)}, userData{userData}, listeners{} {}
};

struct EndpointWrapperRegistry {
    static std::unordered_map<EndpointWrapperHandle *, std::shared_ptr<EndpointWrapper>> endpointWrappers;

    static std::shared_ptr<EndpointWrapper> get(EndpointWrapperHandle *handle) {
        if (endpointWrappers.contains(handle)) {
            return endpointWrappers[handle];
        }

        return {};
    }

    static EndpointWrapperHandle *add(std::shared_ptr<EndpointWrapper> endpointWrapper) noexcept {
        auto handle = new (std::nothrow) EndpointWrapperHandle{};

        endpointWrappers[handle] = std::move(endpointWrapper);

        return handle;
    }

    static bool remove(EndpointWrapperHandle *handle) {
        if (!handle) {
            return false;
        }

        auto result = endpointWrappers.erase(handle) == 1;

        if (result) {
            delete handle;
        }

        return result;
    }
};

std::unordered_map<EndpointWrapperHandle *, std::shared_ptr<EndpointWrapper>>
    EndpointWrapperRegistry::endpointWrappers{};

static dxfcpp::DXEndpoint::Role cApiRoleToRole(dxfc_dxendpoint_role_t role) {
    switch (role) {
    case DXFC_DXENDPOINT_ROLE_FEED:
        return dxfcpp::DXEndpoint::Role::FEED;
    case DXFC_DXENDPOINT_ROLE_ON_DEMAND_FEED:
        return dxfcpp::DXEndpoint::Role::ON_DEMAND_FEED;
    case DXFC_DXENDPOINT_ROLE_STREAM_FEED:
        return dxfcpp::DXEndpoint::Role::STREAM_FEED;
    case DXFC_DXENDPOINT_ROLE_PUBLISHER:
        return dxfcpp::DXEndpoint::Role::PUBLISHER;
    case DXFC_DXENDPOINT_ROLE_STREAM_PUBLISHER:
        return dxfcpp::DXEndpoint::Role::STREAM_PUBLISHER;
    case DXFC_DXENDPOINT_ROLE_LOCAL_HUB:
        return dxfcpp::DXEndpoint::Role::LOCAL_HUB;
    }

    return dxfcpp::DXEndpoint::Role::FEED;
}

static dxfc_dxendpoint_role_t roleToCApiRole(dxfcpp::DXEndpoint::Role role) {
    switch (role) {
    case DXEndpoint::Role::FEED:
        return DXFC_DXENDPOINT_ROLE_FEED;
    case DXEndpoint::Role::ON_DEMAND_FEED:
        return DXFC_DXENDPOINT_ROLE_ON_DEMAND_FEED;
    case DXEndpoint::Role::STREAM_FEED:
        return DXFC_DXENDPOINT_ROLE_STREAM_FEED;
    case DXEndpoint::Role::PUBLISHER:
        return DXFC_DXENDPOINT_ROLE_PUBLISHER;
    case DXEndpoint::Role::STREAM_PUBLISHER:
        return DXFC_DXENDPOINT_ROLE_STREAM_PUBLISHER;
    case DXEndpoint::Role::LOCAL_HUB:
        return DXFC_DXENDPOINT_ROLE_LOCAL_HUB;
    }

    return DXFC_DXENDPOINT_ROLE_FEED;
}

static dxfc_dxendpoint_state_t stateToCApiState(dxfcpp::DXEndpoint::State state) {
    switch (state) {
    case DXEndpoint::State::NOT_CONNECTED:
        return DXFC_DXENDPOINT_STATE_NOT_CONNECTED;
    case DXEndpoint::State::CONNECTING:
        return DXFC_DXENDPOINT_STATE_CONNECTING;
    case DXEndpoint::State::CONNECTED:
        return DXFC_DXENDPOINT_STATE_CONNECTED;
    case DXEndpoint::State::CLOSED:
        return DXFC_DXENDPOINT_STATE_CLOSED;
    }

    return DXFC_DXENDPOINT_STATE_NOT_CONNECTED;
}

} // namespace dxfcpp

dxfc_error_code_t dxfc_dxendpoint_new_builder(DXFC_OUT dxfc_dxendpoint_builder_t *builderHandle) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::BuilderRegistry::add(dxfcpp::DXEndpoint::newBuilder());

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *builderHandle = dxfcpp::bit_cast<dxfc_dxendpoint_builder_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_role(dxfc_dxendpoint_builder_t builderHandle,
                                                    dxfc_dxendpoint_role_t role) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withRole(dxfcpp::cApiRoleToRole(role));

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_name(dxfc_dxendpoint_builder_t builderHandle, const char *name) {
    if (!builderHandle || !name) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withName(name);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_property(dxfc_dxendpoint_builder_t builderHandle, const char *key,
                                                        const char *value) {
    if (!builderHandle || !key || !value) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withProperty(key, value);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_properties(dxfc_dxendpoint_builder_t builderHandle,
                                                          const dxfc_dxendpoint_property_t **properties, size_t size) {
    if (!builderHandle || !properties || size == 0) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    for (std::size_t i = 0; i < size; i++) {
        if (!properties[i]) {
            continue;
        }

        if (!properties[i]->key || !properties[i]->value) {
            continue;
        }

        builder->withProperty(properties[i]->key, properties[i]->value);
    }

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_supports_property(dxfc_dxendpoint_builder_t builderHandle, const char *key,
                                                            DXFC_OUT int *supports) {
    if (!builderHandle || !key || !supports) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    *supports = builder->supportsProperty(key);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_build(dxfc_dxendpoint_builder_t builderHandle, void *userData,
                                                DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!builderHandle || !endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = builder->build();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_free(dxfc_dxendpoint_builder_t builderHandle) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    if (!dxfcpp::BuilderRegistry::remove(dxfcpp::bit_cast<dxfcpp::BuilderHandle *>(builderHandle))) {
        return DXFC_EC_ERROR;
    }

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_instance(void *userData, DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::getInstance();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_instance2(dxfc_dxendpoint_role_t role, void *userData,
                                                DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::getInstance(dxfcpp::cApiRoleToRole(role));

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_create(void *userData, DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::create();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_create2(dxfc_dxendpoint_role_t role, void *userData,
                                          DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::create(dxfcpp::cApiRoleToRole(role));

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_close(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->close();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_close_and_await_termination(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->closeAndAwaitTermination();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_role(dxfc_dxendpoint_t endpointHandle, DXFC_OUT dxfc_dxendpoint_role_t *role) {
    if (!endpointHandle || !role) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    *role = dxfcpp::roleToCApiRole(endpointWrapper->endpoint->getRole());

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_user(dxfc_dxendpoint_t endpointHandle, const char *user) {
    if (!endpointHandle || !user) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->user(user);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_password(dxfc_dxendpoint_t endpointHandle, const char *password) {
    if (!endpointHandle || !password) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->password(password);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_connect(dxfc_dxendpoint_t endpointHandle, const char *address) {
    if (!endpointHandle || !address) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->connect(address);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_reconnect(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->reconnect();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_disconnect(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->disconnect();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_disconnect_and_clear(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->disconnectAndClear();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_await_processed(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->awaitProcessed();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_await_not_connected(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->awaitNotConnected();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_state(dxfc_dxendpoint_t endpointHandle, DXFC_OUT dxfc_dxendpoint_state_t *state) {
    if (!endpointHandle || !state) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    *state = dxfcpp::stateToCApiState(endpointWrapper->endpoint->getState());

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_add_state_change_listener(dxfc_dxendpoint_t endpointHandle,
                                                            dxfc_dxendpoint_state_change_listener listener) {
    if (!endpointHandle || !listener) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    if (endpointWrapper->listeners.contains(listener)) {
        return DXFC_EC_SUCCESS;
    }

    endpointWrapper->listeners[listener] = endpointWrapper->endpoint->onStateChange() +=
        [userData = endpointWrapper->userData, listener](dxfcpp::DXEndpoint::State oldState,
                                                         dxfcpp::DXEndpoint::State newState) {
            listener(dxfcpp::stateToCApiState(oldState), dxfcpp::stateToCApiState(newState), userData);
        };

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_remove_state_change_listener(dxfc_dxendpoint_t endpointHandle,
                                                               dxfc_dxendpoint_state_change_listener listener) {
    if (!endpointHandle || !listener) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    if (!endpointWrapper->listeners.contains(listener)) {
        return DXFC_EC_SUCCESS;
    }

    endpointWrapper->endpoint->onStateChange() -= endpointWrapper->listeners[listener];

    return DXFC_EC_SUCCESS;
}

// TODO: implement
dxfc_error_code_t dxfc_dxendpoint_get_feed(dxfc_dxendpoint_t endpointHandle, DXFC_OUT dxfc_dxfeed_t *feed) {
    return DXFC_EC_SUCCESS;
}

// TODO: implement
dxfc_error_code_t dxfc_dxendpoint_get_publisher(dxfc_dxendpoint_t endpointHandle,
                                                DXFC_OUT dxfc_dxpublisher_t *publisher) {
    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_free(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    if (!dxfcpp::EndpointWrapperRegistry::remove(dxfcpp::bit_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle))) {
        return DXFC_EC_ERROR;
    }

    return DXFC_EC_SUCCESS;
}