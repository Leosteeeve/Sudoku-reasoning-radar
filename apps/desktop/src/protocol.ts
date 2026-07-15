export const CHANNELS = Object.freeze({
  ocrSelectAndRecognize: "srr:ocr:select-and-recognize",
  legacyImport: "srr:legacy:import",
  backupImport: "srr:backup:import",
  backupExport: "srr:backup:export",
  updateCheck: "srr:update:check",
} as const);

export type DesktopChannel = typeof CHANNELS[keyof typeof CHANNELS];
export const MAX_BACKUP_BYTES = 16 * 1024 * 1024;

type JsonRecord = Record<string, unknown>;

function record(value: unknown, label: string): JsonRecord {
  if (value === null || typeof value !== "object" || Array.isArray(value)) throw new Error(`${label} must be an object`);
  return value as JsonRecord;
}

function onlyKeys(value: JsonRecord, keys: readonly string[], label: string): void {
  const allowed = new Set(keys);
  const unknown = Object.keys(value).find((key) => !allowed.has(key));
  if (unknown) throw new Error(`${label} contains unknown field: ${unknown}`);
}

function version(value: JsonRecord, label: string): void {
  if (value.version !== 1) throw new Error(`${label} has unsupported version`);
}

function knownChannel(channel: string): asserts channel is DesktopChannel {
  if (!(Object.values(CHANNELS) as string[]).includes(channel)) throw new Error(`Unknown IPC channel: ${channel}`);
}

export function parseRequest(channel: string, input: unknown): JsonRecord {
  knownChannel(channel);
  const value = record(input, "request");
  const keys = channel === CHANNELS.backupExport ? ["version", "contents"] : ["version"];
  onlyKeys(value, keys, "request");
  version(value, "request");
  if (channel === CHANNELS.backupExport) {
    if (typeof value.contents !== "string") throw new Error("backup contents must be a string");
    if (Buffer.byteLength(value.contents, "utf8") > MAX_BACKUP_BYTES) throw new Error("backup contents are oversized");
  }
  return value;
}

function text(value: unknown, label: string): string {
  if (typeof value !== "string" || value.length === 0) throw new Error(`${label} must be a non-empty string`);
  return value;
}

function parseStatus(value: JsonRecord, allowed: readonly string[]): string {
  if (typeof value.status !== "string" || !allowed.includes(value.status)) throw new Error("response has invalid status");
  return value.status;
}

function parseHttps(value: unknown, label: string): string {
  const raw = text(value, label);
  let url: URL;
  try { url = new URL(raw); } catch { throw new Error(`${label} must be an HTTPS URL`); }
  if (url.protocol !== "https:") throw new Error(`${label} must be an HTTPS URL`);
  return url.toString();
}

function isoTimestamp(value: unknown, label: string): string {
  const raw = text(value, label);
  if (Number.isNaN(Date.parse(raw)) || new Date(raw).toISOString() !== raw) throw new Error(`${label} must be an ISO timestamp`);
  return raw;
}

function parseOcrCell(input: unknown, index: number): number {
  const value = record(input, `response.cells[${index}]`);
  onlyKeys(value, ["digit", "confidence", "lowConfidence"], `response.cells[${index}]`);
  if (!Number.isInteger(value.digit) || (value.digit as number) < 0 || (value.digit as number) > 9) throw new Error(`response.cells[${index}].digit is invalid`);
  if (typeof value.confidence !== "number" || !Number.isFinite(value.confidence) || value.confidence < 0 || value.confidence > 100) throw new Error(`response.cells[${index}].confidence is invalid`);
  if (typeof value.lowConfidence !== "boolean") throw new Error(`response.cells[${index}].lowConfidence is invalid`);
  return value.digit as number;
}

function optionalText(value: unknown, label: string): void {
  if (value !== undefined) text(value, label);
}

function parsePuzzleRecord(input: unknown, index: number): void {
  const label = `response.records[${index}]`;
  const value = record(input, label);
  onlyKeys(value, ["puzzle", "name", "difficulty", "solution", "elapsedMs", "seed", "source", "createdAt", "updatedAt"], label);
  if (typeof value.puzzle !== "string" || !/^[0-9]{81}$/.test(value.puzzle)) throw new Error(`${label}.puzzle must have 81 digits`);
  optionalText(value.name, `${label}.name`);
  optionalText(value.difficulty, `${label}.difficulty`);
  optionalText(value.source, `${label}.source`);
  if (value.solution !== undefined && (typeof value.solution !== "string" || !/^[0-9]{81}$/.test(value.solution))) throw new Error(`${label}.solution must have 81 digits`);
  if (value.elapsedMs !== undefined && (typeof value.elapsedMs !== "number" || !Number.isFinite(value.elapsedMs) || value.elapsedMs < 0)) throw new Error(`${label}.elapsedMs is invalid`);
  if (value.seed !== undefined && (!Number.isInteger(value.seed) || (value.seed as number) < 0)) throw new Error(`${label}.seed is invalid`);
  if (value.createdAt !== undefined) isoTimestamp(value.createdAt, `${label}.createdAt`);
  if (value.updatedAt !== undefined) isoTimestamp(value.updatedAt, `${label}.updatedAt`);
}

function parseLegacyError(input: unknown, index: number): void {
  const label = `response.errors[${index}]`;
  const value = record(input, label);
  onlyKeys(value, ["line", "message"], label);
  if (!Number.isInteger(value.line) || (value.line as number) < 1) throw new Error(`${label}.line is invalid`);
  text(value.message, `${label}.message`);
}

export function parseResponse(channel: string, input: unknown): JsonRecord {
  knownChannel(channel);
  const value = record(input, "response");
  version(value, "response");
  if (channel === CHANNELS.ocrSelectAndRecognize) {
    const status = parseStatus(value, ["ok", "cancelled", "error"]);
    if (status === "cancelled") onlyKeys(value, ["version", "status"], "response");
    else if (status === "error") {
      onlyKeys(value, ["version", "status", "code"], "response");
      text(value.code, "response.code");
    } else {
      onlyKeys(value, ["version", "status", "puzzle", "cells"], "response");
      if (typeof value.puzzle !== "string" || !/^[0-9]{81}$/.test(value.puzzle)) throw new Error("response puzzle must have 81 digits");
      if (!Array.isArray(value.cells) || value.cells.length !== 81) throw new Error("response cells must have 81 entries");
      const puzzle = value.cells.map(parseOcrCell).join("");
      if (puzzle !== value.puzzle) throw new Error("response cells do not match response puzzle");
    }
  } else if (channel === CHANNELS.legacyImport) {
    const status = parseStatus(value, ["ok", "not-found"]);
    if (status === "not-found") onlyKeys(value, ["version", "status"], "response");
    else {
      onlyKeys(value, ["version", "status", "records", "errors"], "response");
      if (!Array.isArray(value.records) || !Array.isArray(value.errors)) throw new Error("legacy response arrays are required");
      value.records.forEach(parsePuzzleRecord);
      value.errors.forEach(parseLegacyError);
    }
  } else if (channel === CHANNELS.backupImport) {
    const status = parseStatus(value, ["ok", "cancelled"]);
    if (status === "cancelled") onlyKeys(value, ["version", "status"], "response");
    else {
      onlyKeys(value, ["version", "status", "contents"], "response");
      if (typeof value.contents !== "string" || Buffer.byteLength(value.contents, "utf8") > MAX_BACKUP_BYTES) throw new Error("invalid backup contents");
    }
  } else if (channel === CHANNELS.backupExport) {
    parseStatus(value, ["ok", "cancelled"]);
    onlyKeys(value, ["version", "status"], "response");
  } else {
    const status = parseStatus(value, ["ok", "error"]);
    if (status === "error") {
      onlyKeys(value, ["version", "status", "code"], "response");
      text(value.code, "response.code");
    } else {
      onlyKeys(value, value.available === true
        ? ["version", "status", "checkedAt", "available", "releaseUrl"]
        : ["version", "status", "checkedAt", "available"], "response");
      isoTimestamp(value.checkedAt, "response.checkedAt");
      if (typeof value.available !== "boolean") throw new Error("response.available must be boolean");
      if (value.available) parseHttps(value.releaseUrl, "response.releaseUrl");
    }
  }
  return value;
}
