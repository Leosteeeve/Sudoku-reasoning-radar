import type { SolveStepV1 } from "@srr/core-client";
import type { Dispatch } from "react";
import { translate, type MessageKey } from "../i18n";
import { actionForKey, type Language, type SessionAction, type SessionState } from "../session";

interface BoardProps {
  state: SessionState;
  dispatch: Dispatch<SessionAction>;
}

function hasCell(cells: { row: number; col: number }[], row: number, col: number): boolean {
  return cells.some((cell) => cell.row === row && cell.col === col);
}

function isPeer(selected: SessionState["selected"], row: number, col: number): boolean {
  if (!selected || (selected.row === row && selected.col === col)) return false;
  return selected.row === row || selected.col === col
    || (Math.floor(selected.row / 3) === Math.floor(row / 3)
      && Math.floor(selected.col / 3) === Math.floor(col / 3));
}

function candidates(mask: number): number[] {
  return Array.from({ length: 9 }, (_, index) => index + 1)
    .filter((digit) => (mask & (1 << (digit - 1))) !== 0);
}

function cellLabel(language: Language, row: number, col: number, value: number, given: boolean): string {
  if (language === "zh") {
    return `第 ${row + 1} 行，第 ${col + 1} 列，${given ? "题目数字" : value ? "填写数字" : "空格"}${value || ""}`;
  }
  return `Row ${row + 1}, column ${col + 1}, ${given ? `given ${value}` : value ? `entered ${value}` : "empty"}`;
}

function currentStep(state: SessionState): SolveStepV1 | undefined {
  return state.trace[state.currentStep];
}

export function Board({ state, dispatch }: BoardProps) {
  const step = currentStep(state);
  const selectedValue = state.selected
    ? state.values[state.selected.row * 9 + state.selected.col]
    : 0;
  return (
    <section className="board-panel" aria-labelledby="board-heading">
      <div className="board-kicker">
        <h2 id="board-heading">{translate(state.language, "board")}</h2>
        <span className={state.noteMode ? "mode-chip is-active" : "mode-chip"}>
          {translate(state.language, "noteMode")}
        </span>
      </div>
      <table className="sudoku-board" aria-label={translate(state.language, "board")}>
        <tbody>
          {Array.from({ length: 9 }, (_, row) => (
            <tr key={row}>
              {Array.from({ length: 9 }, (_, col) => {
                const index = row * 9 + col;
                const value = state.values[index] ?? 0;
                const given = state.givens[index] !== 0;
                const selected = state.selected?.row === row && state.selected.col === col;
                const peer = isPeer(state.selected, row, col);
                const target = hasCell(step?.targets ?? [], row, col);
                const delta = step?.candidateDeltas.find((item) => item.cell.row === row && item.cell.col === col);
                const contradiction = step?.action === "contradiction" && target;
                const sameDigit = selectedValue !== 0 && value === selectedValue && !selected;
                const notes = candidates(state.candidates[index] ?? 0);
                const states: MessageKey[] = [];
                if (selected) states.push("selected");
                if (peer) states.push("peer");
                if (target) states.push("target");
                if (delta) states.push("eliminated");
                if (contradiction) states.push("contradiction");
                if (sameDigit) states.push("sameDigit");
                const description = [
                  ...notes.map((digit) => `${translate(state.language, "candidate")} ${digit}`),
                  ...states.map((item) => translate(state.language, item)),
                ].join(", ");
                return (
                  <td
                    role="gridcell"
                    key={col}
                    className={`${col % 3 === 2 && col !== 8 ? "box-right " : ""}${row % 3 === 2 && row !== 8 ? "box-bottom" : ""}`}
                  >
                    <button
                      type="button"
                      className={[
                        "sudoku-cell",
                        given ? "is-given" : value ? "is-entered" : "is-empty",
                        selected && "is-selected",
                        peer && "is-peer",
                        target && "is-target",
                        delta && "is-elimination",
                        contradiction && "is-contradiction",
                        sameDigit && "is-same-digit",
                      ].filter(Boolean).join(" ")}
                      aria-label={cellLabel(state.language, row, col, value, given)}
                      aria-description={description || undefined}
                      aria-readonly={given ? "true" : "false"}
                    onClick={() => dispatch({ type: "select", cell: { row, col } })}
                    onKeyDown={(event) => {
                      const action = actionForKey(event.key, event);
                      if (!action || ["togglePlay", "setCommandOpen", "dismissOverlays"].includes(action.type)) return;
                      event.preventDefault();
                      dispatch(action);
                    }}
                  >
                      {given && <span className="cell-origin" aria-hidden="true">●</span>}
                      {value ? <span className="cell-value">{value}</span> : (
                        <span className="candidate-grid" aria-hidden="true">
                          {Array.from({ length: 9 }, (_, candidateIndex) => (
                            <span key={candidateIndex}>{notes.includes(candidateIndex + 1) ? candidateIndex + 1 : ""}</span>
                          ))}
                        </span>
                      )}
                      {target && <span className="state-marker marker-target" aria-hidden="true">◆</span>}
                      {delta && <span className="state-marker marker-elimination" aria-hidden="true">×</span>}
                      {contradiction && <span className="state-marker marker-contradiction" aria-hidden="true">!</span>}
                    </button>
                  </td>
                );
              })}
            </tr>
          ))}
        </tbody>
      </table>
    </section>
  );
}
