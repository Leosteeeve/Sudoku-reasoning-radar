export interface DesktopWindowOptions {
  width: number;
  height: number;
  show: boolean;
  webPreferences: {
    preload: string;
    sandbox: true;
    contextIsolation: true;
    nodeIntegration: false;
  };
}

export function createWindowOptions(preload: string): DesktopWindowOptions {
  return {
    width: 1280,
    height: 820,
    show: false,
    webPreferences: {
      preload,
      sandbox: true,
      contextIsolation: true,
      nodeIntegration: false,
    },
  };
}
