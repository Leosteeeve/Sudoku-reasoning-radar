import react from "@vitejs/plugin-react";
import { defineConfig, loadEnv } from "vite";

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, ".", "SRR_");

  return {
    base: env.SRR_BASE_PATH ?? "/",
    plugins: [react()],
    build: { outDir: "dist", emptyOutDir: true },
    test: {
      environment: "jsdom",
      setupFiles: "./test/setup.ts",
      css: true,
    },
  };
});
