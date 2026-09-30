// Copyright 2026 The Lynxtron Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { expect } from 'chai';
import * as childProcess from 'node:child_process';
import * as fs from 'node:fs';
import * as http from 'node:http';
import * as https from 'node:https';
import * as path from 'node:path';
import { promisify } from 'node:util';

import { listen } from './lib/spec-helpers';

interface FixtureResult {
  startupModules: string[];
  bindingsBeforeMenu: string[];
  menuResult: {
    label: string;
    applicationMenu: null;
    showErrorBox: string;
    fork: string;
  };
  file: { statusCode: number; nonempty: boolean };
  fileModules: string[];
  http: { statusCode: number; body: string };
  httpModules: string[];
  https: { statusCode: number; body: string };
  httpsModules: string[];
}

describe('lazy module loading', () => {
  let result: FixtureResult;
  const servers: http.Server[] = [];
  const fixturePath = path.join(__dirname, 'fixtures', 'lazy-module-loading');

  before(async function () {
    this.timeout(45000);
    // Trust the fixture certificate only in the child process. The regular
    // spec runner already loads network modules, so inspect a fresh process.
    const certificatePath = path.join(fixturePath, 'cert.pem');
    const secureServer = https.createServer(
      {
        key: fs.readFileSync(path.join(fixturePath, 'key.pem')),
        cert: fs.readFileSync(certificatePath),
      },
      (_request, response) => response.end('secure-resource')
    );
    servers.push(secureServer);
    const { url: secureURL } = await listen(secureServer);
    const server = http.createServer((request, response) => {
      if (request.url === '/upgrade') {
        response.writeHead(302, {
          location: secureURL.replace('http:', 'https:'),
        });
        response.end();
      } else {
        response.end('network-resource');
      }
    });
    servers.push(server);
    const { url } = await listen(server);
    const env: NodeJS.ProcessEnv = {
      ...process.env,
      NODE_EXTRA_CA_CERTS: certificatePath,
    };
    delete env.LYNXTRON_RUN_AS_NODE;
    const { stdout } = await promisify(childProcess.execFile)(
      process.execPath,
      [fixturePath, url, `${url}/upgrade`],
      { env, timeout: 30000 }
    );
    const prefix = 'LAZY_MODULE_RESULT ';
    const line = stdout.split(/\r?\n/).find((line) => line.startsWith(prefix));
    expect(line, stdout).to.be.a('string');
    result = JSON.parse(line!.slice(prefix.length));
  });

  after(async () => {
    await Promise.all(
      servers.map(
        (server) =>
          new Promise<void>((resolve, reject) => {
            if (!server.listening) return resolve();
            server.close((error) => (error ? reject(error) : resolve()));
          })
      )
    );
  });

  it('does not load network modules in the default launcher or API getters', () => {
    expect(result.startupModules).to.deep.equal([]);
    expect(result.bindingsBeforeMenu).to.include('lynxtron_lynx_window');
    expect(result.bindingsBeforeMenu).to.include('lynxtron_binding_dialog');
    expect(result.bindingsBeforeMenu).not.to.include('lynxtron_binding_menu');
    expect(result.bindingsBeforeMenu).not.to.include(
      'lynxtron_binding_power_monitor'
    );
    expect(result.menuResult).to.deep.equal({
      label: 'Lazy item',
      applicationMenu: null,
      showErrorBox: 'function',
      fork: 'function',
    });
  });

  it('reads local resources without loading network modules', () => {
    expect(result.file).to.deep.equal({ statusCode: 0, nonempty: true });
    expect(result.fileModules).to.deep.equal([]);
  });

  it('loads HTTP without loading HTTPS, TLS or crypto', () => {
    expect(result.http).to.deep.equal({
      statusCode: 0,
      body: 'network-resource',
    });
    expect(result.httpModules).to.deep.equal(['http']);
  });

  it('loads HTTPS, TLS and crypto for a redirect to a secure resource', () => {
    expect(result.https).to.deep.equal({
      statusCode: 0,
      body: 'secure-resource',
    });
    expect(result.httpsModules).to.deep.equal([
      'http',
      'https',
      'tls',
      'crypto',
    ]);
  });
});
