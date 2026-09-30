// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "shell/app/main_parts.h"

#include "shell/app/mac/lynxtron_application.h"
#include "shell/app/mac/lynxtron_application_delegate.h"

namespace lynxtron {

static LynxtronApplicationDelegate* __strong delegate_;

void MainParts::InitializeMacMainMessageLoop() {
  // Set our own application delegate.
  delegate_ = [[LynxtronApplicationDelegate alloc] init];
  [NSApp setDelegate:delegate_];

  RegisterURLHandler();

  // Prevent Cocoa from turning command-line arguments into
  // |-application:openFiles:|, since we already handle them directly.
  [[NSUserDefaults standardUserDefaults]
      setObject:@"NO"
         forKey:@"NSTreatUnknownArgumentsAsOpen"];
}

void MainParts::FreeAppDelegate() {
  delegate_ = nil;
  [NSApp setDelegate:nil];
}

void MainParts::RegisterURLHandler() {
  [[LynxtronApplication sharedApplication] registerURLHandler];
}

void MainParts::RegisterAtomCrApp() {
  // Force the NSApplication subclass to be used.
  [LynxtronApplication sharedApplication];
}

}  // namespace lynxtron
