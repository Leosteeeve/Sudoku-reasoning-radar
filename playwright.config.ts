import { defineConfig } from "@playwright/test";

const port = Number(process.env.SRR_E2E_PORT ?? 43117);

const viewports = [
  { name: "360x640", width: 360, height: 640 },
  { name: "768x1024", width: 768, height: 1024 },
  { name: "1280x800", width: 1280, height: 800 },
  { name: "2560x1440", width: 2560, height: 1440 },
] as const;

export default defineConfig({
  testDir: "./e2e",
  fullyParallel: false,
  forbidOnly: Boolean(process.env.CI),
  retries: process.env.CI ? 1 : 0,
  reporter: process.env.CI ? "github" : "list",
  snapshotPathTemplate: "{testDir}/__screenshots__/{projectName}/{testFilePath}/{arg}{ext}",
  use: {
    baseURL: `http://127.0.0.1:${port}`,
    browserName: "chromium",
    locale: "en-US",
    colorScheme: "light",
    serviceWorkers: "block",
    trace: "retain-on-failure",
  },
  projects: viewports.map(({ name, width, height }) => ({ name, use: { viewport: { width, height } } })),
  webServer: {
    command: `node apps/renderer/node_modules/vite/bin/vite.js preview apps/renderer --host 127.0.0.1 --port ${port}`,
    url: `http://127.0.0.1:${port}`,
    reuseExistingServer: process.env.SRR_E2E_REUSE_SERVER === "true",
    timeout: 30_000,
  },
});
