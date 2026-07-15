import type { Dispatch } from "react";
import { translate } from "../i18n";
import type { SessionAction, SessionState } from "../session";

export function TopBar({ state, dispatch }: { state: SessionState; dispatch: Dispatch<SessionAction> }) {
  return (
    <header className="top-bar">
      <div className="brand-lockup">
        <span className="brand-mark" aria-hidden="true">◇</span>
        <div><span className="eyebrow">{translate(state.language, "puzzleIdentity")}</span><h1>{translate(state.language, "product")}</h1></div>
      </div>
      <div className="top-actions">
        <label className="mode-select">
          <span>{translate(state.language, "solverMode")}</span>
          <select value={state.solverMode} onChange={(event) => dispatch({ type: "setSolverMode", mode: event.target.value as SessionState["solverMode"] })}>
            <option value="human">{translate(state.language, "human")}</option>
            <option value="smart">{translate(state.language, "smart")}</option>
            <option value="turbo">{translate(state.language, "turbo")}</option>
          </select>
        </label>
        <button type="button" className="text-button" onClick={() => dispatch({ type: "setLanguage", language: state.language === "en" ? "zh" : "en" })}>{translate(state.language, "switchLanguage")}</button>
        <div className="history-actions">
          <button type="button" aria-label={translate(state.language, "undo")} disabled={!state.past.length} onClick={() => dispatch({ type: "undo" })}>↶</button>
          <button type="button" aria-label={translate(state.language, "redo")} disabled={!state.future.length} onClick={() => dispatch({ type: "redo" })}>↷</button>
        </div>
        <button type="button" className="command-trigger" onClick={() => dispatch({ type: "setCommandOpen", open: true })}><span aria-hidden="true">⌘</span>{translate(state.language, "commands")}</button>
        <details className="settings-menu">
          <summary aria-label={translate(state.language, "settings")}>◐</summary>
          <div>
            <button type="button" onClick={() => dispatch({ type: "setTheme", theme: state.theme === "light" ? "dark" : "light" })}>{translate(state.language, "theme")}: {translate(state.language, state.theme)}</button>
            <label><input type="checkbox" checked={state.highContrast} onChange={(event) => dispatch({ type: "setHighContrast", enabled: event.target.checked })} />{translate(state.language, "highContrast")}</label>
          </div>
        </details>
      </div>
    </header>
  );
}
