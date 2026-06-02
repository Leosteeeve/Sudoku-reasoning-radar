(function () {
    const navLinks = Array.from(document.querySelectorAll(".nav-links a"));
    const revealItems = Array.from(document.querySelectorAll(".reveal"));
    const techniqueBadge = document.getElementById("techniqueBadge");
    const stepCounter = document.getElementById("stepCounter");
    const previewFrame = document.getElementById("previewFrame");

    const techniques = [
        "Naked Single",
        "Hidden Single",
        "Locked Candidate",
        "MRV Guess",
        "Backtrack"
    ];
    let techniqueIndex = 0;

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
            target.scrollIntoView({ behavior: "smooth", block: "start" });
        });
    });

    const revealObserver = "IntersectionObserver" in window
        ? new IntersectionObserver((entries) => {
            entries.forEach((entry) => {
                if (entry.isIntersecting) {
                    entry.target.classList.add("visible");
                    revealObserver.unobserve(entry.target);
                }
            });
        }, { threshold: 0.18 })
        : null;

    revealItems.forEach((item) => {
        if (revealObserver) {
            revealObserver.observe(item);
        } else {
            item.classList.add("visible");
        }
    });

    const updateActiveNav = () => {
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
    };

    window.addEventListener("scroll", updateActiveNav, { passive: true });
    updateActiveNav();

    if (techniqueBadge && stepCounter) {
        window.setInterval(() => {
            techniqueIndex = (techniqueIndex + 1) % techniques.length;
            techniqueBadge.textContent = techniques[techniqueIndex];
            stepCounter.textContent = `Step ${18 + techniqueIndex * 3}`;
        }, 1800);
    }

    if (previewFrame) {
        const screenshotProbe = new Image();
        screenshotProbe.onload = () => {
            previewFrame.classList.add("has-screenshot");
        };
        screenshotProbe.onerror = () => {
            previewFrame.classList.remove("has-screenshot");
        };
        screenshotProbe.src = "assets/screenshot.png";
    }
})();
