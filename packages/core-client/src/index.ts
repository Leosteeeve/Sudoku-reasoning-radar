export const SCHEMA_VERSION = 1 as const;

export type SolverMode = "human-logic" | "smart" | "turbo";
export type PuzzleDifficulty = "easy" | "medium" | "hard" | "expert";
export type HintLevel = "gentle" | "technique" | "direct";
export type SolveClassification = "invalid" | "unsolvable" | "unique" | "multiple";
export type TechniqueId =
  | "naked-single"
  | "hidden-single"
  | "locked-candidate"
  | "box-line-reduction"
  | "naked-pair"
  | "hidden-pair"
  | "x-wing";
export type TraceAction =
  | "observe"
  | "eliminate"
  | "place"
  | "inform"
  | "branch"
  | "contradict"
  | "revert"
  | "complete";
export type EvidenceRelation = "supports" | "excludes" | "conflicts" | "branches" | "reverts";

export interface CellRef {
  row: number;
  col: number;
}

export interface CandidateDelta {
  cell: CellRef;
  beforeMask: number;
  afterMask: number;
  removedMask: number;
}

export interface EvidenceNode {
  id: string;
  kind: string;
  row?: number;
  col?: number;
  value?: number;
  mask?: number;
}

export interface EvidenceEdge {
  from: string;
  to: string;
  relation: EvidenceRelation;
}

export interface TraceStep {
  id: string;
  technique: TechniqueId | null;
  action: TraceAction;
  targets: CellRef[];
  candidateDeltas: CandidateDelta[];
  evidence: { nodes: EvidenceNode[]; edges: EvidenceEdge[] };
  branch: { depth: number; parentId: string | null };
  explanationKey: string;
  params: Record<string, number | string>;
}

export interface DifficultyStats {
  nakedSingles: number;
  hiddenSingles: number;
  lockedCandidates: number;
  boxLineReductions: number;
  nakedPairs: number;
  hiddenPairs: number;
  xWings: number;
  guesses: number;
  backtracks: number;
  contradictions: number;
  totalSteps: number;
}

export interface DifficultyReport {
  grade: "easy" | "medium" | "hard" | "expert" | "invalid" | "multiple" | "unsolvable";
  score: number;
  givens: number;
  emptyCells: number;
  maxBranchDepth: number;
  hardestTechnique: TechniqueId | null;
  stats: DifficultyStats;
}

export interface SolveRequest {
  schemaVersion: 1;
  operation: "solve";
  puzzle: string;
  mode: SolverMode;
  includeTrace: boolean;
}

export interface GenerateRequest {
  schemaVersion: 1;
  operation: "generate";
  difficulty: PuzzleDifficulty;
  seed?: number;
}

export interface HintRequest {
  schemaVersion: 1;
  operation: "hint";
  puzzle: string;
  level: HintLevel;
}

export interface AnalyzeRequest {
  schemaVersion: 1;
  operation: "analyze";
  puzzle: string;
  mode: SolverMode;
}

export type CoreRequest = SolveRequest | GenerateRequest | HintRequest | AnalyzeRequest;

interface SuccessEnvelope<Operation extends CoreRequest["operation"]> {
  schemaVersion: 1;
  operation: Operation;
  ok: true;
}

export interface SolveResponse extends SuccessEnvelope<"solve"> {
  result: SolveClassification;
  solution: string | null;
  elapsedMicros: number;
  difficulty: DifficultyReport;
  steps?: TraceStep[];
}

export interface GenerateResponse extends SuccessEnvelope<"generate"> {
  puzzle: string;
  solution: string;
  difficulty: PuzzleDifficulty;
  givens: number;
  attempts: number;
  seed: number;
  report: DifficultyReport;
}

export interface HintResponse extends SuccessEnvelope<"hint"> {
  available: boolean;
  step: TraceStep | null;
}

export interface AnalyzeResponse extends SuccessEnvelope<"analyze"> {
  result: SolveClassification;
  difficulty: DifficultyReport;
}

export interface ErrorResponse {
  schemaVersion: 1;
  operation: CoreRequest["operation"] | null;
  ok: false;
  error: { code: string; path: string; params: Record<string, number | string> };
}

export type CoreResponse = SolveResponse | GenerateResponse | HintResponse | AnalyzeResponse | ErrorResponse;
export type CoreDispatcher = (requestJson: string) => string | Promise<string>;

export interface SolveOptions {
  puzzle: string;
  mode?: SolverMode;
  includeTrace?: boolean;
}

export interface GenerateOptions {
  difficulty: PuzzleDifficulty;
  seed?: number;
}

export interface HintOptions {
  puzzle: string;
  level?: HintLevel;
}

export interface AnalyzeOptions {
  puzzle: string;
  mode?: SolverMode;
}

export class CoreProtocolError extends Error {
  readonly code: string;
  readonly path?: string;

  constructor(message: string, code = "protocol_error", path?: string) {
    super(message);
    this.name = "CoreProtocolError";
    this.code = code;
    if (path !== undefined) this.path = path;
  }
}

const modes = ["human-logic", "smart", "turbo"] as const;
const difficulties = ["easy", "medium", "hard", "expert"] as const;
const hintLevels = ["gentle", "technique", "direct"] as const;
const classifications = ["invalid", "unsolvable", "unique", "multiple"] as const;
const techniques = [
  "naked-single",
  "hidden-single",
  "locked-candidate",
  "box-line-reduction",
  "naked-pair",
  "hidden-pair",
  "x-wing",
] as const;
const actions = [
  "observe",
  "eliminate",
  "place",
  "inform",
  "branch",
  "contradict",
  "revert",
  "complete",
] as const;
const relations = ["supports", "excludes", "conflicts", "branches", "reverts"] as const;
const grades = [...difficulties, "invalid", "multiple", "unsolvable"] as const;

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isMember<T extends readonly string[]>(values: T, value: unknown): value is T[number] {
  return typeof value === "string" && values.includes(value as T[number]);
}

function isInteger(value: unknown, minimum = 0, maximum = Number.MAX_SAFE_INTEGER): value is number {
  return typeof value === "number" && Number.isSafeInteger(value) && value >= minimum && value <= maximum;
}

function protocolFailure(detail: string): never {
  throw new CoreProtocolError(`Invalid core response: ${detail}`, "invalid_response");
}

function normalizePuzzle(puzzle: unknown): string {
  if (typeof puzzle !== "string") {
    throw new CoreProtocolError("Puzzle must be a string", "wrong_type", "$.puzzle");
  }
  const normalized = puzzle.replace(/\s/g, "").replace(/\./g, "0");
  if (!/^[0-9]{81}$/.test(normalized)) {
    throw new CoreProtocolError("Puzzle must contain exactly 81 digits or dots", "malformed_puzzle", "$.puzzle");
  }
  return normalized;
}

function validateCell(value: unknown, detail: string): asserts value is CellRef {
  if (!isRecord(value) || !isInteger(value.row, 0, 8) || !isInteger(value.col, 0, 8)) {
    protocolFailure(detail);
  }
}

function validateTraceStep(value: unknown): asserts value is TraceStep {
  if (!isRecord(value) || typeof value.id !== "string"
      || !(value.technique === null || isMember(techniques, value.technique))
      || !isMember(actions, value.action) || !Array.isArray(value.targets)
      || !Array.isArray(value.candidateDeltas) || !isRecord(value.evidence)
      || !isRecord(value.branch) || typeof value.explanationKey !== "string"
      || !isRecord(value.params)) {
    protocolFailure("trace step shape");
  }
  value.targets.forEach((target) => validateCell(target, "trace target"));
  for (const delta of value.candidateDeltas) {
    if (!isRecord(delta)) protocolFailure("candidate delta");
    validateCell(delta.cell, "candidate delta cell");
    if (!isInteger(delta.beforeMask, 0, 511) || !isInteger(delta.afterMask, 0, 511)
        || !isInteger(delta.removedMask, 0, 511)) {
      protocolFailure("candidate masks");
    }
  }
  if (!Array.isArray(value.evidence.nodes) || !Array.isArray(value.evidence.edges)) {
    protocolFailure("evidence graph");
  }
  for (const node of value.evidence.nodes) {
    if (!isRecord(node) || typeof node.id !== "string" || typeof node.kind !== "string") {
      protocolFailure("evidence node");
    }
    if (node.row !== undefined && !isInteger(node.row, 0, 8)) protocolFailure("evidence node row");
    if (node.col !== undefined && !isInteger(node.col, 0, 8)) protocolFailure("evidence node col");
    if (node.value !== undefined && !isInteger(node.value, 1, 9)) protocolFailure("evidence node value");
    if (node.mask !== undefined && !isInteger(node.mask, 0, 511)) protocolFailure("evidence node mask");
  }
  for (const edge of value.evidence.edges) {
    if (!isRecord(edge) || typeof edge.from !== "string" || typeof edge.to !== "string"
        || !isMember(relations, edge.relation)) {
      protocolFailure("evidence edge");
    }
  }
  if (!isInteger(value.branch.depth, 0)
      || !(value.branch.parentId === null || typeof value.branch.parentId === "string")) {
    protocolFailure("branch metadata");
  }
  for (const parameter of Object.values(value.params)) {
    if (!(typeof parameter === "string" || isInteger(parameter, Number.MIN_SAFE_INTEGER))) {
      protocolFailure("localization parameter");
    }
  }
}

function validateDifficulty(value: unknown): asserts value is DifficultyReport {
  if (!isRecord(value) || !isMember(grades, value.grade) || !isInteger(value.score)
      || !isInteger(value.givens, 0, 81) || !isInteger(value.emptyCells, 0, 81)
      || !isInteger(value.maxBranchDepth) || !(value.hardestTechnique === null
        || isMember(techniques, value.hardestTechnique)) || !isRecord(value.stats)) {
    protocolFailure("difficulty report");
  }
  const keys: (keyof DifficultyStats)[] = [
    "nakedSingles", "hiddenSingles", "lockedCandidates", "boxLineReductions",
    "nakedPairs", "hiddenPairs", "xWings", "guesses", "backtracks",
    "contradictions", "totalSteps",
  ];
  for (const key of keys) {
    if (!isInteger(value.stats[key])) protocolFailure(`difficulty stats.${key}`);
  }
}

function validatePuzzleString(value: unknown, detail: string): asserts value is string {
  if (typeof value !== "string" || !/^[0-9]{81}$/.test(value)) protocolFailure(detail);
}

function validateResponse(value: unknown, operation: CoreRequest["operation"]): CoreResponse {
  if (!isRecord(value)) protocolFailure("root object");
  if (value.schemaVersion !== SCHEMA_VERSION) {
    throw new CoreProtocolError("Core response schema version mismatch", "schema_mismatch", "$.schemaVersion");
  }
  if (value.operation !== operation || typeof value.ok !== "boolean") protocolFailure("envelope");
  if (!value.ok) {
    if (!isRecord(value.error) || typeof value.error.code !== "string"
        || typeof value.error.path !== "string" || !isRecord(value.error.params)) {
      protocolFailure("error envelope");
    }
    for (const parameter of Object.values(value.error.params)) {
      if (!(typeof parameter === "string" || isInteger(parameter, Number.MIN_SAFE_INTEGER))) {
        protocolFailure("error parameter");
      }
    }
    throw new CoreProtocolError(`Core error: ${value.error.code}`, value.error.code, value.error.path);
  }

  if (operation === "solve") {
    if (!isMember(classifications, value.result)
        || !(value.solution === null || (typeof value.solution === "string" && /^[0-9]{81}$/.test(value.solution)))
        || !isInteger(value.elapsedMicros)) protocolFailure("solve response");
    validateDifficulty(value.difficulty);
    if (value.steps !== undefined) {
      if (!Array.isArray(value.steps)) protocolFailure("solve trace");
      value.steps.forEach(validateTraceStep);
    }
  } else if (operation === "generate") {
    validatePuzzleString(value.puzzle, "generated puzzle");
    validatePuzzleString(value.solution, "generated solution");
    if (!isMember(difficulties, value.difficulty) || !isInteger(value.givens, 0, 81)
        || !isInteger(value.attempts) || !isInteger(value.seed, 0, 0xffffffff)) {
      protocolFailure("generate response");
    }
    validateDifficulty(value.report);
  } else if (operation === "hint") {
    if (typeof value.available !== "boolean" || !(value.step === null || isRecord(value.step))) {
      protocolFailure("hint response");
    }
    if (value.step !== null) validateTraceStep(value.step);
  } else {
    if (!isMember(classifications, value.result)) protocolFailure("analyze response");
    validateDifficulty(value.difficulty);
  }
  return value as unknown as CoreResponse;
}

async function dispatch<Response extends CoreResponse>(
  dispatcher: CoreDispatcher,
  request: CoreRequest,
): Promise<Response> {
  let decoded: unknown;
  const responseJson = await dispatcher(JSON.stringify(request));
  if (typeof responseJson !== "string") {
    throw new CoreProtocolError("WASM transport did not return JSON text", "transport_type");
  }
  try {
    decoded = JSON.parse(responseJson) as unknown;
  } catch {
    throw new CoreProtocolError("Malformed transport JSON", "transport_json");
  }
  return validateResponse(decoded, request.operation) as Response;
}

export class CoreClient {
  readonly #dispatcher: CoreDispatcher;

  constructor(dispatcher: CoreDispatcher) {
    if (typeof dispatcher !== "function") {
      throw new CoreProtocolError("Core dispatcher must be a function", "transport_type");
    }
    this.#dispatcher = dispatcher;
  }

  async solve(options: SolveOptions): Promise<SolveResponse> {
    const mode = options.mode ?? "smart";
    if (!isMember(modes, mode)) throw new CoreProtocolError("Invalid solver mode", "invalid_value", "$.mode");
    if (options.includeTrace !== undefined && typeof options.includeTrace !== "boolean") {
      throw new CoreProtocolError("includeTrace must be boolean", "wrong_type", "$.includeTrace");
    }
    return await dispatch<SolveResponse>(this.#dispatcher, {
      schemaVersion: SCHEMA_VERSION,
      operation: "solve",
      puzzle: normalizePuzzle(options.puzzle),
      mode,
      includeTrace: options.includeTrace ?? true,
    });
  }

  async generate(options: GenerateOptions): Promise<GenerateResponse> {
    if (!isMember(difficulties, options.difficulty)) {
      throw new CoreProtocolError("Invalid difficulty", "invalid_value", "$.difficulty");
    }
    if (options.seed !== undefined && !isInteger(options.seed, 0, 0xffffffff)) {
      throw new CoreProtocolError("Seed must be a uint32 integer", "out_of_range", "$.seed");
    }
    const request: GenerateRequest = {
      schemaVersion: SCHEMA_VERSION,
      operation: "generate",
      difficulty: options.difficulty,
    };
    if (options.seed !== undefined) request.seed = options.seed;
    return await dispatch<GenerateResponse>(this.#dispatcher, request);
  }

  async hint(options: HintOptions): Promise<HintResponse> {
    const level = options.level ?? "gentle";
    if (!isMember(hintLevels, level)) throw new CoreProtocolError("Invalid hint level", "invalid_value", "$.level");
    return await dispatch<HintResponse>(this.#dispatcher, {
      schemaVersion: SCHEMA_VERSION,
      operation: "hint",
      puzzle: normalizePuzzle(options.puzzle),
      level,
    });
  }

  async analyze(options: AnalyzeOptions): Promise<AnalyzeResponse> {
    const mode = options.mode ?? "smart";
    if (!isMember(modes, mode)) throw new CoreProtocolError("Invalid solver mode", "invalid_value", "$.mode");
    return await dispatch<AnalyzeResponse>(this.#dispatcher, {
      schemaVersion: SCHEMA_VERSION,
      operation: "analyze",
      puzzle: normalizePuzzle(options.puzzle),
      mode,
    });
  }
}
