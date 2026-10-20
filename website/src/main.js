import "@fontsource-variable/geist";
import "@fontsource-variable/geist-mono";
import { setupSearch } from "./search.js";

setupSearch();

// Code blocks with tabs: [data-tab] buttons show the [data-panel] with the same name.
document.querySelectorAll("[data-tabs]").forEach((root) => {
    const tabs = root.querySelectorAll("[data-tab]");
    const panels = root.querySelectorAll("[data-panel]");

    tabs.forEach((tab) => {
        tab.addEventListener("click", () => {
            tabs.forEach((t) => t.setAttribute("aria-pressed", String(t === tab)));
            panels.forEach((p) => (p.hidden = p.dataset.panel !== tab.dataset.tab));
        });
    });
});

// Copies the visible code of the block the button belongs to.
document.querySelectorAll("[data-copy]").forEach((button) => {
    button.addEventListener("click", async () => {
        const scope = button.closest("[data-tabs], [data-copy-scope]");
        const code = scope?.querySelector("[data-panel]:not([hidden]) code") ?? scope?.querySelector("code");
        if (!code)
            return;

        await navigator.clipboard.writeText(code.innerText);
        button.textContent = "copied";
        setTimeout(() => (button.textContent = "copy"), 1400);
    });
});

// Only the module name and description are searched, so a dependency like yup_core doesn't match everything.
const moduleFilter = document.getElementById("module-filter");

const moduleMatches = (module, query) =>
    [...module.querySelectorAll("[data-filter-text]")].some((text) => text.textContent.toLowerCase().includes(query));

moduleFilter?.addEventListener("input", () => {
    const query = moduleFilter.value.trim().toLowerCase();

    document.querySelectorAll("[data-module]").forEach((module) => {
        module.hidden = query !== "" && !moduleMatches(module, query);
    });

    document.querySelectorAll("[data-group]").forEach((group) => {
        group.hidden = group.querySelector("[data-module]:not([hidden])") === null;
    });
});
