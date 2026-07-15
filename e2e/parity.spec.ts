import { expect, test, type Page } from "@playwright/test";

const puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

async function openCommand(page: Page, name: string) {
  await page.locator(".command-trigger").click();
  await page.getByRole("button", { name }).click();
}

test("analyze or solve creates a trace and timeline playback survives reload recovery", async ({ page }) => {
  await page.goto("/");
  await openCommand(page, "Analyze puzzle");
  await expect(page.locator(".timeline-range")).toBeEnabled();
  await expect(page.locator(".timeline-meta strong")).not.toHaveText("0 / 0");
  await page.getByRole("button", { name: "Next step" }).click();
  const step = await page.locator(".timeline-range").inputValue();
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
  await page.waitForTimeout(350);
  await page.reload();
  await expect(page.getByRole("button", { name: /Row 1, column 3, entered 4/ })).toBeVisible();

  const chooser = page.waitForEvent("filechooser");
  await openCommand(page, "Import backup");
  await (await chooser).setFiles({
    name: "fixture.srr.json",
    mimeType: "application/json",
    buffer: Buffer.from(JSON.stringify({ schemaVersion: 1, puzzles: [], sessions: [], settings: [], exportedAt: "2026-07-15T00:00:00.000Z", appVersion: "0.4.0-beta.1" })),
  });
  const download = page.waitForEvent("download");
  await openCommand(page, "Export backup");
  expect((await download).suggestedFilename()).toBe("sudoku-reasoning-radar.srr.json");
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
    (window as unknown as { srrDesktop: unknown }).srrDesktop = {
      ocr: { selectAndRecognize: async () => {
        (window as unknown as { __ocrCalls: number }).__ocrCalls += 1;
        return { version: 1, status: "ok", puzzle: legacyPuzzle, cells: [...legacyPuzzle].map((digit) => ({ digit: Number(digit), confidence: 99, lowConfidence: false })) };
      } },
      legacy: { import: async () => ({ version: 1, status: "ok", records: [{ puzzle: legacyPuzzle, source: "desktop-fixture" }], errors: [] }) },
      backup: { import: async () => ({ version: 1, status: "cancelled" }), export: async () => ({ version: 1, status: "ok" }) },
      update: { check: async () => ({ version: 1, status: "ok", checkedAt: "2026-07-15T00:00:00.000Z", available: false }) },
    };
  }, puzzle);
  await page.goto("/");
  await expect.poll(() => page.evaluate(() => (window as unknown as { __ocrCalls: number }).__ocrCalls)).toBe(0);
  await openCommand(page, "Import puzzle image");
  await page.getByRole("button", { name: "Select Sudoku image" }).click();
  await expect.poll(() => page.evaluate(() => (window as unknown as { __ocrCalls: number }).__ocrCalls)).toBe(1);
  await expect(page.getByLabel("Image row 1, column 1")).toHaveValue("5");
  await page.getByRole("button", { name: "Apply reviewed puzzle" }).click();
  await expect(page.getByRole("button", { name: /Row 1, column 1, given 5/ })).toBeVisible();
});
