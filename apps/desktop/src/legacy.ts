import { readFile } from "node:fs/promises";
import path from "node:path";

import { parseLegacyPipeText, type LegacyParseResult } from "@srr/storage";

export async function discoverLegacyPuzzles(roots: readonly string[]): Promise<LegacyParseResult> {
  for (const root of roots) {
    try {
      const contents = await readFile(path.join(root, "data", "puzzles.txt"), "utf8");
      return parseLegacyPipeText(contents);
    } catch (error) {
      if ((error as NodeJS.ErrnoException).code !== "ENOENT") throw error;
    }
  }
  return { records: [], errors: [] };
}
