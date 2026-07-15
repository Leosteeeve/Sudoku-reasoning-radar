import assert from "node:assert/strict";
import test from "node:test";
import { parseLegacyPipeText } from "../src/index.ts";

const puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
const solution = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";

test("parses legacy pipe rows while ignoring comments and blank lines", () => {
  const parsed = parseLegacyPipeText(`
# preserved source comment
Starter|easy|${puzzle}|${solution}|2024-01-02T03:04:05.000Z|42
// another comment
  `);

  assert.deepEqual(parsed.errors, []);
  assert.deepEqual(parsed.records, [{
    puzzle,
    name: "Starter",
    difficulty: "easy",
    solution,
    createdAt: "2024-01-02T03:04:05.000Z",
    seed: 42,
    source: "legacy-windows-pipe",
  }]);
});

test("reports malformed columns, puzzle, solution, date, and seed without losing valid rows", () => {
  const parsed = parseLegacyPipeText([
    "too|few|columns",
    `Bad puzzle|easy|123|${solution}|2024-01-02T03:04:05.000Z|1`,
    `Bad solution|easy|${puzzle}|123|2024-01-02T03:04:05.000Z|1`,
    `Bad date|easy|${puzzle}|${solution}|not-a-date|1`,
    `Bad seed|easy|${puzzle}|${solution}|2024-01-02T03:04:05.000Z|-1`,
    `Valid||${puzzle}|${solution}||`,
  ].join("\n"));

  assert.equal(parsed.records.length, 1);
  assert.equal(parsed.records[0]?.name, "Valid");
  assert.deepEqual(parsed.errors.map((error) => error.line), [1, 2, 3, 4, 5]);
  assert.match(parsed.errors[0]?.message ?? "", /columns/i);
});

test("deduplicates by normalized puzzle and merges only complementary metadata", () => {
  const dotted = puzzle.replaceAll("0", ".");
  const parsed = parseLegacyPipeText([
    `Newest name||${puzzle}||2026-07-15T10:00:00.000Z|`,
    `|hard|${dotted}|${solution}|2026-07-14T10:00:00.000Z|99`,
  ].join("\n"));

  assert.deepEqual(parsed.errors, []);
  assert.deepEqual(parsed.records, [{
    puzzle,
    name: "Newest name",
    difficulty: "hard",
    solution,
    seed: 99,
    source: "legacy-windows-pipe",
    createdAt: "2026-07-14T10:00:00.000Z",
  }]);
});
