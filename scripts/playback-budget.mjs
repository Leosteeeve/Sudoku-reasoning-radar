export const PLAYBACK_STEPS = 2000;
export const MAX_LONG_TASK_MS = 50;

export function assertPlaybackBudget({ steps, taskDurations }) {
  if (steps !== PLAYBACK_STEPS) throw new Error(`Playback benchmark must execute exactly ${PLAYBACK_STEPS} steps`);
  const longestTaskMs = taskDurations.length ? Math.max(...taskDurations) : 0;
  if (longestTaskMs > MAX_LONG_TASK_MS) throw new Error(`Playback exceeded the ${MAX_LONG_TASK_MS} ms long-task budget: ${longestTaskMs} ms`);
  return { steps, longestTaskMs };
}
