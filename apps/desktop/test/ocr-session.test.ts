import assert from "node:assert/strict";
import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import { once } from "node:events";
import test from "node:test";

import { recognizeWithSidecar } from "../src/ocr-session.ts";

const puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
const cells = Array.from(puzzle, (value) => ({ digit: Number(value), confidence: value === "0" ? 0 : 92.5, lowConfidence: false }));

function childScript(body: string): string {
  return `let input='';process.stdin.setEncoding('utf8');process.stdin.on('data',c=>input+=c);process.stdin.on('end',()=>{${body}});`;
}

async function exited(child: ChildProcessWithoutNullStreams): Promise<void> {
  if (child.exitCode !== null || child.signalCode !== null) return;
  await once(child, "exit");
}

test("OCR uses one versioned JSON-line request and cleans up after success", async () => {
  let child: ChildProcessWithoutNullStreams | undefined;
  const result = await recognizeWithSidecar("C:\\chosen-by-dialog\\grid.png", {
    spawnHelper: () => (child = spawn(process.execPath, ["-e", childScript(`
      const request=JSON.parse(input.trim());
      if(request.version!==1||request.operation!=='recognize')process.exit(9);
      process.stdout.write(JSON.stringify({version:1,ok:true,puzzle:${JSON.stringify(puzzle)},cells:${JSON.stringify(cells)}})+'\\n');
    `)])),
    timeoutMs: 1_000,
    maxOutputBytes: 64 * 1024,
  });
  assert.equal(result.status, "ok");
  assert.equal(result.puzzle, puzzle);
  assert.equal(result.cells?.length, 81);
  assert.ok(child);
  await exited(child);
  assert.notEqual(child.exitCode ?? child.signalCode, null);
});

test("timeout terminates only the OCR helper session and cleans it up", async () => {
  let child: ChildProcessWithoutNullStreams | undefined;
  const result = await recognizeWithSidecar("grid.png", {
    spawnHelper: () => (child = spawn(process.execPath, ["-e", "setInterval(()=>{},1000)"])),
    timeoutMs: 40,
    maxOutputBytes: 1024,
  });
  assert.deepEqual(result, { version: 1, status: "error", code: "timeout" });
  assert.ok(child);
  await exited(child);
  assert.notEqual(child.signalCode, null);
});

test("crash, malformed JSON, oversized output, and schema mismatch are isolated", async () => {
  const cases = [
    { code: "crash", source: "process.exit(7)", max: 1024 },
    { code: "malformed-response", source: childScript("process.stdout.write('not-json\\n')"), max: 1024 },
    { code: "oversized-response", source: childScript("process.stdout.write('x'.repeat(2048))"), max: 128 },
    { code: "schema-mismatch", source: childScript("process.stdout.write(JSON.stringify({version:2,ok:true})+'\\n')"), max: 1024 },
  ] as const;
  for (const entry of cases) {
    let child: ChildProcessWithoutNullStreams | undefined;
    const result = await recognizeWithSidecar("grid.png", {
      spawnHelper: () => (child = spawn(process.execPath, ["-e", entry.source])),
      timeoutMs: 1_000,
      maxOutputBytes: entry.max,
    });
    assert.deepEqual(result, { version: 1, status: "error", code: entry.code });
    assert.ok(child);
    await exited(child);
  }

  const recovery = await recognizeWithSidecar("grid.png", {
    spawnHelper: () => spawn(process.execPath, ["-e", childScript(`process.stdout.write(JSON.stringify({version:1,ok:true,puzzle:${JSON.stringify(puzzle)},cells:${JSON.stringify(cells)}})+'\\n')`)]),
    timeoutMs: 1_000,
    maxOutputBytes: 64 * 1024,
  });
  assert.equal(recovery.status, "ok");
});
