import { expect, test } from "@playwright/test";
import { assertPlaybackBudget } from "../scripts/playback-budget.mjs";

test("2000-step timeline playback has no main-thread long task over 50 ms", async ({ page }) => {
  await page.goto("/");
  await page.locator(".command-trigger").click();
  await page.getByRole("button", { name: "Analyze puzzle" }).click();
  await expect(page.locator(".timeline-range")).toBeEnabled();

  const taskDurations = await page.evaluate(async () => {
    const durations: number[] = [];
    const observer = new PerformanceObserver((list) => durations.push(...list.getEntries().map((entry) => entry.duration)));
    observer.observe({ type: "longtask", buffered: true });
    const range = document.querySelector<HTMLInputElement>(".timeline-range")!;
    for (let batch = 0; batch < 200; batch += 1) {
      for (let offset = 0; offset < 10; offset += 1) {
        range.value = String((batch * 10 + offset) % (Number(range.max) + 1));
        range.dispatchEvent(new Event("input", { bubbles: true }));
      }
      await new Promise(requestAnimationFrame);
    }
    await new Promise(requestAnimationFrame);
    observer.disconnect();
    return durations;
  });

  expect(assertPlaybackBudget({ steps: 2000, taskDurations }).longestTaskMs).toBeLessThanOrEqual(50);
});
