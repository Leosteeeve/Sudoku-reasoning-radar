import { expect, test } from "@playwright/test";
import { assertPlaybackBudget } from "../scripts/playback-budget.mjs";

test("2000-step timeline playback has no main-thread long task over 50 ms", async ({ page }) => {
  await page.goto("/");
  await page.locator(".command-trigger").click();
  await page.getByRole("button", { name: "Analyze puzzle" }).click();
  await expect(page.locator(".timeline-range")).toBeEnabled();

  const result = await page.evaluate(async () => {
    const durations: number[] = [];
    let dispatchCount = 0;
    const observer = new PerformanceObserver((list) => durations.push(...list.getEntries().map((entry) => entry.duration)));
    observer.observe({ type: "longtask", buffered: true });
    const range = document.querySelector<HTMLInputElement>(".timeline-range")!;
    for (let batch = 0; batch < 200; batch += 1) {
      for (let offset = 0; offset < 10; offset += 1) {
        range.value = String((batch * 10 + offset) % (Number(range.max) + 1));
        range.dispatchEvent(new Event("input", { bubbles: true }));
        dispatchCount += 1;
      }
      await new Promise(requestAnimationFrame);
    }
    await new Promise(requestAnimationFrame);
    observer.disconnect();
    return { taskDurations: durations, dispatchCount };
  });

  expect(result.dispatchCount).toBe(2000);
  expect(assertPlaybackBudget({ steps: result.dispatchCount, taskDurations: result.taskDurations }).longestTaskMs).toBeLessThanOrEqual(50);
});
