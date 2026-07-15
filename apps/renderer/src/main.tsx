import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { CoreShell } from "./app";

const starterPuzzle =
  "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

createRoot(document.getElementById("root")!).render(
  <StrictMode><CoreShell initialPuzzle={starterPuzzle} /></StrictMode>,
);
