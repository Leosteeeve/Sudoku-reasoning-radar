import { expect, test } from "vitest";

import { actionForKey, createInitialSession, sessionReducer } from "../src/session.ts";

test("editing a cell is undoable and redoable without changing givens", () => {
  const initial = createInitialSession(
    "530000000" + "0".repeat(72),
    { language: "en" },
  );

  const selected = sessionReducer(initial, { type: "select", cell: { row: 0, col: 2 } });
  const edited = sessionReducer(selected, { type: "enterDigit", digit: 4 });
  expect(edited.values[2]).toBe(4);

  const givenSelected = sessionReducer(edited, { type: "select", cell: { row: 0, col: 0 } });
  const givenEdit = sessionReducer(givenSelected, { type: "enterDigit", digit: 9 });
  expect(givenEdit.values[0]).toBe(5);

  const undone = sessionReducer(givenEdit, { type: "undo" });
  expect(undone.values[2]).toBe(0);
  const redone = sessionReducer(undone, { type: "redo" });
  expect(redone.values[2]).toBe(4);
});

test("notes mode toggles candidates and undo restores the candidate mask", () => {
  let state = createInitialSession("0".repeat(81), { language: "en" });
  state = sessionReducer(state, { type: "select", cell: { row: 4, col: 4 } });
  state = sessionReducer(state, { type: "toggleNotes" });
  state = sessionReducer(state, { type: "enterDigit", digit: 7 });
  expect(state.candidates[40]).toBe(1 << 6);
  expect(state.values[40]).toBe(0);

  state = sessionReducer(state, { type: "undo" });
  expect(state.candidates[40]).toBe(0);
  state = sessionReducer(state, { type: "redo" });
  expect(state.candidates[40]).toBe(1 << 6);
});

test("notes mode does not add candidates to a cell that already has a value", () => {
  let state = createInitialSession("0".repeat(81), { language: "en" });
  state = sessionReducer(state, { type: "select", cell: { row: 0, col: 0 } });
  state = sessionReducer(state, { type: "enterDigit", digit: 4 });
  state = sessionReducer(state, { type: "toggleNotes" });
  state = sessionReducer(state, { type: "enterDigit", digit: 7 });
  expect(state.values[0]).toBe(4);
  expect(state.candidates[0]).toBe(0);
});

test("keyboard actions navigate, edit, toggle modes, and open workspace controls", () => {
  let state = createInitialSession("0".repeat(81), { language: "zh" });
  state = sessionReducer(state, { type: "select", cell: { row: 0, col: 0 } });
  for (const key of ["ArrowRight", "ArrowDown", "n", "8"]) {
    const action = actionForKey(key);
    expect(action).not.toBeNull();
    if (!action) throw new Error("expected keyboard action");
    state = sessionReducer(state, action);
  }
  expect(state.selected).toEqual({ row: 1, col: 1 });
  expect(state.noteMode).toBe(true);
  expect(state.candidates[10]).toBe(1 << 7);

  expect(actionForKey("z", { ctrlKey: true })).toEqual({ type: "undo" });
  expect(actionForKey("y", { metaKey: true })).toEqual({ type: "redo" });
  expect(actionForKey(" ")).toEqual({ type: "togglePlay" });
  expect(actionForKey("/")).toEqual({ type: "setCommandOpen", open: true });
  expect(actionForKey("Escape")).toEqual({ type: "dismissOverlays" });
  expect(actionForKey("Backspace")).toEqual({ type: "clearCell" });
});

test("trace loading and timeline actions clamp steps and control playback", () => {
  const steps = [
    { id: "step-000001", action: "analyze" },
    { id: "step-000002", action: "complete" },
  ] as never[];
  let state = createInitialSession("0".repeat(81), { language: "en" });
  state = sessionReducer(state, { type: "loadTrace", steps });
  expect(state.currentStep).toBe(0);
  expect(state.playing).toBe(false);
  state = sessionReducer(state, { type: "togglePlay" });
  state = sessionReducer(state, { type: "setSpeed", speed: 2 });
  expect(state.playing).toBe(true);
  expect(state.speed).toBe(2);
  state = sessionReducer(state, { type: "setStep", step: 99 });
  expect(state.currentStep).toBe(1);
  expect(state.playing).toBe(false);
});

test("an empty or completed timeline cannot enter playing state", () => {
  let state = createInitialSession("0".repeat(81), { language: "en" });
  state = sessionReducer(state, { type: "togglePlay" });
  expect(state.playing).toBe(false);

  const steps = [{ id: "step-000001" }, { id: "step-000002" }] as never[];
  state = sessionReducer(state, { type: "loadTrace", steps });
  state = sessionReducer(state, { type: "setStep", step: 1 });
  state = sessionReducer(state, { type: "togglePlay" });
  expect(state.playing).toBe(false);
});

test("advanced constellation auto-opens once and stays closed after manual dismissal", () => {
  let state = createInitialSession("0".repeat(81), { language: "en" });
  state = sessionReducer(state, { type: "showAdvancedConstellation", stepId: "step-000001" });
  expect(state.constellationOpen).toBe(true);
  state = sessionReducer(state, { type: "setConstellationOpen", open: false });
  state = sessionReducer(state, { type: "showAdvancedConstellation", stepId: "step-000001" });
  expect(state.constellationOpen).toBe(false);
});

test("workspace preferences switch instantly through reducer state", () => {
  let state = createInitialSession("0".repeat(81), { language: "en" });
  state = sessionReducer(state, { type: "setLanguage", language: "zh" });
  state = sessionReducer(state, { type: "setSolverMode", mode: "human" });
  state = sessionReducer(state, { type: "setTheme", theme: "dark" });
  state = sessionReducer(state, { type: "setHighContrast", enabled: true });
  state = sessionReducer(state, { type: "setConstellationOpen", open: true });
  expect(state.language).toBe("zh");
  expect(state.solverMode).toBe("human");
  expect(state.theme).toBe("dark");
  expect(state.highContrast).toBe(true);
  expect(state.constellationOpen).toBe(true);
});
