import { render, screen, waitFor } from "@testing-library/react";
import { expect, it, vi } from "vitest";

import { CoreShell } from "../src/app.tsx";
import {
  createDesktopFileActions,
  createDesktopImageImportAdapter,
  getDesktopBridge,
  type DesktopBridge,
} from "../src/desktop/bridge.ts";
import { fixtureClient } from "./fixtures.ts";

const puzzle = `5${"0".repeat(80)}`;

function bridge(): DesktopBridge {
  return {
    ocr: { selectAndRecognize: vi.fn().mockResolvedValue({
      version: 1, status: "ok", puzzle,
      cells: Array.from(puzzle, (value) => ({ digit: Number(value), confidence: 90, lowConfidence: false })),
    }) },
    legacy: { import: vi.fn().mockResolvedValue({ version: 1, status: "not-found" }) },
    backup: {
      import: vi.fn().mockResolvedValue({ version: 1, status: "ok", contents: "{\"schemaVersion\":1}" }),
      export: vi.fn().mockResolvedValue({ version: 1, status: "ok" }),
    },
    update: { check: vi.fn().mockResolvedValue({
      version: 1, status: "ok", checkedAt: "2026-07-15T00:00:00.000Z", available: true,
      releaseUrl: "https://github.com/example/srr/releases/tag/v1.0.0",
    }) },
  };
}

it("keeps browser adapters active when no desktop bridge exists", () => {
  expect(getDesktopBridge({})).toBeUndefined();
});

it("adapts scoped desktop backup and OCR methods to existing renderer workflows", async () => {
  const desktop = bridge();
  const files = createDesktopFileActions(desktop);
  expect(await files.pickText("ignored-by-desktop")).toBe("{\"schemaVersion\":1}");
  await files.downloadText("ignored.srr.json", "{}\n", "application/json");
  expect(desktop.backup.export).toHaveBeenCalledWith("{}\n");

  const image = createDesktopImageImportAdapter(desktop);
  expect(await image.select()).toEqual([5, ...Array(80).fill(0)]);
});

it("checks updates only with a desktop bridge and safely displays the HTTPS release page", async () => {
  const desktop = bridge();
  render(<CoreShell
    initialPuzzle={"0".repeat(81)}
    desktopBridge={desktop}
    loadClient={async () => fixtureClient()}
    loadStorage={async () => ({ service: undefined as never, migrationReport: { imported: 0, skipped: true, errors: [] } })}
  />);
  const release = await screen.findByRole("textbox", { name: /release page/i });
  expect(release).toHaveValue("https://github.com/example/srr/releases/tag/v1.0.0");
  await waitFor(() => expect(desktop.update.check).toHaveBeenCalledOnce());
});
