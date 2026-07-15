import { useEffect, useRef, useState } from "react";
import { translate } from "../i18n";
import type { Language } from "../session";

export const MAX_IMAGE_BYTES = 8 * 1024 * 1024;
const supportedTypes = new Set(["image/png", "image/jpeg", "image/webp"]);

export interface ImageImportAdapter {
  extract?(file: File): Promise<number[]>;
  select?(): Promise<number[] | null>;
}

export const emptyImageImportAdapter: ImageImportAdapter = {
  async extract() { return Array(81).fill(0); },
};

export function validateImageFile(file: File): void {
  if (!supportedTypes.has(file.type)) throw new Error("Unsupported image type. Choose PNG, JPEG, or WebP.");
  if (file.size > MAX_IMAGE_BYTES) throw new Error("Image size must be 8 MB or smaller.");
}

export function findSudokuConflicts(values: number[]): Set<number> {
  const conflicts = new Set<number>();
  const units: number[][] = [];
  for (let row = 0; row < 9; row += 1) units.push(Array.from({ length: 9 }, (_, column) => row * 9 + column));
  for (let column = 0; column < 9; column += 1) units.push(Array.from({ length: 9 }, (_, row) => row * 9 + column));
  for (let boxRow = 0; boxRow < 3; boxRow += 1) {
    for (let boxColumn = 0; boxColumn < 3; boxColumn += 1) {
      units.push(Array.from({ length: 9 }, (_, offset) =>
        (boxRow * 3 + Math.floor(offset / 3)) * 9 + boxColumn * 3 + (offset % 3)));
    }
  }
  for (const unit of units) {
    const seen = new Map<number, number[]>();
    for (const index of unit) {
      const digit = values[index] ?? 0;
      if (digit === 0) continue;
      seen.set(digit, [...(seen.get(digit) ?? []), index]);
    }
    for (const indexes of seen.values()) if (indexes.length > 1) indexes.forEach((index) => conflicts.add(index));
  }
  return conflicts;
}

export function ImageReviewDialog({ adapter = emptyImageImportAdapter, onApply, onClose, language = "en" }: {
  adapter?: ImageImportAdapter;
  onApply: (puzzle: string) => void;
  onClose: () => void;
  language?: Language;
}) {
  const [values, setValues] = useState<number[]>(() => Array(81).fill(0));
  const [preview, setPreview] = useState("");
  const [error, setError] = useState("");
  const previewRef = useRef("");
  const conflicts = findSudokuConflicts(values);

  const releasePreview = () => {
    if (previewRef.current) URL.revokeObjectURL(previewRef.current);
    previewRef.current = "";
  };

  useEffect(() => releasePreview, []);

  const chooseImage = async (file: File | undefined) => {
    if (!file) return;
    try {
      validateImageFile(file);
      releasePreview();
      const url = URL.createObjectURL(file);
      previewRef.current = url;
      setPreview(url);
      if (!adapter.extract) throw new Error("File image import is unavailable.");
      applyExtracted(await adapter.extract(file));
      setError("");
    } catch (reason) {
      setError(reason instanceof Error ? reason.message : String(reason));
    }
  };

  const applyExtracted = (extracted: number[]) => {
    if (extracted.length !== 81 || extracted.some((digit) => !Number.isInteger(digit) || digit < 0 || digit > 9)) {
      throw new Error("Image adapter must return 81 digits from 0 to 9.");
    }
    setValues([...extracted]);
  };

  const selectDesktopImage = async () => {
    if (!adapter.select) return;
    try {
      const extracted = await adapter.select();
      if (extracted) applyExtracted(extracted);
      setError("");
    } catch (reason) {
      setError(reason instanceof Error ? reason.message : String(reason));
    }
  };

  const updateCell = (index: number, text: string) => {
    if (!/^[1-9]?$/.test(text)) return;
    setValues((current) => current.map((digit, cell) => cell === index ? Number(text || 0) : digit));
  };

  return (
    <div className="command-backdrop">
      <section className="command-panel image-review" role="dialog" aria-modal="true" aria-labelledby="image-review-title">
        <header><h2 id="image-review-title">{translate(language, "imageReviewTitle")}</h2><button type="button" onClick={onClose}>{translate(language, "close")}</button></header>
        <p>{translate(language, "imageDisclaimer")}</p>
        {adapter.select
          ? <button type="button" onClick={() => void selectDesktopImage()}>{translate(language, "selectImage")}</button>
          : <label>{translate(language, "selectImage")}<input type="file" accept="image/png,image/jpeg,image/webp" onChange={(event) => void chooseImage(event.target.files?.[0])} /></label>}
        {preview && <img className="image-preview" src={preview} alt={translate(language, "imagePreview")} />}
        <div className="image-review-grid" aria-label="Editable image review cells">
          {values.map((digit, index) => (
            <input
              key={index}
              aria-label={`Image row ${Math.floor(index / 9) + 1}, column ${(index % 9) + 1}`}
              aria-invalid={conflicts.has(index)}
              inputMode="numeric"
              maxLength={1}
              value={digit || ""}
              onChange={(event) => updateCell(index, event.target.value)}
            />
          ))}
        </div>
        {(error || conflicts.size > 0) && <p role="alert">{error || translate(language, "imageConflict")}</p>}
        <button type="button" disabled={conflicts.size > 0} onClick={() => onApply(values.join(""))}>{translate(language, "applyReviewedPuzzle")}</button>
      </section>
    </div>
  );
}
