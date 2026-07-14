import assert from 'node:assert/strict';
import test from 'node:test';

import { checkPrerequisites, formatReport } from './bootstrap.mjs';

test('bootstrap reports every missing prerequisite in one run', () => {
  const attempted = [];
  const runner = (command) => {
    attempted.push(command);
    return { ok: false, detail: 'not found' };
  };

  const results = checkPrerequisites({ nodeVersion: '23.0.0', runner, environment: {} });
  const report = formatReport(results);

  assert.equal(results.length, 6);
  assert.deepEqual(results.map((result) => result.ok), [false, false, false, false, false, false]);
  assert.match(report, /Node 24/);
  assert.match(report, /pnpm 11/);
  assert.match(report, /CMake/);
  assert.match(report, /Ninja/);
  assert.match(report, /C\+\+ compiler/);
  assert.match(report, /Emscripten/);
  assert.ok(attempted.includes('pnpm'));
  assert.ok(attempted.includes('cmake'));
  assert.ok(attempted.includes('ninja'));
  assert.ok(attempted.includes('cl'));
  assert.ok(attempted.includes('clang++'));
  assert.ok(attempted.includes('g++'));
  assert.ok(attempted.includes('c++'));
  assert.ok(attempted.includes('em++'));
  assert.ok(attempted.includes('emcc'));
});

test('bootstrap accepts the required major versions', () => {
  const versions = new Map([
    ['pnpm', '11.2.0'],
    ['cmake', 'cmake version 4.3.4'],
    ['ninja', '1.13.2'],
    ['cl', 'Microsoft C/C++ 19.44'],
    ['em++', 'emcc 4.0.10'],
  ]);
  const runner = (command) => versions.has(command)
    ? { ok: true, detail: versions.get(command) }
    : { ok: false, detail: 'not found' };

  const results = checkPrerequisites({ nodeVersion: '24.18.0', runner, environment: {} });

  assert.equal(results.every((result) => result.ok), true);
});

test('bootstrap reads pnpm version from the standard pnpm user agent', () => {
  const attempted = [];
  const runner = (command) => {
    attempted.push(command);
    return command === 'pnpm'
      ? { ok: false, detail: 'Windows cmd shim cannot be spawned directly' }
      : { ok: true, detail: 'available' };
  };

  const results = checkPrerequisites({
    nodeVersion: '24.18.0',
    runner,
    environment: { npm_config_user_agent: 'pnpm/11.7.0 npm/? node/v24.18.0 win32 x64' },
  });

  assert.equal(results[1].ok, true);
  assert.equal(results[1].detail, '11.7.0');
  assert.equal(attempted.includes('pnpm'), false);
});
