// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNXTRON_SHELL_API_USER_ACTIVITY_UPDATE_ERROR_H_
#define LYNXTRON_SHELL_API_USER_ACTIVITY_UPDATE_ERROR_H_

#include "base/values.h"
#include "shell/app/application_observer.h"

namespace lynxtron::api {

// Converts the native failure into the stable object exposed by
// update-activity-state-error. Keeping this conversion independent of App makes
// the public contract testable without constructing V8, Node, or NSApplication.
base::Value::Dict CreateUserActivityUpdateErrorValue(
    const UserActivityUpdateErrorDetails& details);

}  // namespace lynxtron::api

#endif  // LYNXTRON_SHELL_API_USER_ACTIVITY_UPDATE_ERROR_H_
