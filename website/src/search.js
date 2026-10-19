// Docs search dialog (#search): opens from [data-search-open], Cmd/Ctrl+K or "/", and searches the
// sections of virtual:docs-index, which loads the first time the dialog opens.
export function setupSearch({ onOpen = () => {}, onClose = () => {}, slashOpens = () => true } = {}) {
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
            const empty = element("li", "search-empty", `No sections match "${query}".`);
            empty.setAttribute("role", "none");
            searchResults.replaceChildren(empty);
            return;
        }

        searchResults.replaceChildren(...ranked.map(({ entry }) => {
            const hit = Object.assign(element("a", "search-hit"), { href: entry.u, target: "_blank", rel: "noopener" });
            hit.setAttribute("role", "option");
            hit.append(element("span", "search-hit-title", entry.h));
            if (entry.p !== entry.h)
                hit.append(element("span", "search-hit-page", entry.p));
            hit.append(element("span", "search-hit-text", entry.t));

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
        onOpen();

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
        if ((e.key === "k" && (e.metaKey || e.ctrlKey)) || (e.key === "/" && !typing && slashOpens())) {
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

    searchDialog.addEventListener("close", onClose);
}
