import { fireEvent, render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { expect, it, vi } from "vitest";
import {
  ImageReviewDialog,
  MAX_IMAGE_BYTES,
  validateImageFile,
} from "../src/components/ImageReviewDialog.tsx";
import { ReasoningWorkspace } from "../src/app.tsx";
import { fixtureClient } from "./fixtures.ts";

it("validates image type and size and treats an empty selection as cancel", async () => {
  expect(() => validateImageFile(new File(["x"], "grid.txt", { type: "text/plain" }))).toThrow(/type/i);
  expect(() => validateImageFile(new File([new Uint8Array(MAX_IMAGE_BYTES + 1)], "grid.png", { type: "image/png" }))).toThrow(/size/i);

  const adapter = { extract: vi.fn().mockResolvedValue(Array(81).fill(0)) };
  render(<ImageReviewDialog adapter={adapter} onApply={vi.fn()} onClose={vi.fn()} />);
  fireEvent.change(screen.getByLabelText(/select sudoku image/i), { target: { files: [] } });
  expect(adapter.extract).not.toHaveBeenCalled();
});

it("creates and cleans a safe preview and exposes 81 editable review cells", async () => {
  const createObjectURL = vi.spyOn(URL, "createObjectURL").mockReturnValue("blob:safe-preview");
  const revokeObjectURL = vi.spyOn(URL, "revokeObjectURL").mockImplementation(() => undefined);
  const adapter = { extract: vi.fn().mockResolvedValue(Array(81).fill(0)) };
  const { unmount } = render(<ImageReviewDialog adapter={adapter} onApply={vi.fn()} onClose={vi.fn()} />);

  expect(screen.getAllByLabelText(/image row \d, column \d/i)).toHaveLength(81);
  await userEvent.upload(screen.getByLabelText(/select sudoku image/i), new File(["png"], "grid.png", { type: "image/png" }));
  expect(await screen.findByRole("img", { name: /sudoku image preview/i })).toHaveAttribute("src", "blob:safe-preview");
  expect(adapter.extract).toHaveBeenCalledOnce();

  unmount();
  expect(revokeObjectURL).toHaveBeenCalledWith("blob:safe-preview");
  createObjectURL.mockRestore();
  revokeObjectURL.mockRestore();
});

it("blocks Sudoku conflicts and applies a reviewed 81-cell puzzle", async () => {
  const user = userEvent.setup();
  const onApply = vi.fn();
  render(<ImageReviewDialog adapter={{ extract: async () => Array(81).fill(0) }} onApply={onApply} onClose={vi.fn()} />);
  const cells = screen.getAllByLabelText(/image row \d, column \d/i);
  await user.type(cells[0]!, "5");
  await user.type(cells[1]!, "5");
  expect(screen.getByRole("alert")).toHaveTextContent(/conflict/i);
  expect(screen.getByRole("button", { name: /apply reviewed puzzle/i })).toBeDisabled();

  await user.clear(cells[1]!);
  await user.type(cells[1]!, "6");
  await user.click(screen.getByRole("button", { name: /apply reviewed puzzle/i }));
  expect(onApply).toHaveBeenCalledWith(`56${"0".repeat(79)}`);
});

it("opens image review from the Web command panel and applies reviewed givens", async () => {
  const user = userEvent.setup();
  render(<ReasoningWorkspace initialPuzzle={"0".repeat(81)} coreClient={fixtureClient()} initialLanguage="en" />);
  await user.keyboard("/");
  await user.click(screen.getByRole("button", { name: /import puzzle image/i }));
  const dialog = screen.getByRole("dialog", { name: /review puzzle image/i });
  await user.type(screen.getByLabelText(/image row 1, column 1/i), "9");
  await user.click(screen.getByRole("button", { name: /apply reviewed puzzle/i }));
  expect(dialog).not.toBeInTheDocument();
  expect(screen.getByRole("button", { name: /row 1, column 1, given 9/i })).toBeInTheDocument();
});
