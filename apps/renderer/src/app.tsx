import type { CoreClient, SolveStepV1 } from "@srr/core-client";
import { normalizePuzzle, type MigrationReport, type PuzzleRecord, type StorageService } from "@srr/storage";
import { useCallback, useEffect, useReducer, useRef, useState, type KeyboardEvent as ReactKeyboardEvent, type ReactNode, type RefObject } from "react";
import { Board } from "./components/Board";
import { CommandPanel } from "./components/CommandPanel";
import { Constellation, isAdvancedStep } from "./components/Constellation";
import { LessonCard } from "./components/LessonCard";
import { ImageReviewDialog } from "./components/ImageReviewDialog";
import { Timeline } from "./components/Timeline";
import { TopBar } from "./components/TopBar";
import { loadBrowserCoreClient } from "./core/wasm-transport";
import { systemLanguage, translate } from "./i18n";
import { actionForKey, createInitialSession, sessionReducer, type Language } from "./session";
import { browserFileActions, type BrowserFileActions } from "./web/files";
import { loadWebStorage, type WebStorageRuntime } from "./web/storage";
import "./styles.css";

interface WorkspaceProps {
  initialPuzzle: string;
  coreClient: CoreClient;
  initialLanguage?: Language;
  initialTrace?: SolveStepV1[];
  storageService?: Pick<StorageService, "loadCurrentSession" | "saveCurrentSession">;
  saveDelayMs?: number;
  fileActions?: BrowserFileActions;
  storageError?: string;
  migrationReport?: MigrationReport;
}

function sessionSignature(value: {
  puzzle: string;
  values: number[];
  noteMasks: number[];
  mode: string;
  trace: unknown[];
  currentStep: number;
}): string {
  return JSON.stringify(value);
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

type WorkspaceStorage = Pick<StorageService,
  "loadCurrentSession" | "saveCurrentSession" | "upsertPuzzle" | "listPuzzles" | "importPuzzles" | "importBackup" | "exportBackup"
>;

export function ReasoningWorkspace({
  initialPuzzle, coreClient, initialLanguage, initialTrace = [], storageService: storage,
  saveDelayMs = 300, fileActions = browserFileActions, storageError, migrationReport,
}: WorkspaceProps & { storageService?: WorkspaceStorage }) {
  const [state, dispatch] = useReducer(sessionReducer, undefined, () => ({
    ...createInitialSession(initialPuzzle, { language: initialLanguage ?? systemLanguage() }),
    trace: initialTrace,
  }));
  const commandTriggerRef = useRef<HTMLButtonElement>(null);
  const lessonTriggerRef = useRef<HTMLButtonElement>(null);
  const constellationTriggerRef = useRef<HTMLButtonElement>(null);
  const [announcement, setAnnouncement] = useState(() => migrationReport?.imported
    ? translate(initialLanguage ?? systemLanguage(), "legacyImported")
    : "");
  const [coreError, setCoreError] = useState(false);
  const [storageReady, setStorageReady] = useState(!storage);
  const lastPersistedRef = useRef<string | null>(null);
  const [operationError, setOperationError] = useState(storageError ?? "");
  const [offlineReady, setOfflineReady] = useState(false);
  const [library, setLibrary] = useState<PuzzleRecord[] | null>(null);
  const [rawImportOpen, setRawImportOpen] = useState(false);
  const [rawPuzzle, setRawPuzzle] = useState("");
  const [imageReviewOpen, setImageReviewOpen] = useState(false);
  const reducedMotion = useMediaQuery("(prefers-reduced-motion: reduce)");
  const narrowLayout = useMediaQuery("(max-width: 700px)");
  const step = state.trace[state.currentStep];
  const constellationOpen = state.constellationOpen;
  const persistenceValue = {
    puzzle: state.givens.join(""),
    values: state.values,
    noteMasks: state.candidates,
    mode: state.solverMode,
    trace: state.trace,
    currentStep: state.currentStep,
  };
  const persistenceKey = sessionSignature(persistenceValue);

  useEffect(() => {
    const ready = () => setOfflineReady(true);
    window.addEventListener("srr:offline-ready", ready);
    return () => window.removeEventListener("srr:offline-ready", ready);
  }, []);

  useEffect(() => {
    if (!storage) return;
    let active = true;
    setStorageReady(false);
    storage.loadCurrentSession().then((session) => {
      if (!active) return;
      if (session) {
        lastPersistedRef.current = sessionSignature(session);
        dispatch({
          type: "hydrate",
          puzzle: session.puzzle,
          values: session.values,
          candidates: session.noteMasks,
          solverMode: session.mode as typeof state.solverMode,
          trace: session.trace as SolveStepV1[],
          currentStep: session.currentStep,
        });
      } else {
        lastPersistedRef.current = sessionSignature(persistenceValue);
      }
      setStorageReady(true);
    }).catch(() => {
      if (active) { setStorageReady(true); setOperationError("load"); }
    });
    return () => { active = false; };
  }, [storage]);

  useEffect(() => {
    if (!storage || !storageReady || persistenceKey === lastPersistedRef.current) return;
    const timer = window.setTimeout(() => {
      storage.saveCurrentSession({
        id: "current",
        ...persistenceValue,
        elapsedMs: 0,
        savedAt: new Date().toISOString(),
      }).then(() => { lastPersistedRef.current = persistenceKey; }).catch(() => setOperationError("save"));
    }, saveDelayMs);
    return () => window.clearTimeout(timer);
  }, [persistenceKey, saveDelayMs, storageReady, storage]);

  const savePuzzle = useCallback(async () => {
    dispatch({ type: "setCommandOpen", open: false });
    if (!storage) { setOperationError("unavailable"); return; }
    try {
      await storage.upsertPuzzle({ puzzle: state.givens.join(""), source: "web", updatedAt: new Date().toISOString() });
    } catch { setOperationError("save-puzzle"); }
  }, [state.givens, storage]);

  const openLibrary = useCallback(async () => {
    dispatch({ type: "setCommandOpen", open: false });
    if (!storage) { setOperationError("unavailable"); return; }
    try { setLibrary(await storage.listPuzzles()); }
    catch { setOperationError("library"); }
  }, [storage]);

  const openRawImport = useCallback(() => {
    dispatch({ type: "setCommandOpen", open: false });
    setRawPuzzle("");
    setRawImportOpen(true);
  }, []);

  const applyRawPuzzle = useCallback(async () => {
    try {
      const puzzle = normalizePuzzle(rawPuzzle);
      dispatch({ type: "loadPuzzle", puzzle });
      setRawImportOpen(false);
      if (storage) await storage.importPuzzles([{ puzzle, source: "web-text-import", updatedAt: new Date().toISOString() }]);
    } catch { setOperationError("raw-import"); }
  }, [rawPuzzle, storage]);

  const importBackup = useCallback(async () => {
    dispatch({ type: "setCommandOpen", open: false });
    if (!storage) { setOperationError("unavailable"); return; }
    try {
      const text = await fileActions.pickText(".srr.json,application/json");
      if (text !== null) await storage.importBackup(text);
    } catch { setOperationError("backup-import"); }
  }, [fileActions, storage]);

  const exportBackup = useCallback(async () => {
    dispatch({ type: "setCommandOpen", open: false });
    if (!storage) { setOperationError("unavailable"); return; }
    try {
      const text = await storage.exportBackup("0.4.0-beta.1");
      fileActions.downloadText("sudoku-reasoning-radar.srr.json", text, "application/json");
    } catch { setOperationError("backup-export"); }
  }, [fileActions, storage]);

  const applyReviewedImage = useCallback((puzzle: string) => {
    dispatch({ type: "loadPuzzle", puzzle });
    setImageReviewOpen(false);
    if (storage) void storage.importPuzzles([{ puzzle, source: "web-image-review", updatedAt: new Date().toISOString() }])
      .catch(() => setOperationError("image-import"));
  }, [storage]);

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
      {state.commandOpen && <CommandPanel language={state.language} onAnalyze={analyze} onClose={closeCommand} onSave={savePuzzle} onLibrary={openLibrary} onImport={openRawImport} onBackupImport={importBackup} onBackupExport={exportBackup} onImageImport={() => { dispatch({ type: "setCommandOpen", open: false }); setImageReviewOpen(true); }} />}
      {library && (
        <div className="command-backdrop">
          <section className="command-panel library-panel" role="dialog" aria-modal="true" aria-labelledby="library-title">
            <header><h2 id="library-title">{translate(state.language, "puzzleLibrary")}</h2><button type="button" onClick={() => setLibrary(null)}>{translate(state.language, "close")}</button></header>
            {library.length === 0 ? <p>{translate(state.language, "emptyLibrary")}</p> : library.map((record) => (
              <button key={record.puzzle} type="button" onClick={() => { dispatch({ type: "loadPuzzle", puzzle: record.puzzle }); setLibrary(null); }}>
                {record.name || record.puzzle} {record.difficulty ? `鈥?${record.difficulty}` : ""}
              </button>
            ))}
          </section>
        </div>
      )}
      {rawImportOpen && (
        <div className="command-backdrop">
          <section className="command-panel" role="dialog" aria-modal="true" aria-labelledby="raw-import-title">
            <header><h2 id="raw-import-title">{translate(state.language, "importPuzzleText")}</h2><button type="button" onClick={() => setRawImportOpen(false)}>{translate(state.language, "close")}</button></header>
            <label>{translate(state.language, "puzzleText")}<textarea value={rawPuzzle} onChange={(event) => setRawPuzzle(event.target.value)} /></label>
            <button type="button" onClick={applyRawPuzzle}>{translate(state.language, "applyPuzzle")}</button>
          </section>
        </div>
      )}
      {imageReviewOpen && <ImageReviewDialog language={state.language} onClose={() => setImageReviewOpen(false)} onApply={applyReviewedImage} />}
      {coreError && <div className="core-toast" role="alert"><span>{translate(state.language, "responseFailed")}</span><button type="button" onClick={analyze}>{translate(state.language, "retry")}</button></div>}
      {(storageError || operationError) && <div className="storage-toast" role="alert">{translate(state.language, storageError || operationError === "unavailable" ? "storageUnavailable" : "storageOperationFailed")}</div>}
      {offlineReady && <output className="offline-status" aria-live="polite">{translate(state.language, "offlineReady")}</output>}
      <div className="sr-only" role="status" aria-live="polite" aria-atomic="true">{announcement}</div>
    </div>
  );
}

interface CoreShellProps {
  loadClient?: () => Promise<CoreClient>;
  loadStorage?: () => Promise<WebStorageRuntime>;
  initialPuzzle: string;
}

export function CoreShell({ loadClient = loadBrowserCoreClient, loadStorage = loadWebStorage, initialPuzzle }: CoreShellProps) {
  const [client, setClient] = useState<CoreClient | null>(null);
  const [storageRuntime, setStorageRuntime] = useState<WebStorageRuntime | null | undefined>(undefined);
  const [failed, setFailed] = useState(false);
  const [attempt, setAttempt] = useState(0);
  useEffect(() => {
    let active = true;
    setFailed(false);
    loadClient().then((loaded) => active && setClient(loaded)).catch(() => active && setFailed(true));
    return () => { active = false; };
  }, [attempt, loadClient]);
  useEffect(() => {
    let active = true;
    loadStorage().then((loaded) => {
      if (active) setStorageRuntime(loaded);
      else loaded.service.close();
    }).catch(() => active && setStorageRuntime(null));
    return () => { active = false; };
  }, [loadStorage]);
  if (client && storageRuntime !== undefined) return <ReasoningWorkspace
    initialPuzzle={initialPuzzle}
    coreClient={client}
    storageService={storageRuntime?.service}
    migrationReport={storageRuntime?.migrationReport}
    storageError={storageRuntime === null ? "unavailable" : undefined}
  />;
  if (failed) return <main className="launch-state"><div role="alert"><h1>{translate(systemLanguage(), "coreFailed")}</h1><button type="button" onClick={() => setAttempt((value) => value + 1)}>{translate(systemLanguage(), "retry")}</button></div></main>;
  return <main className="launch-state" aria-busy="true"><p>{translate(systemLanguage(), "loadingCore")}</p></main>;
}
