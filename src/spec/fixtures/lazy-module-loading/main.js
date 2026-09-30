// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const lynxtron = require('lynxtron');
const { app } = lynxtron;
const networkModules = ['http', 'https', 'tls', 'crypto'];
const loadedNetworkModules = () =>
  networkModules.filter((name) =>
    process.moduleLoadList.includes(`NativeModule ${name}`)
  );
const bindings = [];
const originalLinkedBinding = process._linkedBinding;
process._linkedBinding = function (name) {
  bindings.push(name);
  return originalLinkedBinding.call(this, name);
};

const startupModules = loadedNetworkModules();
const { LynxWindow, MenuItem, dialog, utilityProcess } = lynxtron;
const item = new MenuItem({ label: 'Lazy item' });
const bindingsBeforeMenu = [...bindings];
const menu = new lynxtron.Menu();
menu.append(item);
const menuResult = {
  label: menu.items[0].label,
  applicationMenu: app.applicationMenu,
  showErrorBox: typeof dialog.showErrorBox,
  fork: typeof utilityProcess.fork,
};

// Keep the fixture alive until all resource requests have finished.
app.on('window-all-closed', () => {});
app
  .whenReady()
  .then(async () => {
    const window = new LynxWindow({ show: false });
    const fetchResource = (url) =>
      new Promise((resolve) => {
        window.emit('-on-fetch-resource', { sendReply: resolve }, 'raw', url);
      });
    const fileURL = require('node:url').pathToFileURL(__filename).href;
    const file = await fetchResource(fileURL);
    const fileModules = loadedNetworkModules();
    const http = await fetchResource(process.argv[2]);
    const httpModules = loadedNetworkModules();
    // This URL redirects from HTTP to HTTPS, testing protocol selection again
    // for the redirected request rather than only the initial request.
    const https = await fetchResource(process.argv[3]);
    const httpsModules = loadedNetworkModules();
    const result = {
      startupModules,
      bindingsBeforeMenu,
      menuResult,
      file: { statusCode: file.statusCode, nonempty: file.data.length > 0 },
      fileModules,
      http: { statusCode: http.statusCode, body: http.data.toString() },
      httpModules,
      https: { statusCode: https.statusCode, body: https.data.toString() },
      httpsModules,
    };
    window.destroy();
    process.stdout.write(
      `LAZY_MODULE_RESULT ${JSON.stringify(result)}\n`,
      () => {
        app.exit(0);
      }
    );
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
