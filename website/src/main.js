import Lenis from "lenis";
import "lenis/dist/lenis.css";

// Eases wheel ticks into continuous scrolling; touch stays native and code blocks keep their own horizontal scroll.
new Lenis({ autoRaf: true, anchors: true, allowNestedScroll: true });

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
