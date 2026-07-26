// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "ui/base/resource/resource_scale_factor.h"

#include <array>
#include <vector>

namespace ui {

namespace {

constexpr std::array<float, NUM_SCALE_FACTORS> kResourceScaleFactorScales = {
    1.0f,
    1.0f,
    2.0f,
    3.0f,
};
static_assert(NUM_SCALE_FACTORS == kResourceScaleFactorScales.size(),
              "kScaleFactorScales has incorrect size");

// Resource scale factors are resource-pack/image-density buckets. With HiDPI
// enabled by default on desktop, Lynxtron supports 1x and 2x resources and
// rescales them for intermediate device scale factors.
const std::vector<ResourceScaleFactor>& SupportedResourceScaleFactorsStorage() {
  static const std::vector<ResourceScaleFactor> supported_scale_factors = {
      k100Percent, k200Percent};
  return supported_scale_factors;
}

const float kFallbackToSmallerScaleDiff = 0.20f;

}  // namespace

float GetScaleForResourceScaleFactor(ResourceScaleFactor scale_factor) {
  return kResourceScaleFactorScales[scale_factor];
}

const std::vector<ui::ResourceScaleFactor>& GetSupportedResourceScaleFactors() {
  return SupportedResourceScaleFactorsStorage();
}

ui::ResourceScaleFactor GetSupportedResourceScaleFactorForRescale(float scale) {
  // Returns an exact match, a smaller scale within
  // `kFallbackToSmallerScaleDiff` units, the nearest larger scale, or the max
  // supported scale.
  for (auto supported_scale : GetSupportedResourceScaleFactors()) {
    if (GetScaleForResourceScaleFactor(supported_scale) +
            kFallbackToSmallerScaleDiff >=
        scale) {
      return supported_scale;
    }
  }

  return GetSupportedResourceScaleFactors().back();
}

}  // namespace ui
