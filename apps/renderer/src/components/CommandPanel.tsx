import { useEffect, useMemo, useRef, useState, type KeyboardEvent } from "react";
import { translate } from "../i18n";
import type { Language } from "../session";

interface CommandPanelProps {
  language: Language;
  onAnalyze: () => void;
  onClose: () => void;
  onSave?: () => void;
  onLibrary?: () => void;
  onImport?: () => void;
  onBackupImport?: () => void;
  onBackupExport?: () => void;
  onImageImport?: () => void;
}

export function CommandPanel({ language, onAnalyze, onClose, onSave, onLibrary, onImport, onBackupImport, onBackupExport, onImageImport }: CommandPanelProps) {
  const [query, setQuery] = useState("");
  const panelRef = useRef<HTMLElement>(null);
  const searchRef = useRef<HTMLInputElement>(null);
  const commands = useMemo(() => [
    { id: "analyze", label: translate(language, "analyzePuzzle"), description: translate(language, "analyzeDescription"), disabled: false, run: onAnalyze },
    { id: "generate", label: translate(language, "generatePuzzle"), description: translate(language, "generateUnavailable"), disabled: true },
    { id: "import", label: translate(language, "importPuzzle"), description: translate(language, "importDescription"), disabled: !onImport, run: onImport },
    { id: "ocr", label: translate(language, "ocrPuzzle"), description: translate(language, "ocrUnavailable"), disabled: true },
    { id: "save", label: translate(language, "savePuzzle"), description: translate(language, "saveDescription"), disabled: !onSave, run: onSave },
    { id: "library", label: translate(language, "openLibrary"), description: translate(language, "libraryDescription"), disabled: !onLibrary, run: onLibrary },
    { id: "backup-import", label: translate(language, "importBackup"), description: translate(language, "importBackupDescription"), disabled: !onBackupImport, run: onBackupImport },
    { id: "backup-export", label: translate(language, "exportBackup"), description: translate(language, "exportBackupDescription"), disabled: !onBackupExport, run: onBackupExport },
    { id: "image-import", label: translate(language, "importImage"), description: translate(language, "imageDescription"), disabled: !onImageImport, run: onImageImport },
  ], [language, onAnalyze, onBackupExport, onBackupImport, onImageImport, onImport, onLibrary, onSave]);
  const filtered = commands.filter((command) => `${command.label} ${command.description}`.toLowerCase().includes(query.toLowerCase()));
  useEffect(() => searchRef.current?.focus(), []);

  const keepFocusInside = (event: KeyboardEvent<HTMLElement>) => {
    if (event.key === "Escape") {
      event.preventDefault();
      event.stopPropagation();
      onClose();
      return;
    }
    if (event.key !== "Tab" || !panelRef.current) return;
    const focusable = [...panelRef.current.querySelectorAll<HTMLElement>(
      "button:not([disabled]), input:not([disabled]), select:not([disabled]), textarea:not([disabled]), [tabindex]:not([tabindex='-1'])",
    )];
    if (focusable.length === 0) return;
    const first = focusable[0];
    const last = focusable.at(-1)!;
    if (event.shiftKey && (document.activeElement === first || !panelRef.current.contains(document.activeElement))) {
      event.preventDefault();
      last.focus();
    } else if (!event.shiftKey && (document.activeElement === last || !panelRef.current.contains(document.activeElement))) {
      event.preventDefault();
      first.focus();
    }
  };
  return (
    <div className="command-backdrop" onMouseDown={(event) => event.target === event.currentTarget && onClose()}>
      <section ref={panelRef} className="command-panel" role="dialog" aria-modal="true" aria-label={translate(language, "commandPanel")} onKeyDown={keepFocusInside}>
        <header>
          <span className="eyebrow">⌘ /</span>
          <h2>{translate(language, "commandPanel")}</h2>
          <button type="button" className="icon-button" onClick={onClose} aria-label={translate(language, "close")}>×</button>
        </header>
        <input
          ref={searchRef}
          type="search"
          value={query}
          onChange={(event) => setQuery(event.target.value)}
          placeholder={translate(language, "searchCommands")}
          aria-label={translate(language, "searchCommands")}
        />
        <div className="command-list">
          {filtered.map((command) => (
            <div className={command.disabled ? "command-item is-disabled" : "command-item"} key={command.id}>
              <button type="button" disabled={command.disabled} onClick={command.run}>
                <span>{command.label}</span><span aria-hidden="true">↗</span>
              </button>
              <p>{command.description}</p>
            </div>
          ))}
          {filtered.length === 0 && <p className="empty-commands">{translate(language, "noCommands")}</p>}
        </div>
      </section>
    </div>
  );
}
