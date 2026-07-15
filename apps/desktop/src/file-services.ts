export interface OpenDialogOptions {
  title: string;
  properties: ["openFile"];
  filters: Array<{ name: string; extensions: string[] }>;
}

export interface SaveDialogOptions {
  title: string;
  defaultPath: string;
  filters: Array<{ name: string; extensions: string[] }>;
}

export interface FileServiceDependencies {
  showOpenDialog(options: OpenDialogOptions): Promise<{ canceled: boolean; filePaths: string[] }>;
  showSaveDialog(options: SaveDialogOptions): Promise<{ canceled: boolean; filePath?: string }>;
  readTextFile(filePath: string): Promise<string>;
  writeTextFile(filePath: string, contents: string): Promise<void>;
  recognize(filePath: string): Promise<unknown>;
}

const IMAGE_DIALOG: OpenDialogOptions = {
  title: "Select Sudoku image",
  properties: ["openFile"],
  filters: [{ name: "Sudoku images", extensions: ["png", "jpg", "jpeg", "webp"] }],
};

const BACKUP_IMPORT_DIALOG: OpenDialogOptions = {
  title: "Import Sudoku Reasoning Radar backup",
  properties: ["openFile"],
  filters: [{ name: "SRR backup", extensions: ["srr.json", "json"] }],
};

const BACKUP_EXPORT_DIALOG: SaveDialogOptions = {
  title: "Export Sudoku Reasoning Radar backup",
  defaultPath: "sudoku-reasoning-radar.srr.json",
  filters: [{ name: "SRR backup", extensions: ["srr.json"] }],
};

export function createFileServices(dependencies: FileServiceDependencies) {
  return {
    async ocrSelectAndRecognize(): Promise<unknown> {
      const selection = await dependencies.showOpenDialog(IMAGE_DIALOG);
      if (selection.canceled || selection.filePaths.length !== 1) return { version: 1, status: "cancelled" };
      return dependencies.recognize(selection.filePaths[0]!);
    },
    async backupImport(): Promise<unknown> {
      const selection = await dependencies.showOpenDialog(BACKUP_IMPORT_DIALOG);
      if (selection.canceled || selection.filePaths.length !== 1) return { version: 1, status: "cancelled" };
      return { version: 1, status: "ok", contents: await dependencies.readTextFile(selection.filePaths[0]!) };
    },
    async backupExport(contents: string): Promise<unknown> {
      const selection = await dependencies.showSaveDialog(BACKUP_EXPORT_DIALOG);
      if (selection.canceled || !selection.filePath) return { version: 1, status: "cancelled" };
      await dependencies.writeTextFile(selection.filePath, contents);
      return { version: 1, status: "ok" };
    },
  };
}
