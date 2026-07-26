// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "shell/api/user_activity_update_error.h"

namespace lynxtron::api {

base::Value::Dict CreateUserActivityUpdateErrorValue(
    const UserActivityUpdateErrorDetails& details) {
  base::Value::Dict error;
  switch (details.reason) {
    case UserActivityUpdateErrorReason::kSynchronousUpdateRequired:
      error.Set("reason", "synchronous-update-required");
      break;
    case UserActivityUpdateErrorReason::kUpdateTimeout:
      error.Set("reason", "update-timeout");
      break;
  }
  if (details.timeout_ms) {
    error.Set("timeoutMs", *details.timeout_ms);
  }
  return error;
}

}  // namespace lynxtron::api
