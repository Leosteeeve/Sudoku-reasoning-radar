import {
  migrateLegacyWebPuzzle,
  openStorage,
  type MigrationReport,
  type StorageService,
} from "@srr/storage";

export interface WebStorageRuntime {
  service: StorageService;
  migrationReport: MigrationReport;
}

export async function loadWebStorage(): Promise<WebStorageRuntime> {
  const service = await openStorage();
  const source = {
    get(key: string) { return globalThis.localStorage?.getItem(key); },
  };
  const migrationReport = await migrateLegacyWebPuzzle(source, service);
  return { service, migrationReport };
}
