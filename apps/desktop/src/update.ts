const CACHE_DURATION_MS = 24 * 60 * 60 * 1000;

export interface UpdateSuccess {
  version: 1;
  status: "ok";
  checkedAt: string;
  available: boolean;
  releaseUrl?: string;
}

export interface UpdateError {
  version: 1;
  status: "error";
  code: string;
}

export type UpdateResult = UpdateSuccess | UpdateError;

export interface UpdateCache {
  checkedAtMs: number;
  response: UpdateResult;
}

export interface UpdateCheckerDependencies {
  currentVersion: string;
  now(): number;
  readCache(): Promise<UpdateCache | undefined>;
  writeCache(cache: UpdateCache): Promise<void>;
  fetchLatest(): Promise<unknown>;
}

function release(value: unknown): { tagName: string; url: string } {
  if (value === null || typeof value !== "object" || Array.isArray(value)) throw new Error("release response must be an object");
  const input = value as Record<string, unknown>;
  if (Object.keys(input).some((key) => !["tag_name", "html_url"].includes(key))) throw new Error("release response has unknown fields");
  if (typeof input.tag_name !== "string" || input.tag_name.length === 0) throw new Error("release tag is required");
  if (typeof input.html_url !== "string") throw new Error("release URL is required");
  const url = new URL(input.html_url);
  if (url.protocol !== "https:") throw new Error("unsafe-url");
  return { tagName: input.tag_name.replace(/^v/, ""), url: url.toString() };
}

interface SemanticVersion {
  core: [number, number, number];
  prerelease: Array<number | string>;
}

function semanticVersion(value: string): SemanticVersion | undefined {
  const match = /^v?(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.-]+))?$/.exec(value);
  if (!match) return undefined;
  return {
    core: [Number(match[1]), Number(match[2]), Number(match[3])],
    prerelease: match[4]?.split(".").map((part) => /^\d+$/.test(part) ? Number(part) : part) ?? [],
  };
}

function newer(candidateValue: string, currentValue: string): boolean {
  const candidate = semanticVersion(candidateValue);
  const current = semanticVersion(currentValue);
  if (!candidate || !current) return false;
  for (let index = 0; index < candidate.core.length; index += 1) {
    if (candidate.core[index] !== current.core[index]) return candidate.core[index]! > current.core[index]!;
  }
  if (candidate.prerelease.length === 0 || current.prerelease.length === 0) {
    return candidate.prerelease.length === 0 && current.prerelease.length > 0;
  }
  for (let index = 0; index < Math.max(candidate.prerelease.length, current.prerelease.length); index += 1) {
    const a = candidate.prerelease[index];
    const b = current.prerelease[index];
    if (a === b) continue;
    if (a === undefined) return false;
    if (b === undefined) return true;
    if (typeof a === "number" && typeof b === "number") return a > b;
    if (typeof a === "number") return false;
    if (typeof b === "number") return true;
    return a.localeCompare(b) > 0;
  }
  return false;
}

export function createUpdateChecker(dependencies: UpdateCheckerDependencies): () => Promise<UpdateResult> {
  let inFlight: Promise<UpdateResult> | undefined;
  return async () => {
    const now = dependencies.now();
    const cache = await dependencies.readCache();
    if (cache && now - cache.checkedAtMs < CACHE_DURATION_MS) return cache.response;
    if (inFlight) return inFlight;
    const attempt = (async (): Promise<UpdateResult> => {
      let response: UpdateResult;
      try {
        const latest = release(await dependencies.fetchLatest());
        const available = newer(latest.tagName, dependencies.currentVersion);
        response = {
          version: 1,
          status: "ok",
          checkedAt: new Date(now).toISOString(),
          available,
          ...(available ? { releaseUrl: latest.url } : {}),
        };
      } catch (error) {
        response = { version: 1, status: "error", code: error instanceof Error && error.message === "unsafe-url" ? "unsafe-url" : "update-check-failed" };
      }
      await dependencies.writeCache({ checkedAtMs: now, response });
      return response;
    })();
    inFlight = attempt;
    try { return await attempt; }
    finally { if (inFlight === attempt) inFlight = undefined; }
  };
}
