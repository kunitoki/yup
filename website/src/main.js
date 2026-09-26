import Lenis from "lenis";
import "lenis/dist/lenis.css";

// Eases wheel ticks into continuous scrolling; touch stays native and code blocks keep their own horizontal scroll.
const lenis = new Lenis({ autoRaf: true, anchors: true, allowNestedScroll: true });

const navToggle = document.getElementById("nav-toggle");
const navMenu = document.getElementById("nav-menu");

navToggle?.addEventListener("click", () => {
    const open = navMenu.classList.toggle("hidden") === false;
    navMenu.classList.toggle("flex", open);
    navToggle.setAttribute("aria-expanded", String(open));
});

document.querySelectorAll(".spotlight").forEach((card) => {
    card.addEventListener("pointermove", (e) => {
        const r = card.getBoundingClientRect();
        card.style.setProperty("--mx", `${e.clientX - r.left}px`);
        card.style.setProperty("--my", `${e.clientY - r.top}px`);
    });
});

document.querySelectorAll("[data-tabs]").forEach((root) => {
    const tabs = root.querySelectorAll("[data-tab]");
    const panels = root.querySelectorAll("[data-panel]");

    tabs.forEach((tab) => {
        tab.addEventListener("click", () => {
            tabs.forEach((t) => t.setAttribute("aria-pressed", String(t === tab)));
            panels.forEach((p) => p.classList.toggle("hidden", p.dataset.panel !== tab.dataset.tab));
        });
    });
});

document.querySelectorAll("[data-copy]").forEach((button) => {
    button.addEventListener("click", async () => {
        const scope = button.closest("[data-tabs], [data-copy-scope]");
        const code = scope?.querySelector("[data-panel]:not(.hidden) code") ?? scope?.querySelector("code");
        if (!code) return;

        await navigator.clipboard.writeText(code.innerText);
        button.textContent = "copied";
        setTimeout(() => (button.textContent = "copy"), 1400);
    });
});

const moduleFilter = document.getElementById("module-filter");

moduleFilter?.addEventListener("input", () => {
    const query = moduleFilter.value.trim().toLowerCase();

    document.querySelectorAll("[data-module]").forEach((card) => {
        card.hidden = query !== "" && !card.textContent.toLowerCase().includes(query);
    });

    document.querySelectorAll("[data-group]").forEach((group) => {
        group.hidden = group.querySelector("[data-module]:not([hidden])") === null;
    });
});

const searchDialog = document.getElementById("search");
const searchInput = document.getElementById("search-input");
const searchResults = document.getElementById("search-results");
const searchMore = document.getElementById("search-more");
let searchIndex;
let activeHit = 0;

const element = (tag, className, text) => Object.assign(document.createElement(tag), { className, textContent: text });

const hits = () => [...searchResults.querySelectorAll("a")];

const setActiveHit = (index) => {
    const all = hits();
    activeHit = Math.max(0, Math.min(index, all.length - 1));
    all.forEach((hit, i) => hit.setAttribute("aria-selected", String(i === activeHit)));
    all[activeHit]?.scrollIntoView({ block: "nearest" });
};

// Every term must appear somewhere in the entry; matches in the heading rank first.
const renderSearch = () => {
    const query = searchInput.value.trim();
    const terms = query.toLowerCase().split(/\s+/).filter(Boolean);
    searchMore.href = `https://yup.readthedocs.io/en/latest/search.html?q=${encodeURIComponent(query)}`;

    if (!searchIndex || terms.length === 0) {
        searchResults.replaceChildren();
        return;
    }

    const ranked = searchIndex
        .filter((entry) => terms.every((term) => entry.haystack.includes(term)))
        .map((entry) => ({ entry, score: terms.filter((term) => entry.heading.includes(term)).length }))
        .sort((a, b) => b.score - a.score || a.entry.h.length - b.entry.h.length)
        .slice(0, 12);

    if (ranked.length === 0) {
        const empty = element("li", "px-3 py-6 text-center text-sm text-muted", `No sections match "${query}".`);
        empty.setAttribute("role", "none");
        searchResults.replaceChildren(empty);
        return;
    }

    searchResults.replaceChildren(...ranked.map(({ entry }) => {
        const hit = Object.assign(element("a", "search-hit"), { href: entry.u, target: "_blank", rel: "noopener" });
        hit.setAttribute("role", "option");
        hit.append(element("span", "block text-sm font-medium text-ink", entry.h));
        if (entry.p !== entry.h)
            hit.append(element("span", "block font-mono text-[11px] text-glow-soft", entry.p));
        hit.append(element("span", "mt-1 block text-xs leading-relaxed text-muted", entry.t));

        const item = document.createElement("li");
        item.setAttribute("role", "none");
        item.append(hit);
        return item;
    }));
    setActiveHit(0);
};

const openSearch = async () => {
    if (searchDialog.open)
        return;

    searchDialog.showModal();
    lenis.stop();

    searchIndex ??= (await import("virtual:docs-index")).default.map((entry) => ({
        ...entry,
        heading: entry.h.toLowerCase(),
        haystack: `${entry.h} ${entry.p} ${entry.k} ${entry.t}`.toLowerCase(),
    }));
    renderSearch();
};

if (!/Mac|iPhone|iPad/.test(navigator.platform))
    document.querySelectorAll("[data-search-key]").forEach((key) => (key.textContent = "Ctrl K"));

document.querySelectorAll("[data-search-open]").forEach((button) => button.addEventListener("click", openSearch));

document.addEventListener("keydown", (e) => {
    const typing = e.target.closest?.("input, textarea, [contenteditable]");
    if ((e.key === "k" && (e.metaKey || e.ctrlKey)) || (e.key === "/" && !typing)) {
        e.preventDefault();
        openSearch();
    }
});

searchInput.addEventListener("input", renderSearch);

searchInput.addEventListener("keydown", (e) => {
    if (e.key === "ArrowDown" || e.key === "ArrowUp") {
        e.preventDefault();
        setActiveHit(activeHit + (e.key === "ArrowDown" ? 1 : -1));
    } else if (e.key === "Enter") {
        hits()[activeHit]?.click();
    }
});

searchResults.addEventListener("click", (e) => {
    if (e.target.closest("a"))
        searchDialog.close();
});

// Clicks on the backdrop land on the dialog itself.
searchDialog.addEventListener("click", (e) => {
    if (e.target === searchDialog)
        searchDialog.close();
});

searchDialog.addEventListener("close", () => lenis.start());

const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;

document.querySelectorAll("[data-slideshow]").forEach((show) => {
    const slides = [...show.querySelectorAll("[data-slide]")];
    const buttons = [...show.querySelectorAll("[data-slide-to]")];
    const label = show.querySelector("[data-slide-label]");
    const chips = show.querySelector("[data-slide-chips]");
    let current = 0;
    let timer;

    const go = (index) => {
        current = (index + slides.length) % slides.length;
        const slide = slides[current];

        slides.forEach((s) => s.toggleAttribute("data-active", s === slide));
        buttons.forEach((b, i) => b.setAttribute("aria-pressed", String(i === current)));
        label.textContent = slide.dataset.label;
        chips.replaceChildren(...slide.dataset.chips.split(",").map((name) =>
            Object.assign(document.createElement("span"), { className: "chip chip-on", textContent: name })));
    };

    const play = () => {
        clearInterval(timer);
        if (!reducedMotion)
            timer = setInterval(() => go(current + 1), 5000);
    };

    buttons.forEach((button, i) => button.addEventListener("click", () => { go(i); play(); }));
    show.addEventListener("mouseenter", () => clearInterval(timer));
    show.addEventListener("mouseleave", play);
    play();
});
