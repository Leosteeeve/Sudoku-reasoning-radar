import type { ChildProcessWithoutNullStreams } from "node:child_process";

export interface OcrCell {
  digit: number;
  confidence: number;
  lowConfidence: boolean;
}

export type OcrSessionResult =
  | { version: 1; status: "ok"; puzzle: string; cells: OcrCell[] }
  | { version: 1; status: "error"; code: "timeout" | "crash" | "malformed-response" | "oversized-response" | "schema-mismatch" | "recognizer-error" };

export interface OcrSessionOptions {
  spawnHelper(): ChildProcessWithoutNullStreams;
  timeoutMs: number;
  cleanupTimeoutMs?: number;
  maxOutputBytes: number;
}

function exactKeys(value: Record<string, unknown>, keys: readonly string[]): boolean {
  return Object.keys(value).length === keys.length && Object.keys(value).every((key) => keys.includes(key));
}

function parseSidecarLine(line: string): OcrSessionResult {
  let parsed: unknown;
  try { parsed = JSON.parse(line); } catch { return { version: 1, status: "error", code: "malformed-response" }; }
  if (parsed === null || typeof parsed !== "object" || Array.isArray(parsed)) return { version: 1, status: "error", code: "schema-mismatch" };
  const value = parsed as Record<string, unknown>;
  if (value.version !== 1 || typeof value.ok !== "boolean") return { version: 1, status: "error", code: "schema-mismatch" };
  if (!value.ok) {
    if (!exactKeys(value, ["version", "ok", "error"]) || typeof value.error !== "string") return { version: 1, status: "error", code: "schema-mismatch" };
    return { version: 1, status: "error", code: "recognizer-error" };
  }
  if (!exactKeys(value, ["version", "ok", "puzzle", "cells"]) || typeof value.puzzle !== "string" || !/^[0-9]{81}$/.test(value.puzzle) || !Array.isArray(value.cells) || value.cells.length !== 81) {
    return { version: 1, status: "error", code: "schema-mismatch" };
  }
  const cells: OcrCell[] = [];
  for (const candidate of value.cells) {
    if (candidate === null || typeof candidate !== "object" || Array.isArray(candidate)) return { version: 1, status: "error", code: "schema-mismatch" };
    const cell = candidate as Record<string, unknown>;
    if (!exactKeys(cell, ["digit", "confidence", "lowConfidence"])
      || !Number.isInteger(cell.digit) || (cell.digit as number) < 0 || (cell.digit as number) > 9
      || typeof cell.confidence !== "number" || !Number.isFinite(cell.confidence) || cell.confidence < 0 || cell.confidence > 100
      || typeof cell.lowConfidence !== "boolean") {
      return { version: 1, status: "error", code: "schema-mismatch" };
    }
    cells.push({ digit: cell.digit as number, confidence: cell.confidence, lowConfidence: cell.lowConfidence });
  }
  if (cells.map((cell) => cell.digit).join("") !== value.puzzle) return { version: 1, status: "error", code: "schema-mismatch" };
  return { version: 1, status: "ok", puzzle: value.puzzle, cells };
}

export function recognizeWithSidecar(imagePath: string, options: OcrSessionOptions): Promise<OcrSessionResult> {
  return new Promise((resolve) => {
    let child: ChildProcessWithoutNullStreams;
    try { child = options.spawnHelper(); } catch { resolve({ version: 1, status: "error", code: "crash" }); return; }
    let resultPending: OcrSessionResult | undefined;
    let exited = false;
    let cleanupTimer: ReturnType<typeof setTimeout> | undefined;
    let output = "";
    let outputBytes = 0;

    const resolveResult = () => {
      if (!resultPending) return;
      if (cleanupTimer) clearTimeout(cleanupTimer);
      child.off("exit", onExit);
      resolve(resultPending);
    };
    const finish = (result: OcrSessionResult) => {
      if (resultPending) return;
      resultPending = result;
      clearTimeout(timer);
      child.stdout.off("data", onData);
      child.off("error", onError);
      if (exited || child.exitCode !== null || child.signalCode !== null) { resolveResult(); return; }
      try { child.kill(); } catch { /* bounded cleanup timer still resolves the session */ }
      cleanupTimer = setTimeout(resolveResult, options.cleanupTimeoutMs ?? 1_000);
    };
    const parseOutput = () => finish(parseSidecarLine(output.trim()));
    const onData = (chunk: Buffer | string) => {
      outputBytes += Buffer.byteLength(chunk);
      if (outputBytes > options.maxOutputBytes) { finish({ version: 1, status: "error", code: "oversized-response" }); return; }
      output += chunk.toString();
      if (output.includes("\n")) parseOutput();
    };
    const onError = () => finish({ version: 1, status: "error", code: "crash" });
    const onExit = (code: number | null) => {
      exited = true;
      if (resultPending) { resolveResult(); return; }
      if (output.trim()) parseOutput();
      else finish({ version: 1, status: "error", code: code === 0 ? "malformed-response" : "crash" });
    };
    const timer = setTimeout(() => finish({ version: 1, status: "error", code: "timeout" }), options.timeoutMs);
    child.stdout.setEncoding("utf8");
    child.stdout.on("data", onData);
    child.on("error", onError);
    child.on("exit", onExit);
    child.stdin.end(`${JSON.stringify({ version: 1, operation: "recognize", imagePath })}\n`);
  });
}
