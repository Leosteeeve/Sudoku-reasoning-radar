import { CHANNELS, parseRequest, parseResponse, type DesktopChannel } from "./protocol.ts";

export interface IpcMainLike {
  handle(channel: string, handler: (event: unknown, request: unknown) => Promise<unknown>): void;
}

export interface DesktopServices {
  ocrSelectAndRecognize(): Promise<unknown>;
  legacyImport(): Promise<unknown>;
  backupImport(): Promise<unknown>;
  backupExport(contents: string): Promise<unknown>;
  updateCheck(): Promise<unknown>;
}

export function registerIpcHandlers(ipcMain: IpcMainLike, services: DesktopServices): void {
  const handlers: Record<DesktopChannel, (request: Record<string, unknown>) => Promise<unknown>> = {
    [CHANNELS.ocrSelectAndRecognize]: () => services.ocrSelectAndRecognize(),
    [CHANNELS.legacyImport]: () => services.legacyImport(),
    [CHANNELS.backupImport]: () => services.backupImport(),
    [CHANNELS.backupExport]: (request) => services.backupExport(request.contents as string),
    [CHANNELS.updateCheck]: () => services.updateCheck(),
  };
  for (const channel of Object.values(CHANNELS)) {
    ipcMain.handle(channel, async (_event, request) => {
      const parsed = parseRequest(channel, request);
      return parseResponse(channel, await handlers[channel](parsed));
    });
  }
}
