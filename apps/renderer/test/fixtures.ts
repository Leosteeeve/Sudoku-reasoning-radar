import { CoreClient, type SolveStepV1 } from "@srr/core-client";

export const puzzle =
  "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

export const fixtureStep: SolveStepV1 = {
  id: "step-000001",
  technique: "locked_candidate",
  action: "eliminate",
  targets: [{ row: 2, col: 3 }],
  candidateDeltas: [
    { cell: { row: 2, col: 3 }, beforeMask: 448, afterMask: 192, removedDigits: [9] },
  ],
  evidence: {
    nodes: [
      { id: "step-000001", kind: "branch" },
      { id: "step-000001-cell-1", kind: "cell", cell: { row: 2, col: 3 }, digit: 7 },
    ],
    edges: [{ from: "step-000001", to: "step-000001-cell-1", relation: "conflicts" }],
  },
  branch: { depth: 0 },
  explanationKey: "trace.candidateRemovedByLogic",
  explanationParams: { number: 9, unit: "box" },
};

const difficulty = {
  grade: "medium",
  score: 10,
  givens: 30,
  emptyCells: 51,
  maxBranchDepth: 0,
  hardestTechnique: "locked_candidate",
  stats: {
    nakedSingles: 0,
    hiddenSingles: 0,
    lockedCandidates: 1,
    boxLineReductions: 0,
    nakedPairs: 0,
    hiddenPairs: 0,
    xWings: 0,
    guesses: 0,
    backtracks: 0,
    contradictions: 0,
    totalSteps: 1,
  },
};

export function fixtureClient(responseOverride: Record<string, unknown> = {}): CoreClient {
  return new CoreClient(() => JSON.stringify({
    schemaVersion: 1,
    operation: "solve",
    ok: true,
    result: "unique",
    solution: "534678912672195348198342567859761423426853791713924856961537284287419635345286179",
    elapsedMicroseconds: 42,
    difficulty,
    steps: [fixtureStep],
    ...responseOverride,
  }));
}
