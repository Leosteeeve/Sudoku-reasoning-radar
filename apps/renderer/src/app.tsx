import type { CoreClient, SolveStepV1 } from "@srr/core-client";
import { useCallback, useEffect, useReducer, useState } from "react";
import { Board } from "./components/Board";
import { CommandPanel } from "./components/CommandPanel";
import { Constellation, isAdvancedStep } from "./components/Constellation";
import { LessonCard } from "./components/LessonCard";
import { Timeline } from "./components/Timeline";
import { TopBar } from "./components/TopBar";
import { loadBrowserCoreClient } from "./core/wasm-transport";
import { systemLanguage, translate } from "./i18n";
import { actionForKey, createInitialSession, sessionReducer, type Language } from "./session";
import "./styles.css";

interface WorkspaceProps {
  initialPuzzle: string;
  coreClient: CoreClient;
  initialLanguage?: Language;
  initialTrace?: SolveStepV1[];
}

export function ReasoningWorkspace({ initialPuzzle, coreClient, initialLanguage, initialTrace = [] }: WorkspaceProps) {
  const [state, dispatch] = useReducer(sessionReducer, undefined, () => ({
    ...createInitialSession(initialPuzzle, { language: initialLanguage ?? systemLanguage() }),
    trace: initialTrace,
    constellationOpen: isAdvancedStep(initialTrace[0]),
  }));
  const [announcement, setAnnouncement] = useState("");
  const [coreError, setCoreError] = useState(false);
  const reducedMotion = globalThis.matchMedia?.("(prefers-reduced-motion: reduce)").matches ?? false;
  const step = state.trace[state.currentStep];
  const constellationOpen = state.constellationOpen || isAdvancedStep(step);

  const analyze = useCallback(async () => {
    dispatch({ type: "setCommandOpen", open: false });
    setCoreError(false);
    try {
      const response = await coreClient.solve({ puzzle: state.values.join(""), mode: state.solverMode, includeTrace: true });
      dispatch({ type: "loadTrace", steps: response.steps });
      setAnnouncement(translate(state.language, "traceReady", { count: response.steps.length }));
    } catch {
      setCoreError(true);
      setAnnouncement(translate(state.language, "responseFailed"));
    }
  }, [coreClient, state.language, state.solverMode, state.values]);

  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      const target = event.target as HTMLElement | null;
      if (target?.matches("input, select, textarea") && event.key !== "Escape") return;
      const action = actionForKey(event.key, event);
      if (!action) return;
      event.preventDefault();
      dispatch(action);
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, []);

  useEffect(() => {
    if (!state.playing || state.trace.length < 2) return;
    const timer = window.setInterval(() => {
      if (state.currentStep >= state.trace.length - 1) dispatch({ type: "togglePlay" });
      else dispatch({ type: "setStep", step: state.currentStep + 1 });
    }, 1200 / state.speed);
    return () => window.clearInterval(timer);
  }, [state.currentStep, state.playing, state.speed, state.trace.length]);

  return (
    <div className={`app-shell theme-${state.theme}${state.highContrast ? " high-contrast" : ""}${reducedMotion ? " reduced-motion" : ""}`} lang={state.language === "zh" ? "zh-CN" : "en"}>
      <TopBar state={state} dispatch={dispatch} />
      <main className="workspace-layout">
        <Board state={state} dispatch={dispatch} />
        <aside className={`lesson-rail${state.lessonOpen ? " is-open" : ""}`}>
          <LessonCard step={step} language={state.language} />
          <section className={`constellation-panel${constellationOpen ? " is-open" : ""}`}>
            <button type="button" className="constellation-toggle" onClick={() => dispatch({ type: "setConstellationOpen", open: !constellationOpen })}>
              {translate(state.language, constellationOpen ? "collapseConstellation" : "expandConstellation")}
            </button>
            {constellationOpen && <Constellation step={step} language={state.language} />}
          </section>
        </aside>
        <Timeline state={state} dispatch={dispatch} />
      </main>
      {state.commandOpen && <CommandPanel language={state.language} onAnalyze={analyze} onClose={() => dispatch({ type: "setCommandOpen", open: false })} />}
      {coreError && <div className="core-toast" role="alert"><span>{translate(state.language, "responseFailed")}</span><button type="button" onClick={analyze}>{translate(state.language, "retry")}</button></div>}
      <div className="sr-only" role="status" aria-live="polite" aria-atomic="true">{announcement}</div>
    </div>
  );
}

interface CoreShellProps {
  loadClient?: () => Promise<CoreClient>;
  initialPuzzle: string;
}

export function CoreShell({ loadClient = loadBrowserCoreClient, initialPuzzle }: CoreShellProps) {
  const [client, setClient] = useState<CoreClient | null>(null);
  const [failed, setFailed] = useState(false);
  const [attempt, setAttempt] = useState(0);
  useEffect(() => {
    let active = true;
    setFailed(false);
    loadClient().then((loaded) => active && setClient(loaded)).catch(() => active && setFailed(true));
    return () => { active = false; };
  }, [attempt, loadClient]);
  if (client) return <ReasoningWorkspace initialPuzzle={initialPuzzle} coreClient={client} />;
  if (failed) return <main className="launch-state"><div role="alert"><h1>{translate(systemLanguage(), "coreFailed")}</h1><button type="button" onClick={() => setAttempt((value) => value + 1)}>{translate(systemLanguage(), "retry")}</button></div></main>;
  return <main className="launch-state" aria-busy="true"><p>{translate(systemLanguage(), "loadingCore")}</p></main>;
}
