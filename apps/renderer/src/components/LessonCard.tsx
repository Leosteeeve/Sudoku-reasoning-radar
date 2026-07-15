import type { SolveStepV1 } from "@srr/core-client";
import { techniqueNames, translate } from "../i18n";
import type { Language } from "../session";

export function LessonCard({ step, language }: { step?: SolveStepV1; language: Language }) {
  const digits = step?.candidateDeltas.flatMap((delta) => delta.removedDigits).join(", ") || "—";
  return (
    <article className="lesson-card" data-testid="lesson-card" aria-label={translate(language, "lesson")}>
      <header>
        <span className="eyebrow">{translate(language, "lesson")}</span>
        {step?.technique ? (
          <h2 className="technique-title">
            <span lang="en">{techniqueNames[step.technique].en}</span>
            <span className="technique-divider" aria-hidden="true">／</span>
            <span lang="zh-CN">{techniqueNames[step.technique].zh}</span>
          </h2>
        ) : <h2>{translate(language, "noStep")}</h2>}
      </header>
      <ol className="lesson-sequence">
        <li>
          <span className="lesson-number">01</span>
          <div><h3>{translate(language, "observe")}</h3><p>{translate(language, "observeBody")}</p></div>
        </li>
        <li>
          <span className="lesson-number">02</span>
          <div><h3>{translate(language, "eliminate")}</h3><p>{translate(language, "eliminateBody", { digits })}</p></div>
        </li>
        <li>
          <span className="lesson-number">03</span>
          <div><h3>{translate(language, "conclude")}</h3><p>{translate(language, "concludeBody", { action: step?.action ?? "step" })}</p></div>
        </li>
      </ol>
    </article>
  );
}
