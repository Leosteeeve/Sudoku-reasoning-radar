import { expect, test, type Page } from "@playwright/test";
import { readFile } from "node:fs/promises";

const puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

async function openCommand(page: Page, name: string) {
  await page.locator(".command-trigger").click();
  await page.getByRole("button", { name, exact: true }).click();
}

async function persistedDigit(page: Page, index: number): Promise<number | null> {
  return page.evaluate(async (cell) => {
    const database = await new Promise<IDBDatabase>((resolve, reject) => {
      const request = indexedDB.open("sudoku-reasoning-radar");
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
    try {
      return await new Promise<number | null>((resolve, reject) => {
        const request = database.transaction("sessions", "readonly").objectStore("sessions").get("current");
        request.onsuccess = () => resolve(request.result?.values?.[cell] ?? null);
        request.onerror = () => reject(request.error);
      });
    } finally {
      database.close();
    }
  }, index);
}

async function persistedCurrentStep(page: Page): Promise<number | null> {
  return page.evaluate(async () => {
    const database = await new Promise<IDBDatabase>((resolve, reject) => {
      const request = indexedDB.open("sudoku-reasoning-radar");
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
    try {
      return await new Promise<number | null>((resolve, reject) => {
        const request = database.transaction("sessions", "readonly").objectStore("sessions").get("current");
        request.onsuccess = () => resolve(request.result?.currentStep ?? null);
        request.onerror = () => reject(request.error);
      });
    } finally {
      database.close();
    }
  });
}

test("analyze or solve creates a trace and timeline playback survives reload recovery", async ({ page }) => {
  await page.goto("/");
  await openCommand(page, "Analyze puzzle");
  await expect(page.locator(".timeline-range")).toBeEnabled();
  await expect(page.locator(".timeline-meta strong")).not.toHaveText("0 / 0");
  await page.getByRole("button", { name: "Next step" }).click();
  const step = await page.locator(".timeline-range").inputValue();
  await expect.poll(() => persistedCurrentStep(page)).toBe(Number(step));
  await page.reload();
  await expect(page.locator(".timeline-range")).toHaveValue(step);
});

test("text import, backup import/export, and persistence after reload use deterministic fixtures", async ({ page }) => {
  await page.goto("/");
  await openCommand(page, "Import puzzle");
  await page.getByRole("textbox").fill(puzzle);
  await page.getByRole("button", { name: "Apply puzzle" }).click();

  const empty = page.getByRole("gridcell").nth(2).getByRole("button");
  await empty.click();
  await empty.press("4");
  await expect.poll(() => persistedDigit(page, 2)).toBe(4);
  await page.reload();
  await expect(page.getByRole("button", { name: /Row 1, column 3, entered 4/ })).toBeVisible();

  const chooser = page.waitForEvent("filechooser");
  await openCommand(page, "Import backup");
  await (await chooser).setFiles({
    name: "fixture.srr.json",
    mimeType: "application/json",
    buffer: Buffer.from(JSON.stringify({
      schemaVersion: 1,
      puzzles: [{ puzzle, name: "Backup fixture", source: "e2e-backup" }],
      sessions: [],
      settings: [],
      exportedAt: "2026-07-15T00:00:00.000Z",
      appVersion: "0.4.0-beta.1",
    })),
  });
  const download = page.waitForEvent("download");
  await openCommand(page, "Export backup");
  const exported = await download;
  expect(exported.suggestedFilename()).toBe("sudoku-reasoning-radar.srr.json");
  const downloadPath = await exported.path();
  expect(downloadPath).not.toBeNull();
  const backup = JSON.parse(await readFile(downloadPath!, "utf8"));
  expect(backup.puzzles).toContainEqual(expect.objectContaining({
    puzzle,
    name: "Backup fixture",
    source: "web-text-import",
  }));
});

test("Web legacy migration preserves the source and restart recovery remains idempotent", async ({ page }) => {
  await page.addInitScript((value) => localStorage.setItem("sudoku_reasoning_radar_last", value), puzzle);
  await page.goto("/");
  await expect(page.getByRole("status")).toContainText(/Legacy puzzle imported/);
  await expect.poll(() => page.evaluate(() => localStorage.getItem("sudoku_reasoning_radar_last"))).toBe(puzzle);
  await page.reload();
  await expect.poll(() => page.evaluate(() => localStorage.getItem("sudoku_reasoning_radar_last"))).toBe(puzzle);
});

test("desktop bridge migration and controlled OCR review are lazy", async ({ page }) => {
  await page.addInitScript((legacyPuzzle) => {
    (window as unknown as { __ocrCalls: number }).__ocrCalls = 0;
    (window as unknown as { __legacyCalls: number }).__legacyCalls = Number(sessionStorage.getItem("srr-e2e-legacy-calls") ?? 0);
    (window as unknown as { srrDesktop: unknown }).srrDesktop = {
      ocr: { selectAndRecognize: async () => {
        (window as unknown as { __ocrCalls: number }).__ocrCalls += 1;
        return { version: 1, status: "ok", puzzle: legacyPuzzle, cells: [...legacyPuzzle].map((digit) => ({ digit: Number(digit), confidence: 99, lowConfidence: false })) };
      } },
      legacy: { import: async () => {
        const scope = window as unknown as { __legacyCalls: number };
        scope.__legacyCalls += 1;
        sessionStorage.setItem("srr-e2e-legacy-calls", String(scope.__legacyCalls));
        return { version: 1, status: "ok", records: [{ puzzle: legacyPuzzle, name: "Desktop fixture", source: "desktop-fixture" }], errors: [] };
      } },
      backup: { import: async () => ({ version: 1, status: "cancelled" }), export: async () => ({ version: 1, status: "ok" }) },
      update: { check: async () => ({ version: 1, status: "ok", checkedAt: "2026-07-15T00:00:00.000Z", available: false }) },
    };
  }, puzzle);
  await page.goto("/");
  await expect.poll(() => page.evaluate(() => (window as unknown as { __legacyCalls: number }).__legacyCalls)).toBe(1);
  await openCommand(page, "Open puzzle library");
  await expect(page.getByRole("dialog", { name: "Puzzle library" }).getByRole("button", { name: /Desktop fixture/ })).toBeVisible();
  await page.getByRole("dialog", { name: "Puzzle library" }).getByRole("button", { name: "Close" }).click();
  await expect.poll(() => page.evaluate(() => (window as unknown as { __ocrCalls: number }).__ocrCalls)).toBe(0);
  await openCommand(page, "Import puzzle image");
  await page.getByRole("button", { name: "Select Sudoku image" }).click();
  await expect.poll(() => page.evaluate(() => (window as unknown as { __ocrCalls: number }).__ocrCalls)).toBe(1);
  await expect(page.getByLabel("Image row 1, column 1")).toHaveValue("5");
  await page.getByRole("button", { name: "Apply reviewed puzzle" }).click();
  await expect(page.getByRole("button", { name: /Row 1, column 1, given 5/ })).toBeVisible();
});
