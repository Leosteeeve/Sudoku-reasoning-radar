import type { CellRef, SolveStepV1, SolverMode } from "@srr/core-client";

export type Language = "zh" | "en";
export type Theme = "light" | "dark";

type Snapshot = { values: number[]; candidates: number[] };

export interface SessionState {
  givens: number[];
  values: number[];
  candidates: number[];
  selected: CellRef | null;
  noteMode: boolean;
  solverMode: SolverMode;
  language: Language;
  trace: SolveStepV1[];
  currentStep: number;
  playing: boolean;
  speed: number;
  commandOpen: boolean;
  lessonOpen: boolean;
  constellationOpen: boolean;
  theme: Theme;
  highContrast: boolean;
  past: Snapshot[];
  future: Snapshot[];
}

export type SessionAction =
  | { type: "select"; cell: CellRef }
  | { type: "moveSelection"; rowDelta: number; colDelta: number }
  | { type: "enterDigit"; digit: number }
  | { type: "clearCell" }
  | { type: "toggleNotes" }
  | { type: "setSolverMode"; mode: SolverMode }
  | { type: "setLanguage"; language: Language }
  | { type: "undo" }
  | { type: "redo" }
  | { type: "loadTrace"; steps: SolveStepV1[] }
  | { type: "setStep"; step: number }
  | { type: "togglePlay" }
  | { type: "setSpeed"; speed: number }
  | { type: "setCommandOpen"; open: boolean }
  | { type: "setLessonOpen"; open: boolean }
  | { type: "setConstellationOpen"; open: boolean }
  | { type: "setTheme"; theme: Theme }
  | { type: "setHighContrast"; enabled: boolean }
  | { type: "dismissOverlays" };

export interface KeyModifiers {
  ctrlKey?: boolean;
  metaKey?: boolean;
  shiftKey?: boolean;
}

function parsePuzzle(puzzle: string): number[] {
  if (!/^[0-9]{81}$/.test(puzzle)) throw new Error("Puzzle must contain 81 digits");
  return [...puzzle].map(Number);
}

export function createInitialSession(
  puzzle: string,
  options: { language: Language },
): SessionState {
  const givens = parsePuzzle(puzzle);
  return {
    givens,
    values: [...givens],
    candidates: Array<number>(81).fill(0),
    selected: null,
    noteMode: false,
    solverMode: "smart",
    language: options.language,
    trace: [],
    currentStep: 0,
    playing: false,
    speed: 1,
    commandOpen: false,
    lessonOpen: false,
    constellationOpen: false,
    theme: "light",
    highContrast: false,
    past: [],
    future: [],
  };
}

function withEdit(
  state: SessionState,
  values: number[],
  candidates: number[],
): SessionState {
  return {
    ...state,
    values,
    candidates,
    past: [...state.past, { values: [...state.values], candidates: [...state.candidates] }],
    future: [],
  };
}

function selectedIndex(state: SessionState): number | null {
  return state.selected ? state.selected.row * 9 + state.selected.col : null;
}

export function sessionReducer(state: SessionState, action: SessionAction): SessionState {
  switch (action.type) {
    case "select":
      return { ...state, selected: action.cell };
    case "moveSelection": {
      const current = state.selected ?? { row: 0, col: 0 };
      return {
        ...state,
        selected: {
          row: Math.max(0, Math.min(8, current.row + action.rowDelta)),
          col: Math.max(0, Math.min(8, current.col + action.colDelta)),
        },
      };
    }
    case "enterDigit": {
      const index = selectedIndex(state);
      if (index === null || action.digit < 1 || action.digit > 9 || state.givens[index] !== 0) return state;
      const values = [...state.values];
      const candidates = [...state.candidates];
      if (state.noteMode) {
        candidates[index] ^= 1 << (action.digit - 1);
      } else {
        if (values[index] === action.digit && candidates[index] === 0) return state;
        values[index] = action.digit;
        candidates[index] = 0;
      }
      return withEdit(state, values, candidates);
    }
    case "clearCell": {
      const index = selectedIndex(state);
      if (index === null || state.givens[index] !== 0
          || (state.values[index] === 0 && state.candidates[index] === 0)) return state;
      const values = [...state.values];
      const candidates = [...state.candidates];
      values[index] = 0;
      candidates[index] = 0;
      return withEdit(state, values, candidates);
    }
    case "toggleNotes":
      return { ...state, noteMode: !state.noteMode };
    case "setSolverMode":
      return { ...state, solverMode: action.mode };
    case "setLanguage":
      return { ...state, language: action.language };
    case "undo": {
      const previous = state.past.at(-1);
      if (!previous) return state;
      return {
        ...state,
        values: [...previous.values],
        candidates: [...previous.candidates],
        past: state.past.slice(0, -1),
        future: [
          { values: [...state.values], candidates: [...state.candidates] },
          ...state.future,
        ],
      };
    }
    case "redo": {
      const next = state.future[0];
      if (!next) return state;
      return {
        ...state,
        values: [...next.values],
        candidates: [...next.candidates],
        past: [
          ...state.past,
          { values: [...state.values], candidates: [...state.candidates] },
        ],
        future: state.future.slice(1),
      };
    }
    case "loadTrace":
      return { ...state, trace: action.steps, currentStep: 0, playing: false };
    case "setStep":
      return {
        ...state,
        currentStep: Math.max(0, Math.min(Math.max(0, state.trace.length - 1), action.step)),
      };
    case "togglePlay":
      return { ...state, playing: !state.playing };
    case "setSpeed":
      return { ...state, speed: action.speed };
    case "setCommandOpen":
      return { ...state, commandOpen: action.open };
    case "setLessonOpen":
      return { ...state, lessonOpen: action.open };
    case "setConstellationOpen":
      return { ...state, constellationOpen: action.open };
    case "setTheme":
      return { ...state, theme: action.theme };
    case "setHighContrast":
      return { ...state, highContrast: action.enabled };
    case "dismissOverlays":
      return { ...state, commandOpen: false, lessonOpen: false, constellationOpen: false };
  }
}

export function actionForKey(key: string, modifiers: KeyModifiers = {}): SessionAction | null {
  const command = modifiers.ctrlKey || modifiers.metaKey;
  if (command && key.toLowerCase() === "z") {
    return modifiers.shiftKey ? { type: "redo" } : { type: "undo" };
  }
  if (command && key.toLowerCase() === "y") return { type: "redo" };
  if (/^[1-9]$/.test(key)) return { type: "enterDigit", digit: Number(key) };
  if (key === "ArrowUp") return { type: "moveSelection", rowDelta: -1, colDelta: 0 };
  if (key === "ArrowDown") return { type: "moveSelection", rowDelta: 1, colDelta: 0 };
  if (key === "ArrowLeft") return { type: "moveSelection", rowDelta: 0, colDelta: -1 };
  if (key === "ArrowRight") return { type: "moveSelection", rowDelta: 0, colDelta: 1 };
  if (key === "Delete" || key === "Backspace") return { type: "clearCell" };
  if (key.toLowerCase() === "n") return { type: "toggleNotes" };
  if (key === " ") return { type: "togglePlay" };
  if (key === "/") return { type: "setCommandOpen", open: true };
  if (key === "Escape") return { type: "dismissOverlays" };
  return null;
}
