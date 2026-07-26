// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "shell/api/user_activity_update_error.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace lynxtron::api {

TEST(UserActivityUpdateErrorTest, ConvertsDetailsToJavaScriptValue) {
  const UserActivityUpdateErrorDetails synchronous_details{
      .reason = UserActivityUpdateErrorReason::kSynchronousUpdateRequired,
  };
  base::Value::Dict synchronous_error =
      CreateUserActivityUpdateErrorValue(synchronous_details);

  EXPECT_EQ(*synchronous_error.FindString("reason"),
            "synchronous-update-required");
  EXPECT_EQ(synchronous_error.Find("timeoutMs"), nullptr);

  const UserActivityUpdateErrorDetails timeout_details{
      .reason = UserActivityUpdateErrorReason::kUpdateTimeout,
      .timeout_ms = 1000,
  };
  base::Value::Dict timeout_error =
      CreateUserActivityUpdateErrorValue(timeout_details);

  EXPECT_EQ(*timeout_error.FindString("reason"), "update-timeout");
  ASSERT_TRUE(timeout_error.FindInt("timeoutMs"));
  EXPECT_EQ(*timeout_error.FindInt("timeoutMs"), 1000);
}

}  // namespace lynxtron::api
