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
    MrvGuess,
    ExactCover,
};

enum class TraceAction {
    Analyze,
    Place,
    Eliminate,
    Guess,
    Contradiction,
    Backtrack,
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
    std::vector<int> removedDigits;
};

struct EvidenceUnit {
    std::string kind;
    int index = -1;
};

struct EvidenceNode {
    std::string id;
    std::string kind;
    std::optional<TraceTarget> cell;
    std::optional<int> digit;
    std::optional<EvidenceUnit> unit;
};

enum class EvidenceRelation {
    Supports,
    Excludes,
    Conflicts,
    BranchesTo,
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
    std::optional<std::string> parentStepId;
};

using TraceParameter = std::variant<int, std::string>;

struct TraceStep {
    std::string id;
    std::optional<TechniqueId> technique;
    TraceAction action = TraceAction::Analyze;
    std::vector<TraceTarget> targets;
    std::vector<CandidateDelta> candidateDeltas;
    EvidenceGraph evidence;
    BranchMetadata branch;
    std::string explanationKey;
    std::map<std::string, TraceParameter> explanationParams;
};

std::vector<TraceStep> adaptLegacySteps(const std::vector<SolveStep>& steps);
std::string serializeTrace(const std::vector<TraceStep>& steps);
std::optional<TechniqueId> techniqueForStep(StepType type);
std::string techniqueIdName(TechniqueId technique);
std::string traceActionName(TraceAction action);

}  // namespace srr::v1
