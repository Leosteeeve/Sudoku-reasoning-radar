import {
  normalizePuzzle,
  type PuzzleRecord,
  type SessionRecord,
  type SettingsRecord,
} from "./index.ts";
import { createBackup, parseBackup, serializeBackup } from "./backup.ts";
import { mergePuzzleRecords } from "./legacy.ts";

export interface OpenStorageOptions {
  indexedDB?: IDBFactory;
  name?: string;
}

function requestResult<T>(request: IDBRequest<T>): Promise<T> {
  return new Promise((resolve, reject) => {
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error ?? new Error("IndexedDB request failed"));
  });
}

function transactionDone(transaction: IDBTransaction): Promise<void> {
  return new Promise((resolve, reject) => {
    transaction.oncomplete = () => resolve();
    transaction.onabort = () => reject(transaction.error ?? new Error("IndexedDB transaction aborted"));
    transaction.onerror = () => reject(transaction.error ?? new Error("IndexedDB transaction failed"));
  });
}

function isPresent(value: unknown): boolean {
  return value !== undefined && value !== null && (typeof value !== "string" || value.trim() !== "");
}

function earlier(left?: string, right?: string): string | undefined {
  if (!left) return right;
  if (!right) return left;
  return Date.parse(left) <= Date.parse(right) ? left : right;
}

function later(left?: string, right?: string): string | undefined {
  if (!left) return right;
  if (!right) return left;
  return Date.parse(left) >= Date.parse(right) ? left : right;
}

export function openVersionedDatabase(
  factory: IDBFactory,
  name: string,
  version: number,
  upgrade: (database: IDBDatabase, transaction: IDBTransaction, oldVersion: number) => void,
): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    let request: IDBOpenDBRequest;
    let settled = false;
    let upgradeError: unknown;
    try {
      request = factory.open(name, version);
    } catch (error) {
      reject(error);
      return;
    }
    request.onupgradeneeded = (event) => {
      try {
        upgrade(request.result, request.transaction!, event.oldVersion);
      } catch (error) {
        upgradeError = error;
        request.transaction?.abort();
      }
    };
    request.onsuccess = () => {
      if (settled) request.result.close();
      else {
        settled = true;
        resolve(request.result);
      }
    };
    request.onerror = () => {
      if (settled) return;
      settled = true;
      reject(upgradeError ?? request.error ?? new Error("Unable to open IndexedDB"));
    };
    request.onblocked = () => {
      if (settled) return;
      settled = true;
      reject(new Error("IndexedDB open was blocked"));
    };
  });
}

export class IndexedDbStorage {
  private readonly database: IDBDatabase;

  constructor(database: IDBDatabase) { this.database = database; }

  get version(): number { return this.database.version; }

  get storeNames(): string[] { return Array.from(this.database.objectStoreNames); }

  async upsertPuzzle(record: PuzzleRecord): Promise<void> {
    const puzzle = normalizePuzzle(record.puzzle);
    const transaction = this.database.transaction("puzzles", "readwrite");
    const store = transaction.objectStore("puzzles");
    const existing = await requestResult<PuzzleRecord | undefined>(store.get(puzzle));
    const incoming = { ...record, puzzle };
    const merged: PuzzleRecord = { puzzle };
    for (const key of ["name", "difficulty", "solution", "elapsedMs", "seed", "source"] as const) {
      const value = isPresent(existing?.[key]) ? existing?.[key] : incoming[key];
      if (isPresent(value)) Object.assign(merged, { [key]: value });
    }
    const createdAt = earlier(existing?.createdAt, incoming.createdAt);
    const updatedAt = later(existing?.updatedAt, incoming.updatedAt);
    if (createdAt) merged.createdAt = createdAt;
    if (updatedAt) merged.updatedAt = updatedAt;
    store.put(merged);
    await transactionDone(transaction);
  }

  async getPuzzle(puzzle: string): Promise<PuzzleRecord | undefined> {
    const transaction = this.database.transaction("puzzles", "readonly");
    return await requestResult(transaction.objectStore("puzzles").get(normalizePuzzle(puzzle)));
  }

  async listPuzzles(): Promise<PuzzleRecord[]> {
    const transaction = this.database.transaction("puzzles", "readonly");
    const records = await requestResult<PuzzleRecord[]>(transaction.objectStore("puzzles").getAll());
    return records.sort((left, right) => left.puzzle.localeCompare(right.puzzle));
  }

  async listSessions(): Promise<SessionRecord[]> {
    const transaction = this.database.transaction("sessions", "readonly");
    const records = await requestResult<SessionRecord[]>(transaction.objectStore("sessions").getAll());
    return records.sort((left, right) => left.id.localeCompare(right.id));
  }

  async listSettings(): Promise<SettingsRecord[]> {
    const transaction = this.database.transaction("settings", "readonly");
    const records = await requestResult<SettingsRecord[]>(transaction.objectStore("settings").getAll());
    return records.sort((left, right) => left.id.localeCompare(right.id));
  }

  async saveCurrentSession(session: SessionRecord): Promise<void> {
    const transaction = this.database.transaction("sessions", "readwrite");
    transaction.objectStore("sessions").put({ ...session, puzzle: normalizePuzzle(session.puzzle) });
    await transactionDone(transaction);
  }

  async loadCurrentSession(): Promise<SessionRecord | undefined> {
    const transaction = this.database.transaction("sessions", "readonly");
    return await requestResult(transaction.objectStore("sessions").get("current"));
  }

  async setSettings(settings: SettingsRecord): Promise<void> {
    const transaction = this.database.transaction("settings", "readwrite");
    transaction.objectStore("settings").put(settings);
    await transactionDone(transaction);
  }

  async getSettings(id: string): Promise<SettingsRecord | undefined> {
    const transaction = this.database.transaction("settings", "readonly");
    return await requestResult(transaction.objectStore("settings").get(id));
  }

  async hasMigration(marker: string): Promise<boolean> {
    return (await this.getSettings(`migration:${marker}`)) !== undefined;
  }

  async markMigration(marker: string): Promise<void> {
    await this.setSettings({
      id: `migration:${marker}`,
      migrationSource: marker,
      migrationVersion: 1,
      migratedAt: new Date().toISOString(),
    });
  }

  async importBackup(value: unknown): Promise<void> {
    const backup = parseBackup(value);
    const transaction = this.database.transaction(["puzzles", "sessions", "settings"], "readwrite");
    const completion = transactionDone(transaction);
    try {
      const puzzles = transaction.objectStore("puzzles");
      for (const candidate of backup.puzzles) {
        const existing = await requestResult<PuzzleRecord | undefined>(puzzles.get(candidate.puzzle));
        puzzles.put(existing ? mergePuzzleRecords(existing, candidate) : candidate);
      }
      const sessions = transaction.objectStore("sessions");
      for (const session of backup.sessions) sessions.put(session);
      const settings = transaction.objectStore("settings");
      for (const setting of backup.settings) settings.put(setting);
      await completion;
    } catch (error) {
      try { transaction.abort(); } catch { /* already completed or aborted */ }
      throw error;
    }
  }

  async importPuzzles(records: PuzzleRecord[]): Promise<void> {
    const validated = createBackup({ puzzles: records, sessions: [], settings: [] }, {
      appVersion: "storage-import",
      exportedAt: "1970-01-01T00:00:00.000Z",
    }).puzzles;
    const transaction = this.database.transaction("puzzles", "readwrite");
    const completion = transactionDone(transaction);
    const puzzles = transaction.objectStore("puzzles");
    try {
      for (const candidate of validated) {
        const existing = await requestResult<PuzzleRecord | undefined>(puzzles.get(candidate.puzzle));
        puzzles.put(existing ? mergePuzzleRecords(existing, candidate) : candidate);
      }
      await completion;
    } catch (error) {
      try { transaction.abort(); } catch { /* already completed or aborted */ }
      throw error;
    }
  }

  async exportBackup(appVersion: string, exportedAt?: string): Promise<string> {
    return serializeBackup(createBackup({
      puzzles: await this.listPuzzles(),
      sessions: await this.listSessions(),
      settings: await this.listSettings(),
    }, { appVersion, exportedAt }));
  }

  async clearV1Stores(): Promise<void> {
    const transaction = this.database.transaction(["puzzles", "sessions", "settings"], "readwrite");
    transaction.objectStore("puzzles").clear();
    transaction.objectStore("sessions").clear();
    transaction.objectStore("settings").clear();
    await transactionDone(transaction);
  }

  close(): void { this.database.close(); }
}

export function openStorage(options: OpenStorageOptions = {}): Promise<IndexedDbStorage> {
  const factory = options.indexedDB ?? globalThis.indexedDB;
  if (!factory) return Promise.reject(new Error("IndexedDB is unavailable"));
  return openVersionedDatabase(
    factory,
    options.name ?? "sudoku-reasoning-radar",
    1,
    (database) => {
      if (!database.objectStoreNames.contains("puzzles")) database.createObjectStore("puzzles", { keyPath: "puzzle" });
      if (!database.objectStoreNames.contains("sessions")) database.createObjectStore("sessions", { keyPath: "id" });
      if (!database.objectStoreNames.contains("settings")) database.createObjectStore("settings", { keyPath: "id" });
    },
  ).then((database) => new IndexedDbStorage(database));
}
