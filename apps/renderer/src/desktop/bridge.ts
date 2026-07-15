import type { PuzzleRecord } from "@srr/storage";

type Cancelled = { version: 1; status: "cancelled" };

export interface DesktopOcrCell {
  digit: number;
  confidence: number;
  lowConfidence: boolean;
}

export interface DesktopBridge {
  ocr: {
    selectAndRecognize(): Promise<Cancelled | {
      version: 1;
      status: "ok";
      puzzle: string;
      cells: DesktopOcrCell[];
    } | { version: 1; status: "error"; code: string }>;
  };
  legacy: {
    import(): Promise<
      | { version: 1; status: "not-found" }
      | { version: 1; status: "ok"; records: PuzzleRecord[]; errors: Array<{ line: number; message: string }> }
    >;
  };
  backup: {
    import(): Promise<Cancelled | { version: 1; status: "ok"; contents: string }>;
    export(contents: string): Promise<Cancelled | { version: 1; status: "ok" }>;
  };
  update: {
    check(): Promise<
      | { version: 1; status: "error"; code: string }
      | { version: 1; status: "ok"; checkedAt: string; available: false }
      | { version: 1; status: "ok"; checkedAt: string; available: true; releaseUrl: string }
    >;
  };
}

export function getDesktopBridge(scope: unknown = globalThis): DesktopBridge | undefined {
  if (!scope || typeof scope !== "object") return undefined;
  const bridge = (scope as { srrDesktop?: unknown }).srrDesktop;
  return bridge && typeof bridge === "object" ? bridge as DesktopBridge : undefined;
}

export function createDesktopFileActions(bridge: DesktopBridge) {
  return {
    async pickText(_accept: string): Promise<string | null> {
      const result = await bridge.backup.import();
      return result.status === "ok" ? result.contents : null;
    },
    async downloadText(_filename: string, text: string, _type: string): Promise<void> {
      await bridge.backup.export(text);
    },
  };
}

export function createDesktopImageImportAdapter(bridge: DesktopBridge) {
  return {
    async select(): Promise<number[] | null> {
      const result = await bridge.ocr.selectAndRecognize();
      if (result.status === "cancelled") return null;
      if (result.status === "error") throw new Error(result.code);
      return result.cells.map((cell) => cell.digit);
    },
  };
}
