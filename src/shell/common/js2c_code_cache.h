// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNXTRON_SHELL_COMMON_JS2C_CODE_CACHE_H_
#define LYNXTRON_SHELL_COMMON_JS2C_CODE_CACHE_H_

#include <vector>

namespace node::builtins {
struct CodeCacheInfo;
}

namespace lynxtron {

// Generated against the embedded Node snapshot. These lists are empty when
// code-cache generation is disabled for a cross-OS build.
const std::vector<node::builtins::CodeCacheInfo>&
Js2cStandardWrapperCodeCache();
const std::vector<node::builtins::CodeCacheInfo>& Js2cCustomWrapperCodeCache();

}  // namespace lynxtron

#endif  // LYNXTRON_SHELL_COMMON_JS2C_CODE_CACHE_H_
