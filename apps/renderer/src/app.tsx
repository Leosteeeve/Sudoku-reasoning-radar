import type { CoreClient, SolveStepV1 } from "@srr/core-client";
import { useCallback, useEffect, useReducer, useRef, useState, type KeyboardEvent as ReactKeyboardEvent, type ReactNode, type RefObject } from "react";
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

function useMediaQuery(query: string): boolean {
  const [matches, setMatches] = useState(() => globalThis.matchMedia?.(query).matches ?? false);
  useEffect(() => {
    const media = globalThis.matchMedia?.(query);
    if (!media) return;
    const update = () => setMatches(media.matches);
    update();
    media.addEventListener("change", update);
    return () => media.removeEventListener("change", update);
  }, [query]);
  return matches;
}

function isInteractiveTarget(target: EventTarget | null): boolean {
  if (!(target instanceof Element)) return false;
  let element: Element | null = target;
  while (element) {
    if (element.matches(
      "button, input, select, textarea, a[href], summary, [role='button'], [role='link'], [role='textbox'], [role='combobox']",
    )) return true;
    const editable = (element as HTMLElement).contentEditable;
    if ((editable && editable !== "false" && editable !== "inherit")
        || (element.hasAttribute("contenteditable") && element.getAttribute("contenteditable") !== "false")) {
      return true;
    }
    element = element.parentElement;
  }
  return false;
}

const focusableSelector = [
  "button:not([disabled])", "a[href]", "input:not([disabled])", "select:not([disabled])",
  "textarea:not([disabled])", "[tabindex]:not([tabindex='-1'])",
].join(",");

function MobileSheet({ labelId, returnFocusRef, onClose, children }: {
  labelId: string;
  returnFocusRef: RefObject<HTMLButtonElement | null>;
  onClose: () => void;
  children: ReactNode;
}) {
  const dialogRef = useRef<HTMLElement>(null);

  useEffect(() => {
    const dialog = dialogRef.current;
    const initial = dialog?.querySelector<HTMLElement>(focusableSelector) ?? dialog;
    initial?.focus();
    return () => queueMicrotask(() => returnFocusRef.current?.focus());
  }, [returnFocusRef]);

  const onKeyDown = (event: ReactKeyboardEvent<HTMLElement>) => {
    if (event.key === "Escape") {
      event.preventDefault();
      event.stopPropagation();
      onClose();
      return;
    }
    if (event.key !== "Tab") return;
    const focusable = [...(dialogRef.current?.querySelectorAll<HTMLElement>(focusableSelector) ?? [])];
    if (!focusable.length) {
      event.preventDefault();
      dialogRef.current?.focus();
      return;
    }
    const first = focusable[0];
    const last = focusable[focusable.length - 1];
    if (event.shiftKey && (document.activeElement === first || !dialogRef.current?.contains(document.activeElement))) {
      event.preventDefault();
      last.focus();
    } else if (!event.shiftKey && (document.activeElement === last || !dialogRef.current?.contains(document.activeElement))) {
      event.preventDefault();
      first.focus();
    }
  };

  return (
    <div className="mobile-sheet-backdrop" onMouseDown={(event) => event.target === event.currentTarget && onClose()}>
      <section ref={dialogRef} className="mobile-sheet" role="dialog" aria-modal="true" aria-labelledby={labelId} tabIndex={-1} onKeyDown={onKeyDown}>
        {children}
      </section>
    </div>
  );
}

export function ReasoningWorkspace({ initialPuzzle, coreClient, initialLanguage, initialTrace = [] }: WorkspaceProps) {
  const [state, dispatch] = useReducer(sessionReducer, undefined, () => ({
    ...createInitialSession(initialPuzzle, { language: initialLanguage ?? systemLanguage() }),
    trace: initialTrace,
  }));
  const commandTriggerRef = useRef<HTMLButtonElement>(null);
  const lessonTriggerRef = useRef<HTMLButtonElement>(null);
  const constellationTriggerRef = useRef<HTMLButtonElement>(null);
  const [announcement, setAnnouncement] = useState("");
  const [coreError, setCoreError] = useState(false);
  const reducedMotion = useMediaQuery("(prefers-reduced-motion: reduce)");
  const narrowLayout = useMediaQuery("(max-width: 700px)");
  const step = state.trace[state.currentStep];
  const constellationOpen = state.constellationOpen;

  const closeCommand = useCallback(() => {
    dispatch({ type: "setCommandOpen", open: false });
    queueMicrotask(() => commandTriggerRef.current?.focus());
  }, []);

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
    if (step && isAdvancedStep(step)) {
      dispatch({ type: "showAdvancedConstellation", stepId: step.id });
    }
  }, [step]);

  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      if (isInteractiveTarget(event.target)) return;
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
      <TopBar state={state} dispatch={dispatch} commandTriggerRef={commandTriggerRef} />
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
        {narrowLayout && (
          <nav className="mobile-sheet-actions" aria-label={translate(state.language, "lesson")}>
            <button ref={lessonTriggerRef} type="button" onClick={() => {
              dispatch({ type: "setConstellationOpen", open: false });
              dispatch({ type: "setLessonOpen", open: true });
            }}>
              {translate(state.language, "openLessonSheet")}
            </button>
            <button ref={constellationTriggerRef} type="button" onClick={() => {
              dispatch({ type: "setLessonOpen", open: false });
              dispatch({ type: "setConstellationOpen", open: true });
            }}>
              {translate(state.language, "openConstellationSheet")}
            </button>
          </nav>
        )}
        <Timeline state={state} dispatch={dispatch} />
      </main>
      {narrowLayout && state.lessonOpen && (
        <MobileSheet labelId="lesson-sheet-title" returnFocusRef={lessonTriggerRef} onClose={() => dispatch({ type: "setLessonOpen", open: false })}>
          <header><h2 id="lesson-sheet-title">{translate(state.language, "lesson")}</h2><button type="button" onClick={() => dispatch({ type: "setLessonOpen", open: false })}>{translate(state.language, "closeLessonSheet")}</button></header>
          <LessonCard step={step} language={state.language} />
        </MobileSheet>
      )}
      {narrowLayout && constellationOpen && (
        <MobileSheet labelId="constellation-sheet-title" returnFocusRef={constellationTriggerRef} onClose={() => dispatch({ type: "setConstellationOpen", open: false })}>
          <header><h2 id="constellation-sheet-title">{translate(state.language, "constellation")}</h2><button type="button" onClick={() => dispatch({ type: "setConstellationOpen", open: false })}>{translate(state.language, "closeConstellationSheet")}</button></header>
          <Constellation step={step} language={state.language} />
        </MobileSheet>
      )}
      {state.commandOpen && <CommandPanel language={state.language} onAnalyze={analyze} onClose={closeCommand} />}
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
