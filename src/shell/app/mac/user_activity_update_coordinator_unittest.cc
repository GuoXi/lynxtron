// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "shell/app/mac/user_activity_update_coordinator.h"

#include <thread>

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace lynxtron {

namespace {

constexpr UserActivityUpdateCoordinator::ActivityId kFirstActivity = 1;
constexpr UserActivityUpdateCoordinator::ActivityId kSecondActivity = 2;

}  // namespace

TEST(UserActivityUpdateCoordinatorTest, MainRequestCanCompleteSynchronously) {
  UserActivityUpdateCoordinator coordinator;
  auto request = coordinator.BeginRequest(kFirstActivity, false);

  coordinator.CompleteRequestsForActivity(kFirstActivity);

  EXPECT_TRUE(request->IsCompleted());
  coordinator.EndRequest(request);
}

TEST(UserActivityUpdateCoordinatorTest, SignalBeforeBackgroundWaitIsRetained) {
  UserActivityUpdateCoordinator coordinator;
  auto request = coordinator.BeginRequest(kFirstActivity, true);

  coordinator.CompleteRequestsForActivity(kFirstActivity);

  EXPECT_TRUE(request->Wait(base::Milliseconds(10)));
  coordinator.EndRequest(request);
}

TEST(UserActivityUpdateCoordinatorTest,
     PreviousUpdateDoesNotCompleteLaterRequest) {
  UserActivityUpdateCoordinator coordinator;
  auto previous = coordinator.BeginRequest(kFirstActivity, false);
  coordinator.CompleteRequestsForActivity(kFirstActivity);
  coordinator.EndRequest(previous);

  auto later = coordinator.BeginRequest(kFirstActivity, false);

  EXPECT_FALSE(later->IsCompleted());
  coordinator.EndRequest(later);
}

TEST(UserActivityUpdateCoordinatorTest, BackgroundRequestWaitsForAsyncUpdate) {
  UserActivityUpdateCoordinator coordinator;
  auto request = coordinator.BeginRequest(kFirstActivity, true);

  std::thread waiter(
      [request] { EXPECT_TRUE(request->Wait(base::Seconds(1))); });
  coordinator.CompleteRequestsForActivity(kFirstActivity);
  waiter.join();
  coordinator.EndRequest(request);
}

TEST(UserActivityUpdateCoordinatorTest, BackgroundRequestTimesOut) {
  UserActivityUpdateCoordinator coordinator;
  auto request = coordinator.BeginRequest(kFirstActivity, true);

  EXPECT_FALSE(request->Wait(base::Milliseconds(1)));
  coordinator.EndRequest(request);
}

TEST(UserActivityUpdateCoordinatorTest, LateUpdateDoesNotReviveEndedRequest) {
  UserActivityUpdateCoordinator coordinator;
  auto request = coordinator.BeginRequest(kFirstActivity, true);
  EXPECT_FALSE(request->Wait(base::Milliseconds(1)));
  coordinator.EndRequest(request);

  coordinator.CompleteRequestsForActivity(kFirstActivity);

  EXPECT_FALSE(request->IsCompleted());
}

TEST(UserActivityUpdateCoordinatorTest,
     ReplacementActivityDoesNotCompleteOldRequest) {
  UserActivityUpdateCoordinator coordinator;
  auto old_activity = coordinator.BeginRequest(kFirstActivity, false);

  // Models setUserActivity() replacing the object with another activity of the
  // same type, followed by updateCurrentActivity() updating the replacement.
  coordinator.CompleteRequestsForActivity(kSecondActivity);

  EXPECT_FALSE(old_activity->IsCompleted());
  coordinator.EndRequest(old_activity);
}

TEST(UserActivityUpdateCoordinatorTest,
     BackgroundReplacementActivityDoesNotWakeOldRequest) {
  UserActivityUpdateCoordinator coordinator;
  auto old_activity = coordinator.BeginRequest(kFirstActivity, true);

  coordinator.CompleteRequestsForActivity(kSecondActivity);

  EXPECT_FALSE(old_activity->Wait(base::Milliseconds(1)));
  coordinator.EndRequest(old_activity);
}

TEST(UserActivityUpdateCoordinatorTest, CompletesAllMatchingRequests) {
  UserActivityUpdateCoordinator coordinator;
  auto first = coordinator.BeginRequest(kFirstActivity, false);
  auto second = coordinator.BeginRequest(kFirstActivity, true);

  coordinator.CompleteRequestsForActivity(kFirstActivity);

  EXPECT_TRUE(first->IsCompleted());
  EXPECT_TRUE(second->Wait(base::Milliseconds(10)));
  coordinator.EndRequest(first);
  coordinator.EndRequest(second);
}

}  // namespace lynxtron
