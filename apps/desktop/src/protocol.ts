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
    }
  } else if (channel === CHANNELS.legacyImport) {
    const status = parseStatus(value, ["ok", "not-found"]);
    if (status === "not-found") onlyKeys(value, ["version", "status"], "response");
    else {
      onlyKeys(value, ["version", "status", "records", "errors"], "response");
      if (!Array.isArray(value.records) || !Array.isArray(value.errors)) throw new Error("legacy response arrays are required");
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
      onlyKeys(value, ["version", "status", "checkedAt", "available", "releaseUrl"], "response");
      text(value.checkedAt, "response.checkedAt");
      if (typeof value.available !== "boolean") throw new Error("response.available must be boolean");
      if (value.releaseUrl !== undefined) parseHttps(value.releaseUrl, "response.releaseUrl");
    }
  }
  return value;
}
