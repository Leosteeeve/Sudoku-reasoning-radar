import assert from "node:assert/strict";
import test from "node:test";

import {
  CoreClient,
  CoreProtocolError,
  type CoreRequest,
  type CoreResponse,
  type SolveRequestV1,
  type SolveResponseV1,
  type SolveStepV1,
} from "../src/index.ts";

const canonicalPuzzle =
  "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

const difficulty = {
  grade: "easy",
  score: 4,
  givens: 30,
  emptyCells: 51,
  maxBranchDepth: 0,
  hardestTechnique: "hidden_single",
  stats: {
    nakedSingles: 2,
    hiddenSingles: 1,
    lockedCandidates: 0,
    boxLineReductions: 0,
    nakedPairs: 0,
    hiddenPairs: 0,
    xWings: 0,
    guesses: 0,
    backtracks: 0,
    contradictions: 0,
    totalSteps: 3,
  },
} as const;

const traceStep = {
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
    edges: [
      {
        from: "step-000001",
        to: "step-000001-cell-1",
        relation: "excludes",
      },
    ],
  },
  branch: { depth: 1, parentStepId: "step-000000" },
  explanationKey: "trace.candidateRemovedByLogic",
  explanationParams: { number: 7, unit: "row" },
} as const satisfies SolveStepV1;

function solveResponse(overrides: Record<string, unknown> = {}): CoreResponse {
  return {
    schemaVersion: 1,
    operation: "solve",
    ok: true,
    result: "unique",
    solution:
      "534678912672195348198342567859761423426853791713924856961537284287419635345286179",
    elapsedMicroseconds: 42,
    difficulty,
    steps: [traceStep],
    ...overrides,
  } as CoreResponse;
}

test("CoreClient exposes exactly the four approved public methods", () => {
  const requestContract: SolveRequestV1 = {
    schemaVersion: 1,
    puzzle: canonicalPuzzle,
    mode: "human",
    includeTrace: true,
  };
  const responseContract: SolveResponseV1 = solveResponse() as SolveResponseV1;
  assert.equal(requestContract.mode, "human");
  assert.equal(responseContract.elapsedMicroseconds, 42);
  assert.deepEqual(
    Object.getOwnPropertyNames(CoreClient.prototype).filter((name) => name !== "constructor").sort(),
    ["analyze", "generate", "hint", "solve"],
  );
});

test("solve normalizes and validates its outgoing request", async () => {
  let sent: CoreRequest | undefined;
  const client = new CoreClient((json) => {
    sent = JSON.parse(json) as CoreRequest;
    return JSON.stringify(solveResponse());
  });

  const dotted = canonicalPuzzle.replaceAll("0", ".").match(/.{1,9}/g)?.join("\n") ?? "";
  await client.solve({ puzzle: dotted });

  assert.deepEqual(sent, {
    schemaVersion: 1,
    operation: "solve",
    puzzle: canonicalPuzzle,
    mode: "smart",
    includeTrace: true,
  });
});

test("all four methods send their matching versioned operation", async () => {
  const operations: string[] = [];
  const client = new CoreClient((json) => {
    const request = JSON.parse(json) as CoreRequest;
    operations.push(request.operation);
    if (request.operation === "solve") return JSON.stringify(solveResponse({ steps: [] }));
    if (request.operation === "generate") {
      return JSON.stringify({
        schemaVersion: 1,
        operation: "generate",
        ok: true,
        puzzle: canonicalPuzzle,
        solution: canonicalPuzzle,
        difficulty: "easy",
        givens: 30,
        attempts: 1,
        seed: 7,
        report: difficulty,
      });
    }
    if (request.operation === "hint") {
      return JSON.stringify({
        schemaVersion: 1,
        operation: "hint",
        ok: true,
        available: true,
        disclosure: "direct",
        step: traceStep,
      });
    }
    return JSON.stringify({
      schemaVersion: 1,
      operation: "analyze",
      ok: true,
      result: "unique",
      difficulty,
    });
  });

  await client.solve({ puzzle: canonicalPuzzle, includeTrace: false });
  await client.generate({ difficulty: "easy", seed: 7 });
  await client.hint({ puzzle: canonicalPuzzle, level: "direct" });
  await client.analyze({ puzzle: canonicalPuzzle, mode: "human" });
  assert.deepEqual(operations, ["solve", "generate", "hint", "analyze"]);
});

test("rejects invalid outgoing values before calling transport", async () => {
  let calls = 0;
  const client = new CoreClient(() => {
    calls += 1;
    return "{}";
  });

  await assert.rejects(client.solve({ puzzle: "123", mode: "smart" }), CoreProtocolError);
  await assert.rejects(
    client.generate({ difficulty: "easy", seed: -1 }),
    /seed/i,
  );
  assert.equal(calls, 0);
});

test("accepts a representative language-neutral SolveTrace response", async () => {
  const client = new CoreClient(() => JSON.stringify(solveResponse()));
  const response = await client.solve({ puzzle: canonicalPuzzle });

  assert.equal(response.steps?.[0]?.technique, "locked_candidate");
  assert.deepEqual(response.steps?.[0]?.candidateDeltas[0]?.removedDigits, [9]);
  assert.equal(response.steps?.[0]?.evidence.edges[0]?.relation, "excludes");
  assert.equal(response.steps?.[0]?.branch.parentStepId, "step-000000");
});

test("rejects malformed transport JSON and schema mismatches", async () => {
  const malformed = new CoreClient(() => "not-json");
  await assert.rejects(malformed.solve({ puzzle: canonicalPuzzle }), /transport JSON/i);

  const wrongVersion = new CoreClient(() =>
    JSON.stringify(solveResponse({ schemaVersion: 2 })),
  );
  await assert.rejects(wrongVersion.solve({ puzzle: canonicalPuzzle }), /schema version/i);
});

test("rejects structurally invalid successful responses", async () => {
  const invalidTrace = new CoreClient(() =>
    JSON.stringify(solveResponse({ steps: [{ ...traceStep, action: "translate-prose" }] })),
  );
  await assert.rejects(invalidTrace.solve({ puzzle: canonicalPuzzle }), /response/i);

  const unknownResponseField = new CoreClient(() =>
    JSON.stringify(solveResponse({ surprise: true })),
  );
  await assert.rejects(unknownResponseField.solve({ puzzle: canonicalPuzzle }), /response/i);

  const invalidEvidenceKind = new CoreClient(() => JSON.stringify(solveResponse({
    steps: [{
      ...traceStep,
      evidence: {
        ...traceStep.evidence,
        nodes: [{ id: "not-a-stable-id", kind: "paragraph" }],
      },
    }],
  })));
  await assert.rejects(invalidEvidenceKind.solve({ puzzle: canonicalPuzzle }), /response/i);

  const missingHintDisclosure = new CoreClient(() => JSON.stringify({
    schemaVersion: 1,
    operation: "hint",
    ok: true,
    available: false,
    step: null,
  }));
  await assert.rejects(
    missingHintDisclosure.hint({ puzzle: canonicalPuzzle, level: "gentle" }),
    /response/i,
  );
});
