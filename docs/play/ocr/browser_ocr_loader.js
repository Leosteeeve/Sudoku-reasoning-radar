(function () {
    "use strict";

    const version = "v0.3.0-browser-ocr-bridge";

    function normalizePuzzleString(value) {
        return String(value || "")
            .split("")
            .filter((ch) => (ch >= "0" && ch <= "9") || ch === ".")
            .map((ch) => (ch === "." ? "0" : ch))
            .join("");
    }

    function libraryStatus() {
        const hasTesseract = Boolean(window.Tesseract);
        const hasOpenCV = Boolean(window.cv);
        return {
            hasTesseract,
            hasOpenCV,
            automaticOCR: hasTesseract && hasOpenCV,
            message: hasTesseract && hasOpenCV
                ? "Optional browser OCR libraries detected."
                : "Large OCR libraries are not bundled; lightweight local processing and manual review are available."
        };
    }

    function runtimeReady() {
        return Boolean(window.Module && typeof window.Module.ccall === "function");
    }

    function importPuzzleString(rawValue) {
        const puzzle = normalizePuzzleString(rawValue);
        if (puzzle.length !== 81) {
            return {
                ok: false,
                puzzle,
                message: `Need exactly 81 digits or dots; current cleaned length is ${puzzle.length}.`
            };
        }
        if (!runtimeReady()) {
            return {
                ok: false,
                puzzle,
                message: "WebAssembly runtime is still loading. Try again after the board appears."
            };
        }

        try {
            window.Module.ccall("SRR_ImportPuzzleString", null, ["string"], [puzzle]);
            return {
                ok: true,
                puzzle,
                message: "Imported puzzle string into the board."
            };
        } catch (error) {
            return {
                ok: false,
                puzzle,
                message: `Import failed: ${error.message || error}`
            };
        }
    }

    window.SRRBrowserOCR = Object.assign(window.SRRBrowserOCR || {}, {
        version,
        normalizePuzzleString,
        libraryStatus,
        isAutomaticOCRAvailable() {
            return libraryStatus().automaticOCR;
        },
        importPuzzleString
    });
}());
