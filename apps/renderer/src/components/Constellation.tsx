import type { EvidenceNode, SolveStepV1 } from "@srr/core-client";
import { translate } from "../i18n";
import type { Language } from "../session";

function hash(value: string): number {
  return [...value].reduce((total, character) => ((total * 31) + character.charCodeAt(0)) >>> 0, 17);
}

function position(node: EvidenceNode) {
  const angle = ((hash(node.id) % 3600) / 3600) * Math.PI * 2;
  const radius = 72 + (hash(`${node.id}:radius`) % 44);
  return { x: 150 + Math.cos(angle) * radius, y: 120 + Math.sin(angle) * radius };
}

function NodeShape({ node, x, y }: { node: EvidenceNode; x: number; y: number }) {
  if (node.kind === "candidate") return <polygon points={`${x},${y - 9} ${x + 9},${y + 8} ${x - 9},${y + 8}`} />;
  if (node.kind === "unit") return <rect x={x - 9} y={y - 9} width="18" height="18" rx="2" />;
  if (node.kind === "branch") return <path d={`M ${x} ${y - 10} L ${x + 10} ${y} L ${x} ${y + 10} L ${x - 10} ${y} Z`} />;
  return <circle cx={x} cy={y} r="9" />;
}

export function isAdvancedStep(step?: SolveStepV1): boolean {
  if (!step) return false;
  return ["naked_pair", "hidden_pair", "x_wing", "mrv_guess", "exact_cover"].includes(step.technique ?? "")
    || ["guess", "contradiction", "backtrack"].includes(step.action)
    || step.branch.depth > 0;
}

export function Constellation({ step, language }: { step?: SolveStepV1; language: Language }) {
  if (!step) return null;
  const points = new Map(step.evidence.nodes.map((node) => [node.id, position(node)]));
  return (
    <svg className="constellation" viewBox="0 0 300 240" role="img" aria-label={translate(language, "constellation")}>
      <g className="constellation-edges">
        {step.evidence.edges.map((edge, index) => {
          const from = points.get(edge.from);
          const to = points.get(edge.to);
          if (!from || !to) return null;
          return <line key={`${edge.from}-${edge.to}-${index}`} x1={from.x} y1={from.y} x2={to.x} y2={to.y} className={`evidence-edge evidence-edge--${edge.relation}`} />;
        })}
      </g>
      <g className="constellation-nodes">
        {step.evidence.nodes.map((node) => {
          const point = points.get(node.id)!;
          return <g key={node.id} data-node-id={node.id} data-x={point.x} data-y={point.y} className={`evidence-node evidence-node--${node.kind}`}><NodeShape node={node} {...point} /></g>;
        })}
      </g>
    </svg>
  );
}
