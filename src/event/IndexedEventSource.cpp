// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>

namespace dxfcpp {

const IndexedEventSource IndexedEventSource::DEFAULT{0, "DEFAULT"};

void *IndexedEventSource::toGraal() const noexcept {
    auto *graalSource = new (std::nothrow) dxfg_indexed_event_source_t{INDEXED_EVENT_SOURCE, id_, createCString(name_)};

    return static_cast<void *>(graalSource);
}

void IndexedEventSource::freeGraal(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto *graalSource = static_cast<dxfg_indexed_event_source_t *>(graalNative);

    delete[] graalSource->name;
    delete graalSource;
}

IndexedEventSource IndexedEventSource::fromGraal(void *graalNative) noexcept {
    auto *graalSource = static_cast<dxfg_indexed_event_source_t *>(graalNative);

    if (graalSource == nullptr) {
        return {};
    }

    return {graalSource->id, graalSource->name};
}

} // namespace dxfcpp