#include "SolveTrace.h"

#include "Board.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace srr::v1 {
namespace {

std::string jsonString(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(ch) << std::dec;
            } else {
                out << static_cast<char>(ch);
            }
        }
    }
    out << '"';
    return out.str();
}

std::string stableId(size_t index) {
    std::ostringstream out;
    out << "step-" << std::setw(6) << std::setfill('0') << (index + 1);
    return out.str();
}

TraceAction actionForStep(StepType type) {
    switch (type) {
    case StepType::AnalyzeCell:
    case StepType::ModeChanged: return TraceAction::Analyze;
    case StepType::RemoveCandidate:
    case StepType::LockedCandidate:
    case StepType::BoxLineReduction:
    case StepType::NakedPair:
    case StepType::HiddenPair:
    case StepType::XWing:
    case StepType::CandidateRemovedByLogic: return TraceAction::Eliminate;
    case StepType::NakedSingle:
    case StepType::HiddenSingle:
    case StepType::PlaceNumber: return TraceAction::Place;
    case StepType::Guess: return TraceAction::Guess;
    case StepType::Contradiction:
    case StepType::InvalidInput: return TraceAction::Contradiction;
    case StepType::Backtrack: return TraceAction::Backtrack;
    case StepType::Solved:
    case StepType::NoSolution:
    case StepType::MultipleSolutions:
    case StepType::TurboSolved: return TraceAction::Complete;
    }
    return TraceAction::Analyze;
}

std::string explanationForStep(StepType type) {
    switch (type) {
    case StepType::AnalyzeCell: return "trace.analyzeCell";
    case StepType::RemoveCandidate: return "trace.removeCandidate";
    case StepType::NakedSingle: return "trace.nakedSingle";
    case StepType::HiddenSingle: return "trace.hiddenSingle";
    case StepType::LockedCandidate: return "trace.lockedCandidate";
    case StepType::BoxLineReduction: return "trace.boxLineReduction";
    case StepType::NakedPair: return "trace.nakedPair";
    case StepType::HiddenPair: return "trace.hiddenPair";
    case StepType::XWing: return "trace.xWing";
    case StepType::CandidateRemovedByLogic: return "trace.candidateRemovedByLogic";
    case StepType::ModeChanged: return "trace.modeChanged";
    case StepType::Guess: return "trace.guess";
    case StepType::PlaceNumber: return "trace.placeNumber";
    case StepType::Contradiction: return "trace.contradiction";
    case StepType::Backtrack: return "trace.backtrack";
    case StepType::Solved: return "trace.solved";
    case StepType::NoSolution: return "trace.noSolution";
    case StepType::MultipleSolutions: return "trace.multipleSolutions";
    case StepType::InvalidInput: return "trace.invalidInput";
    case StepType::TurboSolved: return "trace.turboSolved";
    }
    return "trace.unknown";
}

EvidenceRelation relationForAction(TraceAction action) {
    switch (action) {
    case TraceAction::Eliminate: return EvidenceRelation::Excludes;
    case TraceAction::Contradiction: return EvidenceRelation::Conflicts;
    case TraceAction::Guess: return EvidenceRelation::BranchesTo;
    case TraceAction::Backtrack: return EvidenceRelation::Reverts;
    default: return EvidenceRelation::Supports;
    }
}

std::string relationName(EvidenceRelation relation) {
    switch (relation) {
    case EvidenceRelation::Supports: return "supports";
    case EvidenceRelation::Excludes: return "excludes";
    case EvidenceRelation::Conflicts: return "conflicts";
    case EvidenceRelation::BranchesTo: return "branches_to";
    case EvidenceRelation::Reverts: return "reverts";
    }
    return "supports";
}

std::string unitName(int unitType) {
    switch (static_cast<UnitType>(unitType)) {
    case UnitType::Row: return "row";
    case UnitType::Column: return "column";
    case UnitType::Box: return "box";
    case UnitType::None: return "none";
    }
    return "none";
}

void addTarget(std::vector<TraceTarget>& targets, int row, int col) {
    if (!Board::isInside(row, col)) {
        return;
    }
    const auto duplicate = std::find_if(targets.begin(), targets.end(), [=](const TraceTarget& target) {
        return target.row == row && target.col == col;
    });
    if (duplicate == targets.end()) {
        targets.push_back(TraceTarget{row, col});
    }
}

void serializeTarget(std::ostringstream& out, const TraceTarget& target) {
    out << "{\"row\":" << target.row << ",\"col\":" << target.col << '}';
}

}  // namespace

std::optional<TechniqueId> techniqueForStep(StepType type) {
    switch (type) {
    case StepType::NakedSingle: return TechniqueId::NakedSingle;
    case StepType::HiddenSingle: return TechniqueId::HiddenSingle;
    case StepType::LockedCandidate: return TechniqueId::LockedCandidate;
    case StepType::BoxLineReduction: return TechniqueId::BoxLineReduction;
    case StepType::NakedPair: return TechniqueId::NakedPair;
    case StepType::HiddenPair: return TechniqueId::HiddenPair;
    case StepType::XWing: return TechniqueId::XWing;
    case StepType::Guess: return TechniqueId::MrvGuess;
    case StepType::TurboSolved: return TechniqueId::ExactCover;
    case StepType::CandidateRemovedByLogic: return std::nullopt;
    default: return std::nullopt;
    }
}

std::string techniqueIdName(TechniqueId technique) {
    switch (technique) {
    case TechniqueId::NakedSingle: return "naked_single";
    case TechniqueId::HiddenSingle: return "hidden_single";
    case TechniqueId::LockedCandidate: return "locked_candidate";
    case TechniqueId::BoxLineReduction: return "box_line_reduction";
    case TechniqueId::NakedPair: return "naked_pair";
    case TechniqueId::HiddenPair: return "hidden_pair";
    case TechniqueId::XWing: return "x_wing";
    case TechniqueId::MrvGuess: return "mrv_guess";
    case TechniqueId::ExactCover: return "exact_cover";
    }
    return "naked_single";
}

std::string traceActionName(TraceAction action) {
    switch (action) {
    case TraceAction::Analyze: return "analyze";
    case TraceAction::Place: return "place";
    case TraceAction::Eliminate: return "eliminate";
    case TraceAction::Guess: return "guess";
    case TraceAction::Contradiction: return "contradiction";
    case TraceAction::Backtrack: return "backtrack";
    case TraceAction::Complete: return "complete";
    }
    return "analyze";
}

std::vector<TraceStep> adaptLegacySteps(const std::vector<SolveStep>& steps) {
    std::vector<TraceStep> mapped;
    mapped.reserve(steps.size());
    std::map<int, std::string> activeBranches;

    for (size_t index = 0; index < steps.size(); ++index) {
        const SolveStep& legacy = steps[index];
        TraceStep current;
        current.id = stableId(index);
        current.technique = legacy.type == StepType::CandidateRemovedByLogic
            ? techniqueForStep(legacy.sourceTechnique)
            : techniqueForStep(legacy.type);
        current.action = actionForStep(legacy.type);
        current.explanationKey = explanationForStep(legacy.type);
        current.branch.depth = std::max(0, legacy.depth);

        if (current.action == TraceAction::Guess) {
            const auto parent = activeBranches.find(current.branch.depth - 1);
            if (parent != activeBranches.end()) {
                current.branch.parentStepId = parent->second;
            }
            activeBranches[current.branch.depth] = current.id;
        } else {
            const auto parent = activeBranches.find(current.branch.depth);
            if (parent != activeBranches.end()) {
                current.branch.parentStepId = parent->second;
            }
        }

        addTarget(current.targets, legacy.row, legacy.col);
        addTarget(current.targets, legacy.relatedRow, legacy.relatedCol);
        addTarget(current.targets, legacy.row2, legacy.col2);

        if ((legacy.maskBefore | legacy.maskAfter | legacy.removedMask) != 0
            && Board::isInside(legacy.row, legacy.col)) {
            CandidateDelta delta;
            delta.cell = TraceTarget{legacy.row, legacy.col};
            delta.beforeMask = legacy.maskBefore;
            delta.afterMask = legacy.maskAfter;
            for (int digit = 1; digit <= 9; ++digit) {
                if ((legacy.removedMask & Board::bitForNumber(digit)) != 0) {
                    delta.removedDigits.push_back(digit);
                }
            }
            current.candidateDeltas.push_back(std::move(delta));
        }

        if (legacy.number != 0) current.explanationParams["number"] = legacy.number;
        if (legacy.unitType != static_cast<int>(UnitType::None)) {
            current.explanationParams["unit"] = unitName(legacy.unitType);
        }
        if (legacy.unitIndex >= 0) current.explanationParams["unitIndex"] = legacy.unitIndex;

        EvidenceNode branchNode;
        branchNode.id = current.id;
        branchNode.kind = "branch";
        current.evidence.nodes.push_back(branchNode);

        if ((current.action == TraceAction::Guess || current.action == TraceAction::Backtrack)
            && current.branch.parentStepId) {
            EvidenceNode parentNode;
            parentNode.id = *current.branch.parentStepId;
            parentNode.kind = "branch";
            current.evidence.nodes.push_back(parentNode);
            current.evidence.edges.push_back(EvidenceEdge{
                *current.branch.parentStepId,
                current.id,
                relationForAction(current.action),
            });
        }

        if (current.targets.empty()) {
            EvidenceNode resultNode;
            resultNode.id = current.id + "-result";
            resultNode.kind = "branch";
            current.evidence.nodes.push_back(resultNode);
            current.evidence.edges.push_back(
                EvidenceEdge{current.id, resultNode.id, relationForAction(current.action)});
        } else {
            for (size_t targetIndex = 0; targetIndex < current.targets.size(); ++targetIndex) {
                const TraceTarget& target = current.targets[targetIndex];
                const std::string cellId = current.id + "-cell-" + std::to_string(targetIndex + 1);
                EvidenceNode node;
                node.id = cellId;
                node.kind = "cell";
                node.cell = target;
                if (legacy.number != 0) node.digit = legacy.number;
                current.evidence.nodes.push_back(node);
                current.evidence.edges.push_back(
                    EvidenceEdge{current.id, cellId, relationForAction(current.action)});
            }
        }

        if (legacy.number != 0 && !current.targets.empty()) {
            EvidenceNode candidateNode;
            candidateNode.id = current.id + "-candidate";
            candidateNode.kind = "candidate";
            candidateNode.cell = current.targets.front();
            candidateNode.digit = legacy.number;
            current.evidence.nodes.push_back(candidateNode);
            current.evidence.edges.push_back(EvidenceEdge{
                current.id,
                candidateNode.id,
                relationForAction(current.action),
            });
        }

        if (legacy.unitType != static_cast<int>(UnitType::None) && legacy.unitIndex >= 0) {
            EvidenceNode unitNode;
            unitNode.id = current.id + "-unit";
            unitNode.kind = "unit";
            unitNode.unit = EvidenceUnit{unitName(legacy.unitType), legacy.unitIndex};
            current.evidence.nodes.push_back(unitNode);
            current.evidence.edges.push_back(EvidenceEdge{
                current.id,
                unitNode.id,
                relationForAction(current.action),
            });
        }

        mapped.push_back(std::move(current));
        if (legacy.type == StepType::Backtrack) {
            for (auto it = activeBranches.begin(); it != activeBranches.end();) {
                if (it->first >= legacy.depth) it = activeBranches.erase(it);
                else ++it;
            }
        }
    }
    return mapped;
}

std::string serializeTrace(const std::vector<TraceStep>& steps) {
    std::ostringstream out;
    out << '[';
    for (size_t i = 0; i < steps.size(); ++i) {
        if (i != 0) out << ',';
        const TraceStep& step = steps[i];
        out << "{\"id\":" << jsonString(step.id) << ",\"technique\":";
        if (step.technique) out << jsonString(techniqueIdName(*step.technique));
        else out << "null";
        out << ",\"action\":" << jsonString(traceActionName(step.action)) << ",\"targets\":[";
        for (size_t targetIndex = 0; targetIndex < step.targets.size(); ++targetIndex) {
            if (targetIndex != 0) out << ',';
            serializeTarget(out, step.targets[targetIndex]);
        }
        out << "],\"candidateDeltas\":[";
        for (size_t deltaIndex = 0; deltaIndex < step.candidateDeltas.size(); ++deltaIndex) {
            if (deltaIndex != 0) out << ',';
            const CandidateDelta& delta = step.candidateDeltas[deltaIndex];
            out << "{\"cell\":";
            serializeTarget(out, delta.cell);
            out << ",\"beforeMask\":" << delta.beforeMask
                << ",\"afterMask\":" << delta.afterMask
                << ",\"removedDigits\":[";
            for (size_t digitIndex = 0; digitIndex < delta.removedDigits.size(); ++digitIndex) {
                if (digitIndex != 0) out << ',';
                out << delta.removedDigits[digitIndex];
            }
            out << "]}";
        }
        out << "],\"evidence\":{\"nodes\":[";
        for (size_t nodeIndex = 0; nodeIndex < step.evidence.nodes.size(); ++nodeIndex) {
            if (nodeIndex != 0) out << ',';
            const EvidenceNode& node = step.evidence.nodes[nodeIndex];
            out << "{\"id\":" << jsonString(node.id) << ",\"kind\":" << jsonString(node.kind);
            if (node.cell) {
                out << ",\"cell\":";
                serializeTarget(out, *node.cell);
            }
            if (node.digit) out << ",\"digit\":" << *node.digit;
            if (node.unit) {
                out << ",\"unit\":{\"kind\":" << jsonString(node.unit->kind)
                    << ",\"index\":" << node.unit->index << '}';
            }
            out << '}';
        }
        out << "],\"edges\":[";
        for (size_t edgeIndex = 0; edgeIndex < step.evidence.edges.size(); ++edgeIndex) {
            if (edgeIndex != 0) out << ',';
            const EvidenceEdge& edge = step.evidence.edges[edgeIndex];
            out << "{\"from\":" << jsonString(edge.from)
                << ",\"to\":" << jsonString(edge.to)
                << ",\"relation\":" << jsonString(relationName(edge.relation)) << '}';
        }
        out << "]},\"branch\":{\"depth\":" << step.branch.depth;
        if (step.branch.parentStepId) {
            out << ",\"parentStepId\":" << jsonString(*step.branch.parentStepId);
        }
        out << "},\"explanationKey\":" << jsonString(step.explanationKey)
            << ",\"explanationParams\":{";
        size_t paramIndex = 0;
        for (const auto& [key, value] : step.explanationParams) {
            if (paramIndex++ != 0) out << ',';
            out << jsonString(key) << ':';
            if (std::holds_alternative<int>(value)) out << std::get<int>(value);
            else out << jsonString(std::get<std::string>(value));
        }
        out << "}}";
    }
    out << ']';
    return out.str();
}

}  // namespace srr::v1
