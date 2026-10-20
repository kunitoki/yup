import "@fontsource-variable/geist";
import "@fontsource-variable/geist-mono";
import { setupSearch } from "./search.js";

// Built by the website_demos target in src/demos, which copies it into public/yup.
const moduleUrl = new URL("yup/website_demos.js", document.baseURI).href;
const moduleNames = "core events graphics gui audio dsp";

const body = document.body;
const links = [...document.querySelectorAll("main .works a")];
const mask = document.querySelector(".mask");
const readout = document.querySelector(".readout");
const bootText = document.querySelector(".boot-text");
const hoverless = window.matchMedia("(hover: none)");

let active = 0;
let running = false;
let failure = "";

// The mask repeats the list as solid letters: black hides the demo, white lets it through.
const cut = document.querySelector("main .works").cloneNode(true);
cut.removeAttribute("aria-label");
cut.querySelectorAll("a").forEach((link) => link.replaceWith(Object.assign(document.createElement("span"), { textContent: link.textContent })));
mask.append(cut);
const letters = [...cut.querySelectorAll("span")];

const callYup = (name, ...args) => running && window.Module[`_${name}`](...args);

const line = (text) => Object.assign(document.createElement("span"), { textContent: text });

const renderReadout = () => {
    const link = links[active];
    const page = link.hasAttribute("data-page");
    const facts = page ? "a plain page" : failure || moduleNames;
    const action = page ? "read" : running ? "open" : "";
    const verb = hoverless.matches ? "tap to" : "click to";

    readout.replaceChildren(link.dataset.note, line(facts));
    if (action)
        readout.append(line(`${verb} ${action} →`));
};

const pick = (index) => {
    active = index;
    links.forEach((link, i) => link.setAttribute("aria-current", String(i === index)));
    letters.forEach((letter, i) => letter.classList.toggle("on", i === index));

    const box = links[index].getBoundingClientRect();
    body.style.setProperty("--leak-top", `${box.top + box.height / 2 - 0.15 * innerHeight}px`);
    body.style.setProperty("--readout-top", `${box.top + box.height * 0.3}px`);
    body.style.setProperty("--readout-left", `${Math.min(0.72 * innerWidth, box.right + 0.025 * innerWidth)}px`);
    renderReadout();

    callYup("yupWebsiteSetDemo", index);
};

// The demos open their audio output suspended: browsers only let it start from a user gesture, which
// the click opening a demo is. A few seconds after closing, once the sound has faded out, it sleeps.
let audioSleep = 0;

const wakeAudio = () => {
    clearTimeout(audioSleep);
    window.Module?.yupAudioContext?.resume().catch(() => {});
};

const sleepAudio = () => {
    clearTimeout(audioSleep);
    audioSleep = setTimeout(() => window.Module?.yupAudioContext?.suspend().catch(() => {}), 3000);
};

const open = () => {
    if (!running)
        return;

    wakeAudio();
    body.classList.add("open");
    body.style.cursor = canvas.style.cursor;
    callYup("yupWebsiteSetOpen", 1);
};

const close = () => {
    body.classList.remove("open");
    body.style.cursor = "";
    callYup("yupWebsiteSetOpen", 0);
    sleepAudio();
};

links.forEach((link, index) => {
    link.addEventListener("pointerenter", (e) => {
        if (e.pointerType === "mouse" && !body.classList.contains("open"))
            pick(index);
    });

    // Only keyboard focus picks: a tap focuses the link too, and picks and opens it on click instead.
    link.addEventListener("focus", () => link.matches(":focus-visible") && pick(index));

    // With a mouse the first click on another work picks it; without hover a single tap opens it.
    link.addEventListener("click", (e) => {
        if (index !== active && !hoverless.matches) {
            e.preventDefault();
            pick(index);
            return;
        }

        if (index !== active)
            pick(index);

        if (!link.hasAttribute("data-page")) {
            e.preventDefault();
            open();
        }
    });
});

document.querySelector(".back").addEventListener("click", close);

document.addEventListener("keydown", (e) => {
    if (e.key === "Escape")
        close();
});

// SDL listens for keys on window and cancels the ones it handles, which would break typing and Tab on
// the page, so keys reach YUP only while a work is open and not inside the search dialog.
for (const type of ["keydown", "keypress", "keyup"])
    document.addEventListener(type, (e) => (!body.classList.contains("open") || e.target.closest?.("dialog")) && e.stopPropagation());

// The words cover the canvas, so the page hands YUP the pointer itself: moves always, presses only
// while a work is open and not on a page control. YUP captures the pointer on a press, which would
// send the release to the canvas and the click on back or search nowhere.
const canvas = document.getElementById("canvas");
const onControl = (e) => e.target.closest?.("a, button, input, dialog");
const forwardPointer = (e) => {
    if (!running || e.target === canvas)
        return;

    const { clientX, clientY, screenX, screenY, movementX, movementY, pointerId, pointerType, isPrimary, button, buttons, pressure, width, height } = e;
    canvas.dispatchEvent(new PointerEvent(e.type, { clientX, clientY, screenX, screenY, movementX, movementY, pointerId, pointerType, isPrimary, button, buttons, pressure, width, height }));
};

document.addEventListener("pointermove", (e) => (e.pointerType === "mouse" || body.classList.contains("open")) && forwardPointer(e));
for (const type of ["pointerdown", "pointerup", "pointercancel"])
    document.addEventListener(type, (e) => body.classList.contains("open") && !onControl(e) && forwardPointer(e));
document.documentElement.addEventListener("pointerleave", forwardPointer);

// YUP sets the cursor on the canvas, which the browser ignores since the canvas takes no pointer
// events: while a work is open the page shows it instead.
new MutationObserver(() => {
    if (body.classList.contains("open"))
        body.style.cursor = canvas.style.cursor;
}).observe(canvas, { attributes: true, attributeFilter: ["style"] });

// The canvas takes no pointer events itself, so the wheel is handed over too while a work is open.
document.addEventListener("wheel", (e) => {
    if (!running || !body.classList.contains("open") || onControl(e))
        return;

    const { deltaX, deltaY, deltaZ, deltaMode, clientX, clientY, screenX, screenY, buttons, ctrlKey, shiftKey, altKey, metaKey } = e;
    canvas.dispatchEvent(new WheelEvent("wheel", { deltaX, deltaY, deltaZ, deltaMode, clientX, clientY, screenX, screenY, buttons, ctrlKey, shiftKey, altKey, metaKey }));
});

window.addEventListener("resize", () => pick(active));
document.fonts.ready.then(() => pick(active));

// Inside an open work "/" is a key for YUP (a code editor, a text field), not the search shortcut.
setupSearch({ slashOpens: () => !body.classList.contains("open") });

// --------------------------------------------------------------------------- YUP

const fail = (reason) => {
    running = false;
    failure = reason;
    body.classList.remove("ready", "open");
    bootText.textContent = `yup  ${reason}`;
    setTimeout(() => {
        body.classList.add("static");
        renderReadout();
    }, 1600);
};

// The window exists once the frame rate getters stop returning -1.
const waitForWindow = () => {
    if (window.Module._yupIsVsyncEnabled() < 0) {
        requestAnimationFrame(waitForWindow);
        return;
    }

    running = true;
    window.dispatchEvent(new Event("resize"));
    body.classList.add("ready");
};

const startYup = () => {
    if (!navigator.gpu) {
        fail("needs webgpu");
        return;
    }

    // mini-coi.js reloads the page once its service worker isolates it.
    if (!window.crossOriginIsolated) {
        setTimeout(() => fail("needs isolation"), 4000);
        return;
    }

    window.Module = {
        canvas: document.getElementById("canvas"),
        preRun: [
            () => {
                addRunDependency("webgpu-device");
                let requestedCompatibility = false;
                navigator.gpu.requestAdapter().then((adapter) => {
                    if (adapter != null)
                        return adapter;

                    // Compatibility mode runs on older GPUs and on browsers implementing WebGPU over OpenGL ES
                    requestedCompatibility = true;
                    return navigator.gpu.requestAdapter({ featureLevel: "compatibility" });
                }).then((adapter) => {
                    if (adapter == null) throw new Error("WebGPU: no adapter");
                    return adapter.requestDevice();
                }).then((device) => {
                    // An adapter returned for a compatibility request may still give a full (core) device
                    window.Module.webGPUCompatibilityMode = requestedCompatibility && !device.features.has("core-features-and-limits");
                    window.Module.preinitializedWebGPUDevice = device;
                    removeRunDependency("webgpu-device");
                }).catch((e) => {
                    console.error(`WebGPU initialization failed: ${e}`);
                    fail("needs webgpu");
                });
            },
        ],
        postRun: [waitForWindow],
        onAbort: () => fail("stopped"),
    };

    const script = Object.assign(document.createElement("script"), { src: moduleUrl, async: true });
    script.addEventListener("error", () => fail("not built yet"));
    // The dev server answers a missing file with index.html, which loads without defining anything.
    script.addEventListener("load", () => typeof window.addRunDependency !== "function" && fail("not built yet"));
    body.append(script);
};

startYup();
