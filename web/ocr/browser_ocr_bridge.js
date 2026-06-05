(function () {
    "use strict";

    const normSize = 28;
    let panel = null;
    let objectUrl = "";
    let selectedImage = null;
    let selectedFileName = "";
    let templateCache = null;

    function byId(id) {
        return panel ? panel.querySelector(`#${id}`) : null;
    }

    function focusCanvas() {
        const canvas = document.getElementById("canvas");
        if (canvas) {
            canvas.focus({ preventScroll: true });
        }
    }

    function closePanel() {
        if (objectUrl) {
            URL.revokeObjectURL(objectUrl);
            objectUrl = "";
        }
        selectedImage = null;
        selectedFileName = "";
        if (panel) {
            panel.remove();
            panel = null;
        }
        focusCanvas();
    }

    function cleanedValue() {
        const input = byId("srrOcrPuzzleText");
        const raw = input ? input.value : "";
        return window.SRRBrowserOCR.normalizePuzzleString(raw);
    }

    function updateCounter(message, ok) {
        const counter = byId("srrOcrCounter");
        const status = byId("srrOcrStatus");
        const puzzle = cleanedValue();
        if (counter) {
            counter.textContent = `${puzzle.length}/81 cleaned cells`;
            counter.classList.toggle("ready", puzzle.length === 81);
        }
        if (status && message) {
            status.textContent = message;
            status.classList.toggle("error", ok === false);
            status.classList.toggle("success", ok === true);
        }
    }

    function clamp(value, lo, hi) {
        return Math.max(lo, Math.min(hi, value));
    }

    function grayAt(data, index) {
        return data[index] * 0.299 + data[index + 1] * 0.587 + data[index + 2] * 0.114;
    }

    function isLinePixel(data, index) {
        const r = data[index];
        const g = data[index + 1];
        const b = data[index + 2];
        const gray = r * 0.299 + g * 0.587 + b * 0.114;
        const spread = Math.max(r, g, b) - Math.min(r, g, b);
        return gray < 218 || (spread > 28 && gray < 246);
    }

    function isDigitInkPixel(data, index) {
        const r = data[index];
        const g = data[index + 1];
        const b = data[index + 2];
        const gray = r * 0.299 + g * 0.587 + b * 0.114;
        const spread = Math.max(r, g, b) - Math.min(r, g, b);
        return gray < 172 && (spread < 62 || gray < 118);
    }

    function makeCanvas(width, height) {
        const canvas = document.createElement("canvas");
        canvas.width = Math.max(1, Math.round(width));
        canvas.height = Math.max(1, Math.round(height));
        return canvas;
    }

    function drawImageToCanvas(image) {
        const maxSide = 1200;
        const scale = Math.min(1, maxSide / Math.max(image.naturalWidth || image.width, image.naturalHeight || image.height));
        const width = Math.max(1, Math.round((image.naturalWidth || image.width) * scale));
        const height = Math.max(1, Math.round((image.naturalHeight || image.height) * scale));
        const canvas = makeCanvas(width, height);
        const ctx = canvas.getContext("2d", { willReadFrequently: true });
        ctx.drawImage(image, 0, 0, width, height);
        return canvas;
    }

    function groupsFromProjection(counts, threshold) {
        const groups = [];
        let start = -1;
        let sum = 0;
        let weighted = 0;
        let peak = 0;
        for (let i = 0; i < counts.length; ++i) {
            if (counts[i] >= threshold) {
                if (start < 0) {
                    start = i;
                    sum = 0;
                    weighted = 0;
                    peak = 0;
                }
                sum += counts[i];
                weighted += counts[i] * i;
                peak = Math.max(peak, counts[i]);
            } else if (start >= 0) {
                groups.push({ pos: weighted / Math.max(1, sum), start, end: i - 1, peak });
                start = -1;
            }
        }
        if (start >= 0) {
            groups.push({ pos: weighted / Math.max(1, sum), start, end: counts.length - 1, peak });
        }
        return groups;
    }

    function nearestDistance(value, positions) {
        let best = Infinity;
        for (const position of positions) {
            best = Math.min(best, Math.abs(value - position));
        }
        return best;
    }

    function selectUniformGridLines(groups, imageLength) {
        const peaks = groups
            .map((group) => group.pos)
            .filter((pos) => Number.isFinite(pos))
            .sort((a, b) => a - b);
        if (peaks.length < 2) {
            return null;
        }

        let best = null;
        for (let i = 0; i < peaks.length - 1; ++i) {
            for (let j = i + 1; j < peaks.length; ++j) {
                const span = peaks[j] - peaks[i];
                if (span < imageLength * 0.42) {
                    continue;
                }
                const step = span / 9;
                if (step < 12) {
                    continue;
                }
                let score = 0;
                const lines = [];
                for (let k = 0; k <= 9; ++k) {
                    const expected = peaks[i] + step * k;
                    lines.push(expected);
                    const distance = nearestDistance(expected, peaks);
                    score += Math.min(distance, step * 0.7) / step;
                }
                score += Math.abs((span / imageLength) - 0.78) * 0.2;
                if (!best || score < best.score) {
                    best = { score, lines };
                }
            }
        }
        return best && best.score < 4.2 ? best.lines : null;
    }

    function fallbackGridBounds(data, width, height) {
        let minX = width;
        let minY = height;
        let maxX = 0;
        let maxY = 0;
        for (let y = 0; y < height; ++y) {
            for (let x = 0; x < width; ++x) {
                const index = (y * width + x) * 4;
                if (isLinePixel(data, index)) {
                    minX = Math.min(minX, x);
                    minY = Math.min(minY, y);
                    maxX = Math.max(maxX, x);
                    maxY = Math.max(maxY, y);
                }
            }
        }
        if (maxX <= minX || maxY <= minY) {
            const side = Math.min(width, height);
            minX = (width - side) / 2;
            minY = (height - side) / 2;
            maxX = minX + side;
            maxY = minY + side;
        }
        return {
            x: minX,
            y: minY,
            width: maxX - minX,
            height: maxY - minY
        };
    }

    function findGrid(data, width, height) {
        const rowCounts = new Array(height).fill(0);
        const colCounts = new Array(width).fill(0);
        for (let y = 0; y < height; ++y) {
            for (let x = 0; x < width; ++x) {
                const index = (y * width + x) * 4;
                if (isLinePixel(data, index)) {
                    rowCounts[y] += 1;
                    colCounts[x] += 1;
                }
            }
        }

        const rowGroups = groupsFromProjection(rowCounts, width * 0.24);
        const colGroups = groupsFromProjection(colCounts, height * 0.24);
        const yLines = selectUniformGridLines(rowGroups, height);
        const xLines = selectUniformGridLines(colGroups, width);
        if (xLines && yLines) {
            return { xLines, yLines };
        }

        const bounds = fallbackGridBounds(data, width, height);
        const xFallback = [];
        const yFallback = [];
        for (let i = 0; i <= 9; ++i) {
            xFallback.push(bounds.x + bounds.width * i / 9);
            yFallback.push(bounds.y + bounds.height * i / 9);
        }
        return { xLines: xFallback, yLines: yFallback };
    }

    function normalizeMask(width, height, predicate) {
        let minX = width;
        let minY = height;
        let maxX = -1;
        let maxY = -1;
        let inkCount = 0;
        for (let y = 0; y < height; ++y) {
            for (let x = 0; x < width; ++x) {
                if (predicate(x, y)) {
                    ++inkCount;
                    minX = Math.min(minX, x);
                    minY = Math.min(minY, y);
                    maxX = Math.max(maxX, x);
                    maxY = Math.max(maxY, y);
                }
            }
        }
        if (inkCount === 0 || maxX < minX || maxY < minY) {
            return null;
        }

        const boxW = maxX - minX + 1;
        const boxH = maxY - minY + 1;
        const square = Math.max(boxW, boxH) * 1.18;
        const cx = (minX + maxX) / 2;
        const cy = (minY + maxY) / 2;
        const startX = cx - square / 2;
        const startY = cy - square / 2;
        const mask = new Uint8Array(normSize * normSize);
        let normalizedInk = 0;
        for (let y = 0; y < normSize; ++y) {
            for (let x = 0; x < normSize; ++x) {
                const sx = Math.round(startX + (x + 0.5) * square / normSize);
                const sy = Math.round(startY + (y + 0.5) * square / normSize);
                const ink = sx >= 0 && sx < width && sy >= 0 && sy < height && predicate(sx, sy);
                if (ink) {
                    mask[y * normSize + x] = 1;
                    ++normalizedInk;
                }
            }
        }
        return {
            mask,
            ink: normalizedInk,
            ratio: normalizedInk / mask.length,
            aspect: boxW / Math.max(1, boxH),
            box: { minX, minY, maxX, maxY, width: boxW, height: boxH }
        };
    }

    function countHoles(mask) {
        const seen = new Uint8Array(mask.length);
        const queue = [];
        function pushIfOpen(x, y) {
            if (x < 0 || y < 0 || x >= normSize || y >= normSize) {
                return;
            }
            const index = y * normSize + x;
            if (!mask[index] && !seen[index]) {
                seen[index] = 1;
                queue.push([x, y]);
            }
        }
        for (let i = 0; i < normSize; ++i) {
            pushIfOpen(i, 0);
            pushIfOpen(i, normSize - 1);
            pushIfOpen(0, i);
            pushIfOpen(normSize - 1, i);
        }
        while (queue.length) {
            const [x, y] = queue.shift();
            pushIfOpen(x + 1, y);
            pushIfOpen(x - 1, y);
            pushIfOpen(x, y + 1);
            pushIfOpen(x, y - 1);
        }

        let holes = 0;
        for (let y = 1; y < normSize - 1; ++y) {
            for (let x = 1; x < normSize - 1; ++x) {
                const index = y * normSize + x;
                if (mask[index] || seen[index]) {
                    continue;
                }
                let area = 0;
                seen[index] = 1;
                queue.push([x, y]);
                while (queue.length) {
                    const [qx, qy] = queue.shift();
                    ++area;
                    pushIfOpen(qx + 1, qy);
                    pushIfOpen(qx - 1, qy);
                    pushIfOpen(qx, qy + 1);
                    pushIfOpen(qx, qy - 1);
                }
                if (area >= 5) {
                    ++holes;
                }
            }
        }
        return holes;
    }

    function buildTemplates() {
        if (templateCache) {
            return templateCache;
        }
        const fonts = ["Times New Roman", "Georgia", "Arial", "Verdana", "Courier New", "serif", "sans-serif"];
        const sizes = [42, 46, 50, 54];
        const weights = ["normal", "bold"];
        const templates = [];
        for (let digit = 1; digit <= 9; ++digit) {
            for (const font of fonts) {
                for (const size of sizes) {
                    for (const weight of weights) {
                        const canvas = makeCanvas(72, 72);
                        const ctx = canvas.getContext("2d", { willReadFrequently: true });
                        ctx.fillStyle = "#fff";
                        ctx.fillRect(0, 0, 72, 72);
                        ctx.fillStyle = "#000";
                        ctx.textAlign = "center";
                        ctx.textBaseline = "middle";
                        const fontName = font.includes(" ") ? `"${font}"` : font;
                        ctx.font = `${weight} ${size}px ${fontName}`;
                        ctx.fillText(String(digit), 36, 39);
                        const image = ctx.getImageData(0, 0, 72, 72);
                        const normalized = normalizeMask(72, 72, (x, y) => grayAt(image.data, (y * 72 + x) * 4) < 205);
                        if (!normalized || normalized.ink < 20) {
                            continue;
                        }
                        normalized.holes = countHoles(normalized.mask);
                        templates.push({ digit, ...normalized });
                    }
                }
            }
        }
        templateCache = templates;
        return templates;
    }

    function compareMask(sample, template) {
        let intersection = 0;
        let union = 0;
        let templateInk = 0;
        let sampleInk = 0;
        for (let i = 0; i < sample.mask.length; ++i) {
            const a = sample.mask[i] !== 0;
            const b = template.mask[i] !== 0;
            if (a) {
                ++sampleInk;
            }
            if (b) {
                ++templateInk;
            }
            if (a && b) {
                ++intersection;
            }
            if (a || b) {
                ++union;
            }
        }
        const iou = union ? intersection / union : 0;
        const precision = sampleInk ? intersection / sampleInk : 0;
        const recall = templateInk ? intersection / templateInk : 0;
        const ratioPenalty = Math.abs(sample.ratio - template.ratio) * 1.1;
        const aspectPenalty = Math.abs(sample.aspect - template.aspect) * 0.16;
        const holePenalty = Math.min(0.16, Math.abs((sample.holes || 0) - (template.holes || 0)) * 0.055);
        let score = iou * 0.54 + precision * 0.21 + recall * 0.25 - ratioPenalty - aspectPenalty - holePenalty;
        if ((sample.holes || 0) >= 1 && (template.digit === 6 || template.digit === 8 || template.digit === 9)) {
            score += 0.035;
        }
        if ((sample.holes || 0) === 0 && (template.digit === 6 || template.digit === 8 || template.digit === 9)) {
            score -= 0.02;
        }
        return score;
    }

    function recognizeMask(sample) {
        const templates = buildTemplates();
        const bestPerDigit = new Map();
        for (const template of templates) {
            const score = compareMask(sample, template);
            const current = bestPerDigit.get(template.digit);
            if (!current || score > current.score) {
                bestPerDigit.set(template.digit, { digit: template.digit, score });
            }
        }
        const ranked = Array.from(bestPerDigit.values()).sort((a, b) => b.score - a.score);
        const best = ranked[0] || { digit: 0, score: 0 };
        const second = ranked[1] || { digit: 0, score: 0 };
        const confidence = clamp(best.score * 0.72 + (best.score - second.score) * 2.1, 0, 1);
        const accepted = best.score > 0.18 || (best.score > 0.12 && confidence > 0.24);
        return {
            digit: accepted ? best.digit : 0,
            confidence,
            score: best.score,
            second: second.score
        };
    }

    function recognizeCell(data, width, height, x0, y0, x1, y1) {
        const left = Math.max(0, Math.floor(x0));
        const top = Math.max(0, Math.floor(y0));
        const right = Math.min(width - 1, Math.ceil(x1));
        const bottom = Math.min(height - 1, Math.ceil(y1));
        const cellW = Math.max(1, right - left + 1);
        const cellH = Math.max(1, bottom - top + 1);
        let darkPixels = 0;
        for (let y = 0; y < cellH; ++y) {
            for (let x = 0; x < cellW; ++x) {
                const index = ((top + y) * width + (left + x)) * 4;
                if (isDigitInkPixel(data, index)) {
                    ++darkPixels;
                }
            }
        }
        const normalized = normalizeMask(cellW, cellH, (x, y) => {
            const index = ((top + y) * width + (left + x)) * 4;
            return isDigitInkPixel(data, index);
        });
        const ratio = darkPixels / Math.max(1, cellW * cellH);
        if (!normalized || normalized.ink < 10 || ratio < 0.006) {
            return { digit: 0, confidence: 1, empty: true };
        }
        if (normalized.box.width < cellW * 0.12 || normalized.box.height < cellH * 0.18) {
            return { digit: 0, confidence: 0.8, empty: true };
        }
        normalized.holes = countHoles(normalized.mask);
        const result = recognizeMask(normalized);
        return {
            digit: result.digit,
            confidence: result.confidence,
            empty: result.digit === 0,
            score: result.score,
            box: normalized.box
        };
    }

    function recognizeSudokuFromImage(image) {
        const canvas = drawImageToCanvas(image);
        const ctx = canvas.getContext("2d", { willReadFrequently: true });
        const width = canvas.width;
        const height = canvas.height;
        const imageData = ctx.getImageData(0, 0, width, height);
        const grid = findGrid(imageData.data, width, height);
        const cells = [];
        let puzzle = "";
        let recognized = 0;
        let lowConfidence = 0;

        for (let row = 0; row < 9; ++row) {
            for (let col = 0; col < 9; ++col) {
                const cellW = grid.xLines[col + 1] - grid.xLines[col];
                const cellH = grid.yLines[row + 1] - grid.yLines[row];
                const insetX = Math.max(2, Math.abs(cellW) * 0.15);
                const insetY = Math.max(2, Math.abs(cellH) * 0.15);
                const result = recognizeCell(
                    imageData.data,
                    width,
                    height,
                    grid.xLines[col] + insetX,
                    grid.yLines[row] + insetY,
                    grid.xLines[col + 1] - insetX,
                    grid.yLines[row + 1] - insetY
                );
                if (result.digit) {
                    ++recognized;
                    if (result.confidence < 0.38) {
                        ++lowConfidence;
                    }
                    puzzle += String(result.digit);
                } else {
                    puzzle += "0";
                }
                cells.push({ row, col, ...result });
            }
        }

        return { puzzle, cells, recognized, lowConfidence };
    }

    function renderRecognitionBadges(cells) {
        const preview = byId("srrOcrPreview");
        if (!preview) {
            return;
        }
        const previous = preview.querySelector(".srr-ocr-cell-badges");
        if (previous) {
            previous.remove();
        }
        const badges = document.createElement("div");
        badges.className = "srr-ocr-cell-badges";
        badges.setAttribute("aria-hidden", "true");
        for (const cell of cells) {
            if (!cell.digit) {
                continue;
            }
            const badge = document.createElement("span");
            badge.textContent = String(cell.digit);
            badge.style.gridColumn = String(cell.col + 1);
            badge.style.gridRow = String(cell.row + 1);
            if (cell.confidence < 0.38) {
                badge.className = "low";
            }
            badges.appendChild(badge);
        }
        preview.appendChild(badges);
    }

    function formatPuzzleForTextarea(puzzle) {
        return puzzle.match(/.{1,9}/g).join("\n");
    }

    function processSelectedImage() {
        if (!selectedImage || !selectedImage.complete || !(selectedImage.naturalWidth || selectedImage.width)) {
            updateCounter("Open an image first, then press Process Image.", false);
            return;
        }
        const button = byId("srrOcrProcess");
        if (button) {
            button.disabled = true;
            button.textContent = "Processing...";
        }
        window.setTimeout(() => {
            try {
                const result = recognizeSudokuFromImage(selectedImage);
                const input = byId("srrOcrPuzzleText");
                if (input) {
                    input.value = formatPuzzleForTextarea(result.puzzle);
                }
                renderRecognitionBadges(result.cells);
                const quality = result.lowConfidence > 0
                    ? `${result.lowConfidence} low-confidence cells need review.`
                    : "Review the filled string before importing.";
                updateCounter(`Processed ${selectedFileName || "image"}: ${result.recognized} digits detected. ${quality}`, result.recognized > 0);
            } catch (error) {
                updateCounter(`Image processing failed: ${error.message || error}`, false);
            } finally {
                if (button) {
                    button.disabled = false;
                    button.textContent = "Process Image";
                }
            }
        }, 20);
    }

    function setPreview(file) {
        const preview = byId("srrOcrPreview");
        if (!preview) {
            return;
        }
        preview.innerHTML = "";
        selectedImage = null;
        selectedFileName = "";
        if (objectUrl) {
            URL.revokeObjectURL(objectUrl);
            objectUrl = "";
        }
        if (!file) {
            preview.innerHTML = "<span>Open a screenshot to use it as a visual guide.</span><div class=\"srr-ocr-grid-overlay\" aria-hidden=\"true\"></div>";
            updateCounter("Open an image, then press Process Image or enter the puzzle manually.", true);
            return;
        }

        selectedFileName = file.name;
        objectUrl = URL.createObjectURL(file);
        const image = document.createElement("img");
        image.alt = "Selected Sudoku reference";
        image.onload = () => {
            selectedImage = image;
            updateCounter(`Loaded ${file.name}. Press Process Image to attempt local recognition.`, true);
        };
        image.onerror = () => updateCounter("The selected image could not be opened.", false);
        image.src = objectUrl;
        preview.appendChild(image);
        const grid = document.createElement("div");
        grid.className = "srr-ocr-grid-overlay";
        grid.setAttribute("aria-hidden", "true");
        preview.appendChild(grid);
    }

    function importIntoBoard() {
        const result = window.SRRBrowserOCR.importPuzzleString(cleanedValue());
        updateCounter(result.message, result.ok);
        if (result.ok) {
            const input = byId("srrOcrPuzzleText");
            if (input) {
                input.value = formatPuzzleForTextarea(result.puzzle);
            }
            window.setTimeout(closePanel, 360);
        }
    }

    function copyCleanString() {
        const puzzle = cleanedValue();
        if (!navigator.clipboard) {
            updateCounter("Clipboard API is not available in this browser.", false);
            return;
        }
        navigator.clipboard.writeText(puzzle)
            .then(() => updateCounter("Cleaned puzzle string copied.", true))
            .catch((error) => updateCounter(`Copy failed: ${error.message || error}`, false));
    }

    function buildPanel() {
        const status = window.SRRBrowserOCR.libraryStatus();
        const root = document.createElement("div");
        root.className = "srr-ocr-overlay";
        root.setAttribute("role", "dialog");
        root.setAttribute("aria-modal", "true");
        root.setAttribute("aria-label", "Browser image import");
        root.innerHTML = `
            <section class="srr-ocr-card">
                <header class="srr-ocr-header">
                    <div>
                        <p>Browser OCR bridge</p>
                        <h2>Image-Assisted Manual Import</h2>
                    </div>
                    <button class="srr-ocr-close" type="button" id="srrOcrClose">Close</button>
                </header>
                <div class="srr-ocr-status" id="srrOcrStatus">${status.message}</div>
                <div class="srr-ocr-body">
                    <div class="srr-ocr-image-pane">
                        <div class="srr-ocr-toolbar">
                            <label class="srr-ocr-file">
                                <input id="srrOcrImageInput" type="file" accept="image/png,image/jpeg,image/webp,image/bmp">
                                <span>Open Sudoku Image</span>
                            </label>
                            <button class="srr-ocr-process" type="button" id="srrOcrProcess">Process Image</button>
                        </div>
                        <div class="srr-ocr-preview" id="srrOcrPreview">
                            <span>Open a screenshot to use it as a visual guide.</span>
                            <div class="srr-ocr-grid-overlay" aria-hidden="true"></div>
                        </div>
                    </div>
                    <div class="srr-ocr-input-pane">
                        <div class="srr-ocr-help">
                            <strong>Recognized puzzle string</strong>
                            <span>Process Image attempts local template OCR. Use 1-9 for givens, 0 or dot for empty cells, and review before importing.</span>
                        </div>
                        <textarea id="srrOcrPuzzleText" spellcheck="false" inputmode="numeric" placeholder="530070000600195000098000060800060003400803001700020006060000280000419005000080079"></textarea>
                        <div class="srr-ocr-row">
                            <span id="srrOcrCounter">0/81 cleaned cells</span>
                            <button type="button" id="srrOcrClear">Clear</button>
                        </div>
                        <div class="srr-ocr-actions">
                            <button type="button" id="srrOcrImport">Import Into Board</button>
                            <button type="button" id="srrOcrCopy">Copy Clean String</button>
                        </div>
                        <p class="srr-ocr-note">Browser processing is a lightweight no-dependency recognizer for clean screenshots. For highest OCR accuracy, use the Windows ZIP with OpenCV and Tesseract.</p>
                    </div>
                </div>
            </section>`;
        return root;
    }

    function openImportPanel() {
        if (panel) {
            panel.classList.add("visible");
            const text = byId("srrOcrPuzzleText");
            if (text) {
                text.focus();
            }
            return;
        }

        panel = buildPanel();
        document.body.appendChild(panel);
        requestAnimationFrame(() => panel && panel.classList.add("visible"));

        byId("srrOcrClose").addEventListener("click", closePanel);
        byId("srrOcrImageInput").addEventListener("change", (event) => {
            setPreview(event.target.files && event.target.files[0]);
        });
        byId("srrOcrProcess").addEventListener("click", processSelectedImage);
        byId("srrOcrPuzzleText").addEventListener("input", () => updateCounter());
        byId("srrOcrImport").addEventListener("click", importIntoBoard);
        byId("srrOcrCopy").addEventListener("click", copyCleanString);
        byId("srrOcrClear").addEventListener("click", () => {
            byId("srrOcrPuzzleText").value = "";
            renderRecognitionBadges([]);
            updateCounter("Puzzle string cleared.", true);
        });
        panel.addEventListener("click", (event) => {
            if (event.target === panel) {
                closePanel();
            }
        });
        window.addEventListener("keydown", function onKeydown(event) {
            if (!panel) {
                window.removeEventListener("keydown", onKeydown);
                return;
            }
            if (event.key === "Escape") {
                event.preventDefault();
                closePanel();
                window.removeEventListener("keydown", onKeydown);
            }
        });
        updateCounter("Open an image, then press Process Image or enter the puzzle manually.", true);
    }

    window.SRRBrowserOCR = Object.assign(window.SRRBrowserOCR || {}, {
        openImportPanel
    });
}());
