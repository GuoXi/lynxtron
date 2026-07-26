// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "shell/app/application.h"

#include <string>
#include <string_view>
#include <utility>

#include "shell/app/application_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace lynxtron {

namespace {

void ExpectStringValue(const base::Value::Dict& dict,
                       std::string_view key,
                       std::string_view expected) {
  const std::string* value = dict.FindString(key);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(*value, expected);
}

class RecordingApplicationObserver : public ApplicationObserver {
 public:
  void OnWillContinueUserActivity(bool* prevent_default,
                                  const std::string& type) override {
    will_continue_type = type;
    if (prevent_will_continue) {
      *prevent_default = true;
    }
  }

  void OnDidFailToContinueUserActivity(const std::string& type,
                                       const std::string& error) override {
    failed_type = type;
    failed_error = error;
  }

  void OnContinueUserActivity(bool* prevent_default,
                              const std::string& type,
                              base::Value::Dict user_info,
                              base::Value::Dict details) override {
    continue_type = type;
    continue_user_info = std::move(user_info);
    continue_details = std::move(details);
    if (prevent_continue) {
      *prevent_default = true;
    }
  }

  void OnUserActivityWasContinued(const std::string& type,
                                  base::Value::Dict user_info) override {
    continued_type = type;
    continued_user_info = std::move(user_info);
  }

  void OnUpdateUserActivityState(bool* prevent_default,
                                 const std::string& type,
                                 base::Value::Dict user_info) override {
    update_type = type;
    update_user_info = std::move(user_info);
    if (prevent_update) {
      *prevent_default = true;
    }
  }

  void OnUpdateUserActivityStateError(
      const std::string& type,
      const UserActivityUpdateErrorDetails& details) override {
    update_error_type = type;
    update_error_details = details;
  }

  bool prevent_will_continue = false;
  bool prevent_continue = false;
  bool prevent_update = false;
  std::string will_continue_type;
  std::string failed_type;
  std::string failed_error;
  std::string continue_type;
  base::Value::Dict continue_user_info;
  base::Value::Dict continue_details;
  std::string continued_type;
  base::Value::Dict continued_user_info;
  std::string update_type;
  base::Value::Dict update_user_info;
  std::string update_error_type;
  std::optional<UserActivityUpdateErrorDetails> update_error_details;
};

class ApplicationUserActivityTest : public testing::Test {
 protected:
  void SetUp() override { application_.AddObserver(&observer_); }

  void TearDown() override { application_.RemoveObserver(&observer_); }

  Application application_;
  RecordingApplicationObserver observer_;
};

TEST_F(ApplicationUserActivityTest, WillContinueUserActivity) {
  EXPECT_FALSE(
      application_.WillContinueUserActivity("com.lynxtron.activity.read"));
  EXPECT_EQ(observer_.will_continue_type, "com.lynxtron.activity.read");

  observer_.prevent_will_continue = true;
  EXPECT_TRUE(
      application_.WillContinueUserActivity("com.lynxtron.activity.write"));
  EXPECT_EQ(observer_.will_continue_type, "com.lynxtron.activity.write");
}

TEST_F(ApplicationUserActivityTest, DidFailToContinueUserActivity) {
  application_.DidFailToContinueUserActivity("com.lynxtron.activity",
                                             "The activity expired");

  EXPECT_EQ(observer_.failed_type, "com.lynxtron.activity");
  EXPECT_EQ(observer_.failed_error, "The activity expired");
}

TEST_F(ApplicationUserActivityTest, ContinueUserActivity) {
  base::Value::Dict user_info;
  user_info.Set("documentId", "42");
  base::Value::Dict details;
  details.Set("webpageURL", "https://example.com/document/42");

  observer_.prevent_continue = true;
  EXPECT_TRUE(application_.ContinueUserActivity(
      "com.lynxtron.activity", std::move(user_info), std::move(details)));

  EXPECT_EQ(observer_.continue_type, "com.lynxtron.activity");
  ExpectStringValue(observer_.continue_user_info, "documentId", "42");
  ExpectStringValue(observer_.continue_details, "webpageURL",
                    "https://example.com/document/42");
}

TEST_F(ApplicationUserActivityTest, UserActivityWasContinued) {
  base::Value::Dict user_info;
  user_info.Set("documentId", "42");

  application_.UserActivityWasContinued("com.lynxtron.activity",
                                        std::move(user_info));

  EXPECT_EQ(observer_.continued_type, "com.lynxtron.activity");
  ExpectStringValue(observer_.continued_user_info, "documentId", "42");
}

TEST_F(ApplicationUserActivityTest, UpdateUserActivityState) {
  base::Value::Dict user_info;
  user_info.Set("documentId", "42");

  observer_.prevent_update = true;
  EXPECT_TRUE(application_.UpdateUserActivityState("com.lynxtron.activity",
                                                   std::move(user_info)));

  EXPECT_EQ(observer_.update_type, "com.lynxtron.activity");
  ExpectStringValue(observer_.update_user_info, "documentId", "42");
}

TEST_F(ApplicationUserActivityTest, UserActivityUpdateFailed) {
  UserActivityUpdateErrorDetails details{
      .reason = UserActivityUpdateErrorReason::kUpdateTimeout,
      .timeout_ms = 1000,
  };

  application_.UserActivityUpdateFailed("com.lynxtron.activity", details);

  EXPECT_EQ(observer_.update_error_type, "com.lynxtron.activity");
  ASSERT_TRUE(observer_.update_error_details);
  EXPECT_EQ(observer_.update_error_details->reason,
            UserActivityUpdateErrorReason::kUpdateTimeout);
  EXPECT_EQ(observer_.update_error_details->timeout_ms, 1000);
}

}  // namespace

}  // namespace lynxtron
