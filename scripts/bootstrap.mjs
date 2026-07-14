import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

function commandRunner(command, args = ['--version']) {
  const completed = spawnSync(command, args, {
    encoding: 'utf8',
    shell: false,
    windowsHide: true,
  });
  if (completed.error || completed.status !== 0) {
    return {
      ok: false,
      detail: completed.error?.code === 'ENOENT'
        ? 'not found on PATH'
        : `not runnable (exit ${completed.status ?? 'unknown'})`,
    };
  }

  const output = `${completed.stdout ?? ''}\n${completed.stderr ?? ''}`
    .split(/\r?\n/)
    .map((line) => line.trim())
    .find(Boolean);
  return { ok: true, detail: output ?? 'available' };
}

function majorVersion(version) {
  const match = String(version).match(/^v?(\d+)/);
  return match ? Number(match[1]) : null;
}

function pnpmVersionFromUserAgent(userAgent) {
  const match = String(userAgent ?? '').match(/(?:^|\s)pnpm\/([^\s]+)/);
  return match?.[1] ?? null;
}

function requiredMajor(label, required, version) {
  const actual = majorVersion(version);
  return actual === required
    ? { label, ok: true, detail: version }
    : {
        label,
        ok: false,
        detail: `requires major ${required}; found ${version || 'an unreadable version'}`,
      };
}

function requiredCommand(label, command, runner, args = ['--version']) {
  const result = runner(command, args);
  return { label, ok: result.ok, detail: result.detail };
}

function firstAvailable(label, candidates, runner) {
  const attempted = [];
  for (const [command, args] of candidates) {
    if (!command || attempted.includes(command)) {
      continue;
    }
    attempted.push(command);
    const result = runner(command, args);
    if (result.ok) {
      return { label, ok: true, detail: `${command}: ${result.detail}` };
    }
  }
  return {
    label,
    ok: false,
    detail: `none found (${attempted.join(', ')})`,
  };
}

export function checkPrerequisites({
  nodeVersion = process.versions.node,
  runner = commandRunner,
  environment = process.env,
} = {}) {
  const pnpmFromEnvironment = pnpmVersionFromUserAgent(environment.npm_config_user_agent);
  const pnpm = pnpmFromEnvironment
    ? { ok: true, detail: pnpmFromEnvironment }
    : runner('pnpm', ['--version']);
  const results = [
    requiredMajor('Node 24', 24, nodeVersion),
    pnpm.ok
      ? requiredMajor('pnpm 11', 11, pnpm.detail)
      : { label: 'pnpm 11', ok: false, detail: pnpm.detail },
    requiredCommand('CMake', 'cmake', runner),
    requiredCommand('Ninja', 'ninja', runner),
    firstAvailable('C++ compiler', [
      [environment.CXX, ['--version']],
      ['c++', ['--version']],
      ['clang++', ['--version']],
      ['g++', ['--version']],
      ['cl', []],
    ], runner),
    firstAvailable('Emscripten', [
      [environment.EMXX, ['--version']],
      ['em++', ['--version']],
      ['emcc', ['--version']],
    ], runner),
  ];
  return results;
}

export function formatReport(results) {
  const lines = ['Sudoku Reasoning Radar bootstrap diagnostic'];
  for (const result of results) {
    lines.push(`[${result.ok ? 'OK' : 'MISSING'}] ${result.label}: ${result.detail}`);
  }
  const missing = results.filter((result) => !result.ok).length;
  lines.push(missing === 0
    ? 'All prerequisites are available.'
    : `${missing} prerequisite group(s) need attention. Nothing was installed.`);
  return lines.join('\n');
}

const isMain = process.argv[1]
  && path.resolve(process.argv[1]) === path.resolve(fileURLToPath(import.meta.url));

if (isMain) {
  const results = checkPrerequisites();
  console.log(formatReport(results));
  process.exitCode = results.every((result) => result.ok) ? 0 : 1;
}
