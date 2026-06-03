(function () {
    const navLinks = Array.from(document.querySelectorAll(".nav-links a"))
        .filter((link) => (link.getAttribute("href") || "").startsWith("#"));
    const revealItems = Array.from(document.querySelectorAll(".reveal"));
    const demoBoard = document.querySelector("[data-demo-board]");
    const techniqueEl = document.getElementById("demoTechnique");
    const reasonEl = document.getElementById("demoReason");
    const stepEl = document.getElementById("demoStep");
    const cellEl = document.getElementById("demoCell");
    const depthEl = document.getElementById("demoDepth");
    const meterEl = document.getElementById("demoMeter");
    const railItems = Array.from(document.querySelectorAll(".rail-item"));

    const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;

    const givens = {
        0: "5", 3: "8", 7: "9", 10: "7", 14: "2", 20: "3", 24: "6",
        27: "8", 31: "6", 35: "3", 39: "4", 43: "1", 47: "6", 51: "7",
        55: "1", 59: "9", 65: "9", 69: "8", 74: "2", 78: "6"
    };

    const solved = {
        5: "4", 11: "1", 16: "6", 18: "9", 22: "7", 26: "2",
        29: "5", 34: "9", 37: "6", 41: "8", 45: "2", 49: "5",
        57: "8", 61: "3", 63: "7", 67: "1", 73: "4", 76: "9"
    };

    const candidateSets = [
        "124", "379", "269", "167", "236", "368", "489", "135",
        "146", "158", "245", "247", "134", "278", "147", "258",
        "139", "257", "369", "348", "179", "238", "149", "256",
        "479", "235", "146", "578", "246", "135", "248", "356",
        "247", "158", "367", "145", "238", "159"
    ];

    const demoSteps = [
        {
            technique: "Naked Single",
            reason: "Only one candidate survives the row, column, and box masks.",
            step: "Step 18",
            cell: "r4c7 = 5",
            depth: "Depth 0",
            focus: 38,
            value: "5",
            meter: 34
        },
        {
            technique: "Hidden Single",
            reason: "The digit appears in one legal position inside this unit.",
            step: "Step 31",
            cell: "r2c3 = 1",
            depth: "Depth 0",
            focus: 11,
            value: "1",
            meter: 48
        },
        {
            technique: "Locked Candidate",
            reason: "Candidates are confined to one line, so peers outside the box fade out.",
            step: "Step 46",
            cell: "remove 7",
            depth: "Depth 0",
            focus: 23,
            value: "",
            meter: 56
        },
        {
            technique: "MRV Guess",
            reason: "Logic stalls. The engine branches on the cell with the fewest candidates.",
            step: "Step 73",
            cell: "r6c2 assume 3",
            depth: "Depth 1",
            focus: 46,
            value: "3",
            meter: 72
        },
        {
            technique: "Contradiction",
            reason: "A branch empties a candidate set, so the trace marks failure before backtracking.",
            step: "Step 79",
            cell: "branch failed",
            depth: "Depth 1",
            focus: 46,
            value: "",
            meter: 78,
            conflict: true
        },
        {
            technique: "Backtrack",
            reason: "The failed assumption is reverted and the next candidate is tested.",
            step: "Step 80",
            cell: "revert r6c2",
            depth: "Depth 0",
            focus: 46,
            value: "",
            meter: 82
        },
        {
            technique: "Solved Unique",
            reason: "The search confirms exactly one valid completion.",
            step: "Step 104",
            cell: "unique solution",
            depth: "Depth 0",
            focus: 80,
            value: "7",
            meter: 100
        }
    ];

    let demoIndex = 0;
    let railIndex = 0;

    function smoothAnchors() {
        document.querySelectorAll('a[href^="#"]').forEach((link) => {
            link.addEventListener("click", (event) => {
                const targetId = link.getAttribute("href");
                if (!targetId || targetId === "#github-link-placeholder") {
                    return;
                }
                const target = document.querySelector(targetId);
                if (!target) {
                    return;
                }
                event.preventDefault();
                target.scrollIntoView({ behavior: reducedMotion ? "auto" : "smooth", block: "start" });
            });
        });
    }

    function setupReveal() {
        const revealObserver = "IntersectionObserver" in window
            ? new IntersectionObserver((entries) => {
                entries.forEach((entry) => {
                    if (entry.isIntersecting) {
                        entry.target.classList.add("visible");
                        revealObserver.unobserve(entry.target);
                    }
                });
            }, { threshold: 0.14 })
            : null;

        revealItems.forEach((item) => {
            if (revealObserver) {
                revealObserver.observe(item);
            } else {
                item.classList.add("visible");
            }
        });
    }

    function updateActiveNav() {
        const sections = navLinks
            .map((link) => document.querySelector(link.getAttribute("href")))
            .filter(Boolean);
        const current = sections
            .filter((section) => section.getBoundingClientRect().top < 160)
            .pop();
        navLinks.forEach((link) => {
            const target = document.querySelector(link.getAttribute("href"));
            link.classList.toggle("active", Boolean(current && target === current));
        });
    }

    function makeCandidates(text) {
        const wrapper = document.createElement("div");
        wrapper.className = "candidates";
        for (let n = 1; n <= 9; n += 1) {
            const span = document.createElement("span");
            span.textContent = text.includes(String(n)) ? String(n) : "";
            wrapper.appendChild(span);
        }
        return wrapper;
    }

    function buildDemoBoard() {
        if (!demoBoard) {
            return;
        }

        for (let index = 0; index < 81; index += 1) {
            const cell = document.createElement("div");
            cell.className = "demo-cell";
            if (Object.prototype.hasOwnProperty.call(givens, index)) {
                cell.classList.add("given");
                cell.textContent = givens[index];
            } else if (Object.prototype.hasOwnProperty.call(solved, index)) {
                cell.classList.add("solved");
                cell.textContent = solved[index];
            } else {
                const candidates = candidateSets[index % candidateSets.length];
                cell.appendChild(makeCandidates(candidates));
            }
            demoBoard.appendChild(cell);
        }
    }

    function peersFor(index) {
        const row = Math.floor(index / 9);
        const col = index % 9;
        const boxRow = Math.floor(row / 3);
        const boxCol = Math.floor(col / 3);
        return Array.from(demoBoard.children).map((cell, i) => {
            const r = Math.floor(i / 9);
            const c = i % 9;
            const sameRow = r === row;
            const sameCol = c === col;
            const sameBox = Math.floor(r / 3) === boxRow && Math.floor(c / 3) === boxCol;
            return { cell, sameRow, sameCol, sameBox };
        });
    }

    function renderDemoStep() {
        if (!demoBoard || !techniqueEl || !reasonEl || !stepEl || !cellEl || !depthEl || !meterEl) {
            return;
        }
        const step = demoSteps[demoIndex];
        const cells = Array.from(demoBoard.children);

        cells.forEach((cell) => {
            cell.classList.remove("focus", "peer", "box-peer", "conflict");
        });

        peersFor(step.focus).forEach(({ cell, sameRow, sameCol, sameBox }) => {
            if (sameBox) {
                cell.classList.add("box-peer");
            }
            if (sameRow || sameCol) {
                cell.classList.add("peer");
            }
        });

        const focusCell = cells[step.focus];
        if (focusCell) {
            focusCell.classList.add("focus");
            if (step.conflict) {
                focusCell.classList.add("conflict");
            }
            if (step.value) {
                focusCell.textContent = step.value;
                focusCell.classList.add("solved");
            }
        }

        techniqueEl.textContent = step.technique;
        reasonEl.textContent = step.reason;
        stepEl.textContent = step.step;
        cellEl.textContent = step.cell;
        depthEl.textContent = step.depth;
        meterEl.style.width = `${step.meter}%`;
    }

    function cycleRail() {
        if (!railItems.length) {
            return;
        }
        railItems.forEach((item, index) => {
            item.classList.toggle("active", index === railIndex);
        });
        railIndex = (railIndex + 1) % railItems.length;
    }

    function startAnimations() {
        buildDemoBoard();
        renderDemoStep();
        cycleRail();

        if (!reducedMotion) {
            window.setInterval(() => {
                demoIndex = (demoIndex + 1) % demoSteps.length;
                renderDemoStep();
            }, 2100);

            window.setInterval(cycleRail, 2600);
        }
    }

    smoothAnchors();
    setupReveal();
    startAnimations();
    updateActiveNav();
    window.addEventListener("scroll", updateActiveNav, { passive: true });
})();
