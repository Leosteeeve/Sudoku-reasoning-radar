#pragma once

#include "StepRecorder.h"

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace srr::v1 {

enum class TechniqueId {
    NakedSingle,
    HiddenSingle,
    LockedCandidate,
    BoxLineReduction,
    NakedPair,
    HiddenPair,
    XWing,
};

enum class TraceAction {
    Observe,
    Eliminate,
    Place,
    Inform,
    Branch,
    Contradict,
    Revert,
    Complete,
};

struct TraceTarget {
    int row = -1;
    int col = -1;
};

struct CandidateDelta {
    TraceTarget cell;
    int beforeMask = 0;
    int afterMask = 0;
    int removedMask = 0;
};

struct EvidenceNode {
    std::string id;
    std::string kind;
    int row = -1;
    int col = -1;
    int value = 0;
    int mask = 0;
};

enum class EvidenceRelation {
    Supports,
    Excludes,
    Conflicts,
    Branches,
    Reverts,
};

struct EvidenceEdge {
    std::string from;
    std::string to;
    EvidenceRelation relation = EvidenceRelation::Supports;
};

struct EvidenceGraph {
    std::vector<EvidenceNode> nodes;
    std::vector<EvidenceEdge> edges;
};

struct BranchMetadata {
    int depth = 0;
    std::optional<std::string> parentId;
};

using TraceParameter = std::variant<int, std::string>;

struct TraceStep {
    std::string id;
    std::optional<TechniqueId> technique;
    TraceAction action = TraceAction::Observe;
    std::vector<TraceTarget> targets;
    std::vector<CandidateDelta> candidateDeltas;
    EvidenceGraph evidence;
    BranchMetadata branch;
    std::string explanationKey;
    std::map<std::string, TraceParameter> params;
};

std::vector<TraceStep> adaptLegacySteps(const std::vector<SolveStep>& steps);
std::string serializeTrace(const std::vector<TraceStep>& steps);
std::optional<TechniqueId> techniqueForStep(StepType type);
std::string techniqueIdName(TechniqueId technique);
std::string traceActionName(TraceAction action);

}  // namespace srr::v1
