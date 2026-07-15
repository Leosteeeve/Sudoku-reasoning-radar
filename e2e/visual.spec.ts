import { expect, test, type Page } from "@playwright/test";

async function assertGeometry(page: Page) {
  const viewport = page.viewportSize()!;
  const board = await page.locator(".sudoku-board").boundingBox();
  expect(board, "board clipping: board must have geometry").not.toBeNull();
  expect(board!.x).toBeGreaterThanOrEqual(0);
  expect(board!.x + board!.width).toBeLessThanOrEqual(viewport.width + 1);
  const cells = page.locator(".sudoku-cell");
  for (let index = 0; index < 81; index += 1) {
    const box = await cells.nth(index).boundingBox();
    expect(box, `text overlap/clipping at cell ${index}`).not.toBeNull();
    expect(box!.x).toBeGreaterThanOrEqual(board!.x - 1);
    expect(box!.x + box!.width).toBeLessThanOrEqual(board!.x + board!.width + 1);
    const value = cells.nth(index).locator(".cell-value");
    if (await value.count()) {
      const text = await value.boundingBox();
      expect(text, `cell-value ${index} must have text geometry`).not.toBeNull();
      expect(text!.x).toBeGreaterThanOrEqual(box!.x - 1);
      expect(text!.y).toBeGreaterThanOrEqual(box!.y - 1);
      expect(text!.x + text!.width).toBeLessThanOrEqual(box!.x + box!.width + 1);
      expect(text!.y + text!.height).toBeLessThanOrEqual(box!.y + box!.height + 1);
    }
  }
  const target = cells.nth(40);
  const box = (await target.boundingBox())!;
  const hit = await page.evaluate(({ x, y }) => document.elementFromPoint(x, y)?.closest(".sudoku-cell") !== null, { x: box.x + box.width / 2, y: box.y + box.height / 2 });
  expect(hit, "interaction-coordinate drift at board center").toBe(true);
}

for (const state of ["English", "Chinese", "high contrast", "reduced motion"] as const) {
  test(`${state} deterministic visual state has no board clipping, text overlap, or coordinate drift`, async ({ page }) => {
    if (state === "reduced motion") await page.emulateMedia({ reducedMotion: "reduce" });
    await page.goto("/");
    if (state === "Chinese") await page.locator(".text-button").click();
    if (state === "high contrast") {
      await page.locator(".settings-menu summary").click();
      await page.locator(".settings-menu input[type=checkbox]").check();
    }
    await page.addStyleTag({ content: "*,*::before,*::after{animation:none!important;transition:none!important;caret-color:transparent!important}" });
    await assertGeometry(page);
    await expect(page.locator(".app-shell")).toHaveScreenshot(`${state.toLowerCase().replaceAll(" ", "-")}.png`, { animations: "disabled" });
  });
}
