# @lynx-js/cef-webview

A `<webview>` element implementation based on Chromium Embedded Framework (CEF) for Lynxtron.

## Overview

This library provides a CEF-based implementation of the `<webview>` element for Lynxtron applications. It allows you to embed Chromium-based web content within your Lynxtron app, providing a full-featured web browsing experience.

## Installation

```bash
npm install @lynx-js/cef-webview
```

## Usage

Enable the Lynxtron autolink plugin in your application build. AutoLink requires
`@lynx-js/cef-webview/lynxtron`, which loads the current platform's Lynxtron
addon so its static Lynx registrations run during startup. CEF itself is
initialized only when you call `initialize()`.

```ts
import cefWebview from '@lynx-js/cef-webview/lynxtron';

cefWebview.initialize();
```

Once initialized, you can use the `<webview>` element in your Lynx templates:

```xml
<webview src="https://www.example.com" width="100%" height="500px"></webview>
```

## Building

### Prerequisites

- Node.js matching this package's engines
- CMake
- CEF SDK

### Build Steps

After preparing the repository's native dependencies, run the package build
from `src`:

```bash
node tools/yarn.js workspace @lynx-js/cef-webview build
```

This builds the addon and stages runtime files in `dist/<platform>/<arch>`.
On macOS it also preserves the CEF Framework links; the install script restores
those links when npm packaging omits them.

On Windows, build the Lynxtron source runtime first, then run the same entry used
by CI and release builds from the repository root:

```powershell
.\lynxtron_tools\build_cef_webview.ps1 -Arch x64
```

The script syncs CEF dependencies and passes `out/Release/lynxtron.dll.lib`
explicitly to the package build. Use `-ImportLibrary <path>` for another
source-build output. A missing import library fails before compilation instead
of falling back to a downloaded runtime.

## Dependencies

- **Runtime Dependencies:**
  - js-yaml
  - plist

- **Development Dependencies:**
  - node-addon-api
  - cmake-js
  - lynxtron

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Authors

Lynxtron Authors
