(function () {
    const canvas = document.getElementById("canvas");
    const appShell = document.querySelector(".app-shell");
    const canvasWrap = document.querySelector(".canvas-wrap");
    const loadingPanel = document.getElementById("loadingPanel");
    const statusText = document.getElementById("statusText");
    const progressFill = document.getElementById("progressFill");
    const runtimeLog = document.getElementById("runtimeLog");
    const fullscreenButton = document.getElementById("fullscreenButton");

    const buildStamp = "20260604-web-parity-hotfix";
    let dependencyTotal = 0;
    let runtimeReady = false;
    let lastWidth = 0;
    let lastHeight = 0;
    let resizeRaf = 0;

    function logLine(text, kind) {
        if (!runtimeLog || !text) {
            return;
        }
        const prefix = kind === "error" ? "[error] " : "";
        runtimeLog.textContent = `${runtimeLog.textContent}${prefix}${text}\n`;
        runtimeLog.scrollTop = runtimeLog.scrollHeight;
    }

    function setStatus(text) {
        if (statusText && text) {
            statusText.textContent = text;
        }
    }

    function setProgress(percent) {
        if (progressFill) {
            progressFill.style.width = `${Math.max(8, Math.min(100, percent))}%`;
        }
    }

    function hideLoading() {
        setProgress(100);
        setStatus("Runtime ready.");
        window.setTimeout(() => {
            if (loadingPanel) {
                loadingPanel.classList.add("hidden");
            }
            if (canvas) {
                canvas.focus({ preventScroll: true });
            }
        }, 240);
    }

    function showBuildMissing() {
        setStatus("WebAssembly build files were not found.");
        setProgress(100);
        logLine("Missing SudokuReasoningRadar.js or SudokuReasoningRadar.wasm.", "error");
        logLine("Run scripts/build_web.bat after activating D:\\emsdk\\emsdk_env.bat.", "error");
    }

    function notifyRuntime(width, height) {
        if (!runtimeReady || !window.Module) {
            return;
        }
        try {
            if (typeof window.Module.ccall === "function") {
                window.Module.ccall("SRR_OnCanvasResize", null, ["number", "number"], [width, height]);
            }
        } catch (error) {
            logLine(`Resize notification failed: ${error.message}`, "error");
        }
    }

    function resizeCanvas() {
        if (!canvas) {
            return;
        }
        const rect = (canvasWrap || canvas).getBoundingClientRect();
        const cssWidth = Math.max(1, Math.floor(canvas.clientWidth || rect.width));
        const cssHeight = Math.max(1, Math.floor(canvas.clientHeight || rect.height));
        const dpr = window.devicePixelRatio || 1;

        // SDL input and rendering currently use logical CSS pixels. Keeping the
        // backing store in the same coordinate system avoids visual/click drift.
        if (canvas.width !== cssWidth) {
            canvas.width = cssWidth;
        }
        if (canvas.height !== cssHeight) {
            canvas.height = cssHeight;
        }
        canvas.dataset.devicePixelRatio = String(dpr);

        if (cssWidth !== lastWidth || cssHeight !== lastHeight) {
            lastWidth = cssWidth;
            lastHeight = cssHeight;
            notifyRuntime(cssWidth, cssHeight);
        }
    }

    function queueResize() {
        if (resizeRaf) {
            window.cancelAnimationFrame(resizeRaf);
        }
        resizeRaf = window.requestAnimationFrame(() => {
            resizeRaf = 0;
            resizeCanvas();
        });
    }

    window.Module = {
        canvas,
        print(text) {
            logLine(text);
        },
        printErr(text) {
            logLine(text, "error");
        },
        locateFile(path) {
            if (path.endsWith(".wasm") || path.endsWith(".data")) {
                return `${path}?v=${buildStamp}`;
            }
            return path;
        },
        setStatus,
        monitorRunDependencies(left) {
            dependencyTotal = Math.max(dependencyTotal, left);
            const loaded = dependencyTotal ? dependencyTotal - left : 0;
            const percent = dependencyTotal ? 20 + (loaded / dependencyTotal) * 65 : 18;
            setProgress(percent);
            if (left === 0) {
                setStatus("Starting WebAssembly module...");
            } else {
                setStatus(`Loading runtime dependencies: ${left}`);
            }
        },
        onRuntimeInitialized() {
            runtimeReady = true;
            resizeCanvas();
            window.setTimeout(queueResize, 0);
            window.setTimeout(queueResize, 250);
            hideLoading();
        }
    };

    if (fullscreenButton) {
        fullscreenButton.addEventListener("click", () => {
            const target = appShell || canvasWrap || canvas;
            if (!document.fullscreenElement && target.requestFullscreen) {
                target.requestFullscreen()
                    .then(() => queueResize())
                    .catch((error) => logLine(error.message, "error"));
            } else if (document.exitFullscreen) {
                document.exitFullscreen()
                    .then(() => queueResize())
                    .catch((error) => logLine(error.message, "error"));
            }
        });
    }

    window.addEventListener("error", (event) => {
        if (String(event.filename || "").includes("SudokuReasoningRadar.js")) {
            showBuildMissing();
            return;
        }
        if (event.message) {
            logLine(event.message, "error");
        }
    });

    window.addEventListener("unhandledrejection", (event) => {
        const reason = event.reason && event.reason.message ? event.reason.message : String(event.reason || "Unhandled promise rejection");
        logLine(reason, "error");
    });

    setStatus("Loading SudokuReasoningRadar.js...");
    setProgress(16);
    resizeCanvas();
    window.addEventListener("resize", queueResize);
    window.addEventListener("fullscreenchange", queueResize);
    window.addEventListener("orientationchange", queueResize);
    if (window.visualViewport) {
        window.visualViewport.addEventListener("resize", queueResize);
    }

    const script = document.createElement("script");
    script.src = `SudokuReasoningRadar.js?v=${buildStamp}`;
    script.async = true;
    script.onerror = showBuildMissing;
    document.body.appendChild(script);
}());
