import assert from "node:assert/strict";
import test from "node:test";

import { assertPlaybackBudget } from "./playback-budget.mjs";

test("the release playback gate requires 2000 steps and rejects any task over 50 ms", () => {
  assert.deepEqual(assertPlaybackBudget({ steps: 2000, taskDurations: [4, 12, 50] }), { steps: 2000, longestTaskMs: 50 });
  assert.throws(() => assertPlaybackBudget({ steps: 1999, taskDurations: [] }), /2000 steps/i);
  assert.throws(() => assertPlaybackBudget({ steps: 2000, taskDurations: [50.01] }), /50 ms/i);
});
