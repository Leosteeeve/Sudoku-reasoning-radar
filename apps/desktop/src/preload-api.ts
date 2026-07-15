import { CHANNELS, parseResponse, type DesktopChannel } from "./protocol.ts";

export type Invoke = (channel: DesktopChannel, request: Record<string, unknown>) => Promise<unknown>;

export function createPreloadApi(invoke: Invoke) {
  const call = async (channel: DesktopChannel, request: Record<string, unknown>) => parseResponse(channel, await invoke(channel, request));
  return Object.freeze({
    ocr: Object.freeze({ selectAndRecognize: () => call(CHANNELS.ocrSelectAndRecognize, { version: 1 }) }),
    legacy: Object.freeze({ import: () => call(CHANNELS.legacyImport, { version: 1 }) }),
    backup: Object.freeze({
      import: () => call(CHANNELS.backupImport, { version: 1 }),
      export: (contents: string) => call(CHANNELS.backupExport, { version: 1, contents }),
    }),
    update: Object.freeze({ check: () => call(CHANNELS.updateCheck, { version: 1 }) }),
  });
}

export type DesktopBridge = ReturnType<typeof createPreloadApi>;
