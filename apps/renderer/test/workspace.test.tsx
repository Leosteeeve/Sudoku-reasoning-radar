import { render, screen, within } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { CoreClient } from "@srr/core-client";
import { CoreShell, ReasoningWorkspace } from "../src/app.tsx";
import { fixtureClient, fixtureStep, puzzle } from "./fixtures.ts";

describe("shared reasoning workspace", () => {
  beforeEach(() => {
    vi.stubGlobal("matchMedia", (query: string) => ({
      matches: query.includes("reduced-motion"),
      media: query,
      onchange: null,
      addEventListener: vi.fn(),
      removeEventListener: vi.fn(),
      addListener: vi.fn(),
      removeListener: vi.fn(),
      dispatchEvent: vi.fn(),
    }));
  });

  it("renders 81 labelled semantic cells, locks givens, and never uses Canvas", async () => {
    const user = userEvent.setup();
    const { container } = render(
      <ReasoningWorkspace initialPuzzle={puzzle} coreClient={fixtureClient()} initialLanguage="en" />,
    );

    expect(screen.getAllByRole("gridcell")).toHaveLength(81);
    const given = screen.getByRole("button", { name: /row 1, column 1, given 5/i });
    await user.click(given);
    await user.keyboard("9");
    expect(given).toHaveTextContent("5");
    expect(given).toHaveAttribute("aria-readonly", "true");
    expect(container.querySelector("canvas")).toBeNull();
    expect(container.querySelector(".workspace-layout")).toBeInTheDocument();
    expect(container.querySelector(".lesson-rail")).toBeInTheDocument();
    expect(container.querySelector(".timeline-dock")).toBeInTheDocument();
  });

  it("supports keyboard notes plus instant mode and language switching", async () => {
    const user = userEvent.setup();
    render(<ReasoningWorkspace initialPuzzle={"0".repeat(81)} coreClient={fixtureClient()} initialLanguage="en" />);
    const first = screen.getByRole("button", { name: /row 1, column 1, empty/i });
    await user.click(first);
    await user.keyboard("n8");
    expect(first).toHaveAccessibleDescription(/candidate 8/i);
    await user.selectOptions(screen.getByRole("combobox", { name: /solver mode/i }), "human");
    expect(screen.getByRole("combobox", { name: /solver mode/i })).toHaveValue("human");
    await user.click(screen.getByRole("button", { name: /中文/ }));
    expect(screen.getByRole("heading", { name: "数独推理雷达" })).toBeInTheDocument();
  });

  it("derives the bilingual three-part lesson and deterministic constellation from one step", () => {
    const advanced = { ...fixtureStep, technique: "x_wing" as const };
    const { container } = render(
      <ReasoningWorkspace
        initialPuzzle={puzzle}
        coreClient={fixtureClient()}
        initialLanguage="en"
        initialTrace={[advanced]}
      />,
    );
    const lesson = screen.getByTestId("lesson-card");
    expect(within(lesson).getByText("Observe")).toBeInTheDocument();
    expect(within(lesson).getByText("Eliminate")).toBeInTheDocument();
    expect(within(lesson).getByText("Conclude")).toBeInTheDocument();
    expect(within(lesson).getByText("X-Wing")).toBeInTheDocument();
    expect(within(lesson).getByText("X 翼")).toBeInTheDocument();
    expect(screen.getByRole("img", { name: /logic constellation/i })).toBeInTheDocument();
    expect(container.querySelectorAll("[data-node-id]")).toHaveLength(2);
    expect(container.querySelector(".evidence-edge--conflicts")).toBeInTheDocument();
  });

  it("keeps the constellation collapsed for basic steps and provides searchable disabled commands", async () => {
    const user = userEvent.setup();
    render(
      <ReasoningWorkspace
        initialPuzzle={puzzle}
        coreClient={fixtureClient()}
        initialLanguage="en"
        initialTrace={[fixtureStep]}
      />,
    );
    expect(screen.queryByRole("img", { name: /logic constellation/i })).not.toBeInTheDocument();
    await user.keyboard("/");
    const panel = screen.getByRole("dialog", { name: /command panel/i });
    await user.type(within(panel).getByRole("searchbox"), "OCR");
    const ocr = within(panel).getByRole("button", { name: /OCR/i });
    expect(ocr).toBeDisabled();
    expect(within(panel).getByText(/available in the Windows shell/i)).toBeInTheDocument();
  });

  it("exposes live announcements, high contrast, visible focus and reduced-motion state", async () => {
    const user = userEvent.setup();
    const { container } = render(
      <ReasoningWorkspace initialPuzzle={puzzle} coreClient={fixtureClient()} initialLanguage="en" />,
    );
    expect(screen.getByRole("status")).toHaveAttribute("aria-live", "polite");
    expect(container.firstElementChild).toHaveClass("reduced-motion");
    await user.click(screen.getByRole("checkbox", { name: /high contrast/i }));
    expect(container.firstElementChild).toHaveClass("high-contrast");
    expect(container.querySelector("[data-electron-only]")).toBeNull();
  });
});

describe("core recovery", () => {
  it("loads a fixture trace through CoreClient solve", async () => {
    const user = userEvent.setup();
    render(<ReasoningWorkspace initialPuzzle={puzzle} coreClient={fixtureClient()} initialLanguage="en" />);
    await user.keyboard("/");
    await user.click(screen.getByRole("button", { name: /analyze puzzle/i }));
    expect(await screen.findByText(/trace ready/i)).toBeInTheDocument();
    expect(screen.getByTestId("lesson-card")).toBeInTheDocument();
  });

  it("shows a recoverable invalid-response error and retries", async () => {
    const user = userEvent.setup();
    let calls = 0;
    const client = new CoreClient(() => {
      calls += 1;
      return calls === 1 ? "not-json" : JSON.stringify({
        schemaVersion: 1,
        operation: "solve",
        ok: false,
        error: { code: "retry_fixture", path: "$.puzzle", params: {} },
      });
    });
    render(<ReasoningWorkspace initialPuzzle={puzzle} coreClient={client} initialLanguage="en" />);
    await user.keyboard("/");
    await user.click(screen.getByRole("button", { name: /analyze puzzle/i }));
    expect(await screen.findByRole("alert")).toHaveTextContent(/could not read the core response/i);
    await user.click(screen.getByRole("button", { name: /retry/i }));
    expect(calls).toBe(2);
  });

  it("offers retry when the real WASM loader fails", async () => {
    const loader = vi.fn()
      .mockRejectedValueOnce(new Error("offline"))
      .mockResolvedValueOnce(fixtureClient());
    const user = userEvent.setup();
    render(<CoreShell loadClient={loader} initialPuzzle={puzzle} />);
    expect(await screen.findByRole("alert")).toHaveTextContent(/core could not start/i);
    await user.click(screen.getByRole("button", { name: /retry/i }));
    expect(await screen.findByRole("heading", { name: /sudoku reasoning radar/i })).toBeInTheDocument();
    expect(loader).toHaveBeenCalledTimes(2);
  });
});
