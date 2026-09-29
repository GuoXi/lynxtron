// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

const { app, LynxWindow } = require('lynxtron');
const { once } = require('node:events');
const path = require('node:path');

app
  .whenReady()
  .then(async () => {
    const window = new LynxWindow({
      width: 800,
      height: 600,
      lynxPreference: {
        preload: path.resolve(
          __dirname,
          '../../case/lynx-card/src/lynx-node-bts-await/preload.js'
        ),
      },
    });
    const message = once(window, '-lynx-message');
    const bundle = path.resolve(
      __dirname,
      '../../case/lynx-card/dist/lynx-node-bts-await.lynx.bundle'
    );
    if (!(await window.loadFile(bundle))) {
      throw new Error('Failed to load Lynx BTS fixture');
    }
    await window.sendGlobalEvent('node_event', { msg: 'cache' });
    const [method, params] = await message;
    process.stdout.write(
      `NODE_BTS_RESULT ${JSON.stringify({ method, from: params.from })}\n`,
      () => app.exit(0)
    );
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
