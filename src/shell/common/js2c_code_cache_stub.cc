// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "shell/common/js2c_code_cache.h"

#include "third_party/node/src/node_builtins.h"

namespace lynxtron {

const std::vector<node::builtins::CodeCacheInfo>&
Js2cStandardWrapperCodeCache() {
  static const std::vector<node::builtins::CodeCacheInfo> cache;
  return cache;
}

const std::vector<node::builtins::CodeCacheInfo>& Js2cCustomWrapperCodeCache() {
  static const std::vector<node::builtins::CodeCacheInfo> cache;
  return cache;
}

}  // namespace lynxtron
