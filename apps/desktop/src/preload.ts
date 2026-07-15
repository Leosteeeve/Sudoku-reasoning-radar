import { contextBridge, ipcRenderer } from "electron";

import { createPreloadApi } from "./preload-api.ts";

contextBridge.exposeInMainWorld("srrDesktop", createPreloadApi((channel, request) => ipcRenderer.invoke(channel, request)));
