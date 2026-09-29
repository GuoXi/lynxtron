// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNXTRON_SHELL_COMMON_JS2C_BUNDLE_IDS_H_
#define LYNXTRON_SHELL_COMMON_JS2C_BUNDLE_IDS_H_

#include <array>
#include <string_view>

#include "v8/include/v8-container.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-primitive.h"

namespace lynxtron::js2c {

// The generator and runtime must use the same bundle IDs and wrapper
// parameters. V8 includes both the source and parameters in its cache key.
inline constexpr char kBrowserInitId[] = "lynxtron/js2c/browser_init";
inline constexpr char kLynxBtsInitId[] = "lynxtron/js2c/lynxbts_init";
inline constexpr char kNodeInitId[] = "lynxtron/js2c/node_init";
inline constexpr std::array<std::string_view, 2> kNodeInitParams = {"process",
                                                                    "require"};

inline v8::LocalVector<v8::String> MakeNodeInitParams(v8::Isolate* isolate) {
  v8::LocalVector<v8::String> parameters(isolate);
  parameters.reserve(kNodeInitParams.size());
  for (std::string_view name : kNodeInitParams) {
    parameters.push_back(
        v8::String::NewFromUtf8(isolate, name.data(),
                                v8::NewStringType::kInternalized,
                                static_cast<int>(name.size()))
            .ToLocalChecked());
  }
  return parameters;
}

}  // namespace lynxtron::js2c

#endif  // LYNXTRON_SHELL_COMMON_JS2C_BUNDLE_IDS_H_
