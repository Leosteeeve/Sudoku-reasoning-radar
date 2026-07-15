import { useMemo, useState } from "react";
import { translate } from "../i18n";
import type { Language } from "../session";

interface CommandPanelProps {
  language: Language;
  onAnalyze: () => void;
  onClose: () => void;
}

export function CommandPanel({ language, onAnalyze, onClose }: CommandPanelProps) {
  const [query, setQuery] = useState("");
  const commands = useMemo(() => [
    { id: "analyze", label: translate(language, "analyzePuzzle"), description: translate(language, "analyzeDescription"), disabled: false, run: onAnalyze },
    { id: "generate", label: translate(language, "generatePuzzle"), description: translate(language, "generateUnavailable"), disabled: true },
    { id: "import", label: translate(language, "importPuzzle"), description: translate(language, "importUnavailable"), disabled: true },
    { id: "ocr", label: translate(language, "ocrPuzzle"), description: translate(language, "ocrUnavailable"), disabled: true },
    { id: "save", label: translate(language, "savePuzzle"), description: translate(language, "saveUnavailable"), disabled: true },
  ], [language, onAnalyze]);
  const filtered = commands.filter((command) => `${command.label} ${command.description}`.toLowerCase().includes(query.toLowerCase()));
  return (
    <div className="command-backdrop" onMouseDown={(event) => event.target === event.currentTarget && onClose()}>
      <section className="command-panel" role="dialog" aria-modal="true" aria-label={translate(language, "commandPanel")}>
        <header>
          <span className="eyebrow">⌘ /</span>
          <h2>{translate(language, "commandPanel")}</h2>
          <button type="button" className="icon-button" onClick={onClose} aria-label={translate(language, "close")}>×</button>
        </header>
        <input
          autoFocus
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
