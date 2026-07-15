import { render, screen, waitFor, within } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { beforeEach, expect, it, vi } from "vitest";
import { CoreShell, ReasoningWorkspace } from "../src/app.tsx";
import { fixtureClient } from "./fixtures.ts";

const empty = "0".repeat(81);
const imported = `1${"0".repeat(80)}`;

function fakeStorage(overrides: Record<string, unknown> = {}) {
  return {
    loadCurrentSession: vi.fn().mockResolvedValue(undefined),
    saveCurrentSession: vi.fn().mockResolvedValue(undefined),
    listPuzzles: vi.fn().mockResolvedValue([]),
    getPuzzle: vi.fn().mockResolvedValue(undefined),
    upsertPuzzle: vi.fn().mockResolvedValue(undefined),
    importPuzzles: vi.fn().mockResolvedValue(undefined),
    setSettings: vi.fn().mockResolvedValue(undefined),
    getSettings: vi.fn().mockResolvedValue(undefined),
    listSessions: vi.fn().mockResolvedValue([]),
    listSettings: vi.fn().mockResolvedValue([]),
    importBackup: vi.fn().mockResolvedValue(undefined),
    exportBackup: vi.fn().mockResolvedValue("{\"schemaVersion\":1}"),
    close: vi.fn(),
    ...overrides,
  };
}

beforeEach(() => {
  vi.stubGlobal("matchMedia", (query: string) => ({
    matches: false, media: query, onchange: null,
    addEventListener: vi.fn(), removeEventListener: vi.fn(), addListener: vi.fn(), removeListener: vi.fn(), dispatchEvent: vi.fn(),
  }));
});

it("restores the current session from an injected storage service on startup", async () => {
  const values = Array(81).fill(0);
  values[0] = 9;
  const storageService = fakeStorage({
    loadCurrentSession: vi.fn().mockResolvedValue({
      id: "current",
      puzzle: empty,
      values,
      noteMasks: Array(81).fill(0),
      mode: "smart",
      trace: [],
      currentStep: 0,
      elapsedMs: 25,
      savedAt: "2026-07-15T10:00:00.000Z",
    }),
  });

  render(
    <ReasoningWorkspace
      initialPuzzle={empty}
      coreClient={fixtureClient()}
      initialLanguage="en"
      storageService={storageService}
    />,
  );

  expect(await screen.findByRole("button", { name: /row 1, column 1, entered 9/i })).toBeInTheDocument();
  expect(storageService.loadCurrentSession).toHaveBeenCalledOnce();
});

it("debounces persistence after a meaningful board edit", async () => {
  const user = userEvent.setup();
  const storageService = fakeStorage();
  render(
    <ReasoningWorkspace
      initialPuzzle={empty}
      coreClient={fixtureClient()}
      initialLanguage="en"
      storageService={storageService}
      saveDelayMs={20}
    />,
  );
  await user.click(screen.getByRole("button", { name: /row 1, column 1, empty/i }));
  await user.keyboard("5");

  await waitFor(() => expect(storageService.saveCurrentSession).toHaveBeenCalledOnce());
  expect(storageService.saveCurrentSession.mock.calls[0]?.[0]).toMatchObject({
    id: "current",
    puzzle: empty,
    values: [5, ...Array(80).fill(0)],
  });
});

it("saves the puzzle and opens the library to load a saved puzzle", async () => {
  const user = userEvent.setup();
  const storageService = fakeStorage({
    listPuzzles: vi.fn().mockResolvedValue([{ puzzle: imported, name: "Library puzzle", difficulty: "easy" }]),
  });
  render(<ReasoningWorkspace initialPuzzle={empty} coreClient={fixtureClient()} initialLanguage="en" storageService={storageService} />);

  await user.keyboard("/");
  await user.click(screen.getByRole("button", { name: /^save puzzle$/i }));
  await waitFor(() => expect(storageService.upsertPuzzle).toHaveBeenCalledOnce());

  await user.keyboard("/");
  await user.click(screen.getByRole("button", { name: /open puzzle library/i }));
  const library = await screen.findByRole("dialog", { name: /puzzle library/i });
  await user.click(within(library).getByRole("button", { name: /library puzzle/i }));
  expect(screen.getByRole("button", { name: /row 1, column 1, given 1/i })).toBeInTheDocument();
});

it("accepts raw 81-character puzzle text and imports backups through scoped file actions", async () => {
  const user = userEvent.setup();
  const storageService = fakeStorage();
  const fileActions = {
    pickText: vi.fn().mockResolvedValue("{\"schemaVersion\":1}"),
    downloadText: vi.fn(),
  };
  render(<ReasoningWorkspace initialPuzzle={empty} coreClient={fixtureClient()} initialLanguage="en" storageService={storageService} fileActions={fileActions} />);

  await user.keyboard("/");
  await user.click(screen.getByRole("button", { name: /^import puzzle$/i }));
  const rawDialog = screen.getByRole("dialog", { name: /import puzzle text/i });
  await user.type(within(rawDialog).getByRole("textbox"), imported);
  await user.click(within(rawDialog).getByRole("button", { name: /apply puzzle/i }));
  expect(screen.getByRole("button", { name: /row 1, column 1, given 1/i })).toBeInTheDocument();

  await user.keyboard("/");
  await user.click(screen.getByRole("button", { name: /import backup/i }));
  await waitFor(() => expect(storageService.importBackup).toHaveBeenCalledWith("{\"schemaVersion\":1}"));

  await user.keyboard("/");
  await user.click(screen.getByRole("button", { name: /export backup/i }));
  await waitFor(() => expect(fileActions.downloadText).toHaveBeenCalledWith(
    "sudoku-reasoning-radar.srr.json", "{\"schemaVersion\":1}", "application/json",
  ));
});

it("keeps the board usable and reports a recoverable storage failure", async () => {
  const user = userEvent.setup();
  render(<ReasoningWorkspace initialPuzzle={empty} coreClient={fixtureClient()} initialLanguage="en" storageError="IndexedDB unavailable" />);

  expect(screen.getByRole("alert")).toHaveTextContent(/local storage is unavailable/i);
  const first = screen.getByRole("button", { name: /row 1, column 1, empty/i });
  await user.click(first);
  await user.keyboard("7");
  expect(first).toHaveTextContent("7");
});

it("integrates Web first-run migration and storage failure fallback in CoreShell", async () => {
  const migratedStorage = fakeStorage();
  const { unmount } = render(<CoreShell
    initialPuzzle={empty}
    loadClient={async () => fixtureClient()}
    loadStorage={async () => ({
      service: migratedStorage,
      migrationReport: { imported: 1, skipped: false, errors: [] },
    })}
  />);
  expect(await screen.findByRole("status")).toHaveTextContent(/legacy puzzle imported/i);
  unmount();

  render(<CoreShell
    initialPuzzle={empty}
    loadClient={async () => fixtureClient()}
    loadStorage={async () => { throw new Error("blocked"); }}
  />);
  expect(await screen.findByRole("alert")).toHaveTextContent(/local storage is unavailable/i);
  expect(screen.getAllByRole("gridcell")).toHaveLength(81);
});
