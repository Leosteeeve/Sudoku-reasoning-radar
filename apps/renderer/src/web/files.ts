export interface BrowserFileActions {
  pickText(accept: string): Promise<string | null>;
  downloadText(filename: string, text: string, type: string): void;
}

export function createBrowserFileActions(documentRef: Document = document): BrowserFileActions {
  return {
    pickText(accept) {
      return new Promise((resolve, reject) => {
        const input = documentRef.createElement("input");
        input.type = "file";
        input.accept = accept;
        input.hidden = true;
        const finish = () => input.remove();
        input.addEventListener("cancel", () => { finish(); resolve(null); }, { once: true });
        input.addEventListener("change", async () => {
          const file = input.files?.[0];
          finish();
          if (!file) { resolve(null); return; }
          try { resolve(await file.text()); }
          catch (error) { reject(error); }
        }, { once: true });
        documentRef.body.append(input);
        input.click();
      });
    },
    downloadText(filename, text, type) {
      const url = URL.createObjectURL(new Blob([text], { type }));
      const anchor = documentRef.createElement("a");
      anchor.href = url;
      anchor.download = filename;
      anchor.hidden = true;
      documentRef.body.append(anchor);
      anchor.click();
      anchor.remove();
      URL.revokeObjectURL(url);
    },
  };
}

export const browserFileActions = createBrowserFileActions();
