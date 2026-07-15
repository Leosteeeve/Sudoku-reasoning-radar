import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { CoreShell } from "./app";

const starterPuzzle =
  "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

createRoot(document.getElementById("root")!).render(
  <StrictMode><CoreShell initialPuzzle={starterPuzzle} /></StrictMode>,
);

if ("serviceWorker" in navigator) {
  window.addEventListener("load", () => {
    const base = new URL(import.meta.env.BASE_URL, window.location.origin);
    void navigator.serviceWorker.register(new URL("sw.js", base)).then(() => navigator.serviceWorker.ready)
      .then(() => window.dispatchEvent(new CustomEvent("srr:offline-ready")))
      .catch(() => undefined);
  }, { once: true });
}
