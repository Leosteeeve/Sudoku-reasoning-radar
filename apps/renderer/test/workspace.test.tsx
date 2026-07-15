import { render, screen, within } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { beforeEach, describe, expect, it, vi } from "vitest";

import { CoreClient } from "@srr/core-client";
import { CoreShell, ReasoningWorkspace } from "../src/app.tsx";
import { Constellation } from "../src/components/Constellation.tsx";
import { LessonCard } from "../src/components/LessonCard.tsx";
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

  it("does not hijack native keyboard behavior on interactive targets", async () => {
    const advanced = { ...fixtureStep, id: "step-000002", technique: "x_wing" as const };
    render(
      <ReasoningWorkspace
        initialPuzzle={puzzle}
        coreClient={fixtureClient()}
        initialLanguage="en"
        initialTrace={[fixtureStep, advanced]}
      />,
    );
    const play = screen.getByRole("button", { name: "Play" });
    const space = new KeyboardEvent("keydown", { key: " ", bubbles: true, cancelable: true });
    play.dispatchEvent(space);
    expect(space.defaultPrevented).toBe(false);

    const editable = document.createElement("div");
    editable.contentEditable = "true";
    document.body.append(editable);
    editable.focus();
    const digit = new KeyboardEvent("keydown", { key: "8", bubbles: true, cancelable: true });
    editable.dispatchEvent(digit);
    expect(digit.defaultPrevented).toBe(false);
    editable.remove();
  });

  it("opens and closes labelled lesson and constellation bottom-sheet dialogs", async () => {
    vi.stubGlobal("matchMedia", (query: string) => ({
      matches: query.includes("max-width"),
      media: query,
      onchange: null,
      addEventListener: vi.fn(),
      removeEventListener: vi.fn(),
      addListener: vi.fn(),
      removeListener: vi.fn(),
      dispatchEvent: vi.fn(),
    }));
    const user = userEvent.setup();
    render(
      <ReasoningWorkspace
        initialPuzzle={puzzle}
        coreClient={fixtureClient()}
        initialLanguage="en"
        initialTrace={[fixtureStep]}
      />,
    );
    await user.click(screen.getByRole("button", { name: /open lesson sheet/i }));
    const lessonSheet = screen.getByRole("dialog", { name: /reasoning lesson/i });
    expect(lessonSheet).toHaveAttribute("aria-modal", "true");
    const lessonClose = within(lessonSheet).getByRole("button", { name: /close lesson sheet/i });
    expect(lessonClose).toHaveFocus();
    await user.keyboard("{Escape}");
    expect(screen.queryByRole("dialog", { name: /reasoning lesson/i })).not.toBeInTheDocument();
    expect(screen.getByRole("button", { name: /open lesson sheet/i })).toHaveFocus();

    await user.click(screen.getByRole("button", { name: /open constellation sheet/i }));
    const constellationSheet = screen.getByRole("dialog", { name: /logic constellation/i });
    expect(within(constellationSheet).getByRole("img", { name: /logic constellation/i })).toBeInTheDocument();
    const constellationClose = within(constellationSheet).getByRole("button", { name: /close constellation sheet/i });
    expect(constellationClose).toHaveFocus();
    await user.tab();
    expect(constellationSheet).toContainElement(document.activeElement as HTMLElement);
    await user.keyboard("{Escape}");
    expect(screen.queryByRole("dialog", { name: /logic constellation/i })).not.toBeInTheDocument();
    expect(screen.getByRole("button", { name: /open constellation sheet/i })).toHaveFocus();
  });

  it("keeps an auto-opened advanced constellation closed after the user dismisses it", async () => {
    const user = userEvent.setup();
    const advanced = { ...fixtureStep, technique: "x_wing" as const };
    render(
      <ReasoningWorkspace initialPuzzle={puzzle} coreClient={fixtureClient()} initialLanguage="en" initialTrace={[advanced]} />,
    );
    expect(await screen.findByRole("img", { name: /logic constellation/i })).toBeInTheDocument();
    await user.click(screen.getByRole("button", { name: /collapse logic constellation/i }));
    expect(screen.queryByRole("img", { name: /logic constellation/i })).not.toBeInTheDocument();
    await user.click(screen.getByRole("button", { name: /中文/ }));
    expect(screen.queryByRole("img", { name: /logic constellation/i })).not.toBeInTheDocument();
  });

  it("localizes action prose without exposing raw protocol action values", () => {
    const backtrack = { ...fixtureStep, action: "backtrack" as const, candidateDeltas: [] };
    const { rerender } = render(<LessonCard step={backtrack} language="en" />);
    expect(screen.getByTestId("lesson-card")).toHaveTextContent("Return to the last consistent branch");
    expect(screen.getByTestId("lesson-card")).not.toHaveTextContent(/backtrack/i);
    expect(screen.getByTestId("lesson-card")).not.toHaveTextContent("Remove —");
    rerender(<LessonCard step={backtrack} language="zh" />);
    expect(screen.getByTestId("lesson-card")).toHaveTextContent("返回上一个可靠分支");
  });

  it("keeps constellation node coordinates stable when evidence order changes", () => {
    const ordered = fixtureStep;
    const reversed = {
      ...fixtureStep,
      evidence: { ...fixtureStep.evidence, nodes: [...fixtureStep.evidence.nodes].reverse() },
    };
    const { container, rerender } = render(<Constellation step={ordered} language="en" />);
    const coordinates = () => new Map(
      [...container.querySelectorAll<SVGGElement>("[data-node-id]")]
        .map((node) => [node.dataset.nodeId, node.innerHTML]),
    );
    const first = coordinates();
    rerender(<Constellation step={reversed} language="en" />);
    expect(coordinates()).toEqual(first);
  });

  it("disables empty and boundary timeline controls", () => {
    const { unmount } = render(
      <ReasoningWorkspace initialPuzzle={puzzle} coreClient={fixtureClient()} initialLanguage="en" />,
    );
    expect(screen.getByRole("button", { name: /previous step/i })).toBeDisabled();
    expect(screen.getByRole("button", { name: "Play" })).toBeDisabled();
    expect(screen.getByRole("button", { name: /next step/i })).toBeDisabled();
    unmount();
    render(
      <ReasoningWorkspace initialPuzzle={puzzle} coreClient={fixtureClient()} initialLanguage="en" initialTrace={[fixtureStep]} />,
    );
    expect(screen.getByRole("button", { name: /previous step/i })).toBeDisabled();
    expect(screen.getByRole("button", { name: /next step/i })).toBeDisabled();
  });

  it("traps command focus and restores it to the trigger after Escape", async () => {
    const user = userEvent.setup();
    render(<ReasoningWorkspace initialPuzzle={puzzle} coreClient={fixtureClient()} initialLanguage="en" />);
    const trigger = screen.getByRole("button", { name: /commands/i });
    await user.click(trigger);
    const panel = screen.getByRole("dialog", { name: /command panel/i });
    expect(within(panel).getByRole("searchbox")).toHaveFocus();
    const close = within(panel).getByRole("button", { name: /close/i });
    close.focus();
    await user.tab({ shift: true });
    expect(panel).toContainElement(document.activeElement as HTMLElement);
    await user.keyboard("{Escape}");
    expect(screen.queryByRole("dialog", { name: /command panel/i })).not.toBeInTheDocument();
    expect(trigger).toHaveFocus();
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
