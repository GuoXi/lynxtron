// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNXTRON_SHELL_APP_MAC_USER_ACTIVITY_UPDATE_COORDINATOR_H_
#define LYNXTRON_SHELL_APP_MAC_USER_ACTIVITY_UPDATE_COORDINATOR_H_

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "base/time/time.h"

namespace lynxtron {

// Coordinates one or more NSUserActivity save requests with
// updateCurrentActivity().
//
// The coordinator deliberately has no global lock:
//
// * The pending request list is only accessed on the main thread, where both
//   JavaScript event dispatch and updateCurrentActivity() run.
// * A Request's completion flag is atomic because a background AppKit delegate
//   thread may be waiting while the main thread completes the request.
// * Only a background request owns a semaphore. Main-thread requests must never
//   wait because waiting there would prevent JavaScript from making progress.
//
// Requests are tracked independently and keyed by concrete activity identity,
// rather than using either a single "update received" bit or activityType.
// This prevents:
//
// * an update for a previous save from satisfying a later save (the
//   signal-before-reset race in the old Electron-style design); and
// * an update to a replacement activity from satisfying a save of an older,
//   different NSUserActivity object that happens to have the same type.
class UserActivityUpdateCoordinator {
 public:
  // An opaque identity for one concrete NSUserActivity object. The coordinator
  // never dereferences it; it is only compared for equality while the native
  // save callback keeps that NSUserActivity alive.
  using ActivityId = std::uintptr_t;

  class Request {
   public:
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;
    ~Request();

    ActivityId activity_id() const { return activity_id_; }

    // Returns immediately for an already completed request. Otherwise waits
    // for at most |timeout|. This is valid only for a request created with
    // |can_wait| == true.
    bool Wait(base::TimeDelta timeout);

    // Used by the main-thread path after the synchronous JS event returns.
    bool IsCompleted() const;

   private:
    friend class UserActivityUpdateCoordinator;

    Request(ActivityId activity_id, bool can_wait);
    void Complete();

    const ActivityId activity_id_;
    const bool can_wait_;
    std::atomic_bool completed_{false};

    // dispatch_semaphore_t is kept out of this C++ header so callers do not
    // need to depend on Objective-C dispatch ownership rules.
    class Semaphore;
    std::unique_ptr<Semaphore> semaphore_;
  };

  using RequestPtr = std::shared_ptr<Request>;

  UserActivityUpdateCoordinator();
  UserActivityUpdateCoordinator(const UserActivityUpdateCoordinator&) = delete;
  UserActivityUpdateCoordinator& operator=(
      const UserActivityUpdateCoordinator&) = delete;
  ~UserActivityUpdateCoordinator();

  // These three methods are main-thread-only. Keeping the vector confined to
  // one sequence is the reason no mutex is necessary.
  RequestPtr BeginRequest(ActivityId activity_id, bool can_wait);
  void CompleteRequestsForActivity(ActivityId activity_id);
  void EndRequest(const RequestPtr& request);

 private:
  std::vector<RequestPtr> pending_requests_;
};

}  // namespace lynxtron

#endif  // LYNXTRON_SHELL_APP_MAC_USER_ACTIVITY_UPDATE_COORDINATOR_H_
