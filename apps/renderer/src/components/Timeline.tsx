import type { Dispatch } from "react";
import { translate } from "../i18n";
import type { SessionAction, SessionState } from "../session";

export function Timeline({ state, dispatch }: { state: SessionState; dispatch: Dispatch<SessionAction> }) {
  const max = Math.max(0, state.trace.length - 1);
  const previousDisabled = state.trace.length === 0 || state.currentStep === 0;
  const nextDisabled = state.trace.length === 0 || state.currentStep >= max;
  const playDisabled = state.trace.length < 2 || (!state.playing && state.currentStep >= max);
  return (
    <section className="timeline-dock" aria-label={translate(state.language, "timeline")}>
      <div className="timeline-meta">
        <span className="eyebrow">{translate(state.language, "timeline")}</span>
        <strong>{state.trace.length ? `${state.currentStep + 1} / ${state.trace.length}` : "0 / 0"}</strong>
      </div>
      <div className="timeline-controls">
        <button type="button" disabled={previousDisabled} onClick={() => dispatch({ type: "setStep", step: state.currentStep - 1 })} aria-label={translate(state.language, "previous")}>←</button>
        <button type="button" disabled={playDisabled} className="play-button" onClick={() => dispatch({ type: "togglePlay" })}>{state.playing ? translate(state.language, "pause") : translate(state.language, "play")}</button>
        <button type="button" disabled={nextDisabled} onClick={() => dispatch({ type: "setStep", step: state.currentStep + 1 })} aria-label={translate(state.language, "next")}>→</button>
      </div>
      <input
        className="timeline-range"
        type="range"
        min="0"
        max={max}
        value={Math.min(state.currentStep, max)}
        disabled={state.trace.length === 0}
        onChange={(event) => dispatch({ type: "setStep", step: Number(event.target.value) })}
        aria-label={translate(state.language, "timeline")}
      />
      <label className="speed-control">
        <span>{translate(state.language, "speed")}</span>
        <select disabled={state.trace.length === 0} value={state.speed} onChange={(event) => dispatch({ type: "setSpeed", speed: Number(event.target.value) })}>
          <option value="0.5">0.5×</option><option value="1">1×</option><option value="2">2×</option>
        </select>
      </label>
    </section>
  );
}
