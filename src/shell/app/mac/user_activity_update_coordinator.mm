// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "shell/app/mac/user_activity_update_coordinator.h"

#import <AppKit/AppKit.h>

#include <algorithm>

#include "base/check.h"

namespace lynxtron {

class UserActivityUpdateCoordinator::Request::Semaphore {
 public:
  Semaphore() : value_(dispatch_semaphore_create(0)) {}

  bool Wait(base::TimeDelta timeout) {
    const dispatch_time_t deadline =
        dispatch_time(DISPATCH_TIME_NOW, timeout.InNanoseconds());
    return dispatch_semaphore_wait(value_, deadline) == 0;
  }

  void Signal() { dispatch_semaphore_signal(value_); }

 private:
  dispatch_semaphore_t value_;
};

UserActivityUpdateCoordinator::Request::Request(ActivityId activity_id,
                                                bool can_wait)
    : activity_id_(activity_id),
      can_wait_(can_wait),
      semaphore_(can_wait ? std::make_unique<Semaphore>() : nullptr) {}

UserActivityUpdateCoordinator::Request::~Request() = default;

bool UserActivityUpdateCoordinator::Request::Wait(base::TimeDelta timeout) {
  CHECK(can_wait_);

  // A dispatch semaphore remembers a signal, so completion between the atomic
  // check and dispatch_semaphore_wait() is not lost. Returning the wait result
  // (instead of re-reading completed_ after a timeout) also gives the
  // one-second deadline a precise boundary: an update arriving after the
  // timeout remains a timeout for this save request.
  if (IsCompleted()) {
    return true;
  }
  return semaphore_->Wait(timeout);
}

bool UserActivityUpdateCoordinator::Request::IsCompleted() const {
  return completed_.load(std::memory_order_acquire);
}

void UserActivityUpdateCoordinator::Request::Complete() {
  // Multiple matching calls to updateCurrentActivity() are legal, but a request
  // is signaled only once. The release pairs with the waiting thread's acquire
  // load in the fast path.
  if (completed_.exchange(true, std::memory_order_acq_rel)) {
    return;
  }
  if (semaphore_) {
    semaphore_->Signal();
  }
}

UserActivityUpdateCoordinator::UserActivityUpdateCoordinator() = default;
UserActivityUpdateCoordinator::~UserActivityUpdateCoordinator() = default;

UserActivityUpdateCoordinator::RequestPtr
UserActivityUpdateCoordinator::BeginRequest(ActivityId activity_id,
                                            bool can_wait) {
  DCHECK([NSThread isMainThread]);
  RequestPtr request(new Request(activity_id, can_wait));
  pending_requests_.push_back(request);
  return request;
}

void UserActivityUpdateCoordinator::CompleteRequestsForActivity(
    ActivityId activity_id) {
  DCHECK([NSThread isMainThread]);
  for (const RequestPtr& request : pending_requests_) {
    // activityType is not an identity. setUserActivity() can replace the
    // current NSUserActivity with another object of the same type while the old
    // object is being saved. Completing by type would then report success for
    // the old object even though only the replacement received the new data.
    if (request->activity_id() == activity_id) {
      request->Complete();
    }
  }
}

void UserActivityUpdateCoordinator::EndRequest(const RequestPtr& request) {
  DCHECK([NSThread isMainThread]);
  std::erase(pending_requests_, request);
}

}  // namespace lynxtron
