// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "shell/app/mac/lynxtron_application.h"

#include <string>
#include <utility>

#include "base/auto_reset.h"
#include "base/observer_list.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "shell/app/application.h"
#include "shell/app/mac/dict_util.h"
#import "shell/app/mac/lynxtron_application_delegate.h"

namespace {

constexpr base::TimeDelta kUserActivityUpdateTimeout = base::Seconds(1);

lynxtron::UserActivityUpdateCoordinator::ActivityId GetUserActivityId(
    NSUserActivity* activity) {
  // The id is only an equality token. A pending request never outlives
  // userActivityWillSave:, so AppKit keeps the callback's activity alive for
  // the entire interval in which the token is stored. The coordinator never
  // dereferences this address.
  return reinterpret_cast<std::uintptr_t>((__bridge void*)activity);
}

void DispatchSyncMain(dispatch_block_t block) {
  if ([NSThread isMainThread]) {
    block();
  } else {
    dispatch_sync(dispatch_get_main_queue(), block);
  }
}

void PostUserActivityUpdateError(
    std::string activity_type,
    lynxtron::UserActivityUpdateErrorDetails details) {
  // Always post instead of emitting inline. By the time this notification is
  // handled, its request has been removed from the coordinator, making it
  // explicit that the event is diagnostic and cannot repair the native save.
  dispatch_async(dispatch_get_main_queue(), ^{
    lynxtron::Application::Get()->UserActivityUpdateFailed(activity_type,
                                                           details);
  });
}

}  // namespace

@implementation LynxtronApplication

+ (LynxtronApplication*)sharedApplication {
  return (LynxtronApplication*)[super sharedApplication];
}

- (void)willPowerOff:(NSNotification*)notify {
  userStoppedShutdown_ = shouldShutdown_ && !shouldShutdown_.Run();
}

- (void)terminate:(id)sender {
  // User will call Quit later.
  if (userStoppedShutdown_) {
    return;
  }

  // We simply try to close the browser, which in turn will try to close the
  // windows. Termination can proceed if all windows are closed or window close
  // can be cancelled which will abort termination.
  lynxtron::Application::Get()->Quit();
}

- (void)setShutdownHandler:(base::RepeatingCallback<bool()>)handler {
  shouldShutdown_ = std::move(handler);
}

- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)sendEvent:(NSEvent*)event {
  base::AutoReset<BOOL> scoper(&handlingSendEvent_, YES);
  [super sendEvent:event];
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL {
  currentActivity_ = [[NSUserActivity alloc] initWithActivityType:type];
  [currentActivity_ setUserInfo:userInfo];
  [currentActivity_ setWebpageURL:webpageURL];
  [currentActivity_ setDelegate:self];
  [currentActivity_ becomeCurrent];
  [currentActivity_ setNeedsSave:YES];
}

- (NSUserActivity*)getCurrentActivity {
  return currentActivity_;
}

- (void)invalidateCurrentActivity {
  if (currentActivity_) {
    [currentActivity_ invalidate];
    currentActivity_ = nil;
  }
}

- (void)resignCurrentActivity {
  if (currentActivity_) {
    [currentActivity_ resignCurrent];
  }
}

- (void)updateCurrentActivity:(NSString*)type
                 withUserInfo:(NSDictionary*)userInfo {
  DispatchSyncMain(^{
    if (!currentActivity_ ||
        ![currentActivity_.activityType isEqualToString:type]) {
      return;
    }

    [currentActivity_ addUserInfoEntriesFromDictionary:userInfo];

    // Complete only after the payload was successfully merged. Completing
    // before the type check or before the merge would release a save request
    // even though that request's data was not updated.
    userActivityUpdates_.CompleteRequestsForActivity(
        GetUserActivityId(currentActivity_));
  });
}

- (void)userActivityWillSave:(NSUserActivity*)userActivity {
  const bool callback_on_main_thread = [NSThread isMainThread];
  __block bool prevented = false;
  __block lynxtron::UserActivityUpdateCoordinator::RequestPtr request;

  DispatchSyncMain(^{
    std::string activity_type(
        base::SysNSStringToUTF8(userActivity.activityType));
    base::Value::Dict user_info =
        lynxtron::NSDictionaryToValue(userActivity.userInfo);

    // AppKit does not document one fixed callback thread for
    // userActivityWillSave:, while JavaScript and currentActivity_ are confined
    // to the main thread. The request must therefore be registered before
    // emitting the event, or a synchronous updateCurrentActivity() would be
    // lost (signal-before-reset). It is keyed by this concrete NSUserActivity
    // object, not activityType: setUserActivity() may replace currentActivity_
    // with a different object of the same type while this older object is still
    // being saved.
    //
    // Main-thread callback:
    //   The JS listener runs inline. It may call preventDefault() and
    //   updateCurrentActivity() synchronously. We never allocate a semaphore or
    //   wait here because blocking the main thread would also block the only
    //   thread capable of running JS.
    //
    // Background-thread callback:
    //   Event dispatch still runs synchronously on the main thread, but after
    //   it returns the AppKit callback thread may wait for up to one second.
    //   This preserves Electron's useful compatibility window for an
    //   asynchronous JS update without stalling the UI thread.
    //
    // The public contract should use the synchronous pattern for both cases,
    // because application code cannot know which thread AppKit selected. The
    // background wait is compatibility, not a promise that async work will
    // always complete.
    request = userActivityUpdates_.BeginRequest(GetUserActivityId(userActivity),
                                                !callback_on_main_thread);
    prevented = lynxtron::Application::Get()->UpdateUserActivityState(
        activity_type, std::move(user_info));

    if (!prevented) {
      // Without preventDefault(), the existing userInfo is the desired state;
      // no replacement update is required.
      userActivityUpdates_.EndRequest(request);
    }
  });

  if (prevented) {
    const bool updated = callback_on_main_thread
                             ? request->IsCompleted()
                             : request->Wait(kUserActivityUpdateTimeout);

    DispatchSyncMain(^{
      userActivityUpdates_.EndRequest(request);

      if (!updated) {
        lynxtron::UserActivityUpdateErrorDetails details;
        if (callback_on_main_thread) {
          // Throwing a JavaScript exception here would be misleading: the event
          // listener has already returned and there is no JS call frame to
          // throw into. Emit a dedicated asynchronous diagnostic instead.
          details.reason = lynxtron::UserActivityUpdateErrorReason::
              kSynchronousUpdateRequired;
        } else {
          details.reason =
              lynxtron::UserActivityUpdateErrorReason::kUpdateTimeout;
          details.timeout_ms =
              static_cast<int>(kUserActivityUpdateTimeout.InMilliseconds());
        }
        PostUserActivityUpdateError(
            base::SysNSStringToUTF8(userActivity.activityType), details);
      }
    });
  }

  [userActivity setNeedsSave:YES];
}

- (void)userActivityWasContinued:(NSUserActivity*)userActivity {
  dispatch_async(dispatch_get_main_queue(), ^{
    std::string activity_type(
        base::SysNSStringToUTF8(userActivity.activityType));
    base::Value::Dict user_info =
        lynxtron::NSDictionaryToValue(userActivity.userInfo);

    lynxtron::Application::Get()->UserActivityWasContinued(
        activity_type, std::move(user_info));
  });
  [userActivity setNeedsSave:YES];
}

- (void)registerURLHandler {
  [[NSAppleEventManager sharedAppleEventManager]
      setEventHandler:self
          andSelector:@selector(handleURLEvent:withReplyEvent:)
        forEventClass:kInternetEventClass
           andEventID:kAEGetURL];
}

- (void)handleURLEvent:(NSAppleEventDescriptor*)event
        withReplyEvent:(NSAppleEventDescriptor*)replyEvent {
  NSString* url =
      [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
  lynxtron::Application::Get()->OpenURL(base::SysNSStringToUTF8(url));
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString*)attribute {
  // Undocumented attribute that screen reader related functionality
  // sets when running.

  return [super accessibilitySetValue:value forAttribute:attribute];
}

- (void)orderFrontStandardAboutPanel:(id)sender {
  lynxtron::Application::Get()->ShowAboutPanel();
}

@end
