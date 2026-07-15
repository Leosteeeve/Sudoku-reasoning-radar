import {
  migrateLegacyDesktopPuzzles,
  migrateLegacyWebPuzzle,
  openStorage,
  type MigrationReport,
  type StorageService,
} from "@srr/storage";
import type { DesktopBridge } from "../desktop/bridge";

export interface WebStorageRuntime {
  service: StorageService;
  migrationReport: MigrationReport;
}

export async function loadWebStorage(desktopBridge?: DesktopBridge): Promise<WebStorageRuntime> {
  const service = await openStorage();
  const source = {
    get(key: string) { return globalThis.localStorage?.getItem(key); },
  };
  const webReport = await migrateLegacyWebPuzzle(source, service);
  const desktopReport = desktopBridge
    ? await migrateLegacyDesktopPuzzles(desktopBridge.legacy, service)
    : { imported: 0, skipped: true, errors: [] };
  const migrationReport = {
    imported: webReport.imported + desktopReport.imported,
    skipped: webReport.skipped && desktopReport.skipped,
    errors: [...webReport.errors, ...desktopReport.errors],
  };
  return { service, migrationReport };
}
