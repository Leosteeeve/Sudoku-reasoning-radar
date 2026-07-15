#include "StepRecorder.h"

void StepRecorder::addStep(StepType type,
                           int row,
                           int col,
                           int number,
                           int relatedRow,
                           int relatedCol,
                           int depth,
                           const std::string& reason) {
    addStep(type,
            row,
            col,
            number,
            relatedRow,
            relatedCol,
            -1,
            -1,
            UnitType::None,
            -1,
            0,
            0,
            0,
            depth,
            reason);
}

void StepRecorder::addStep(StepType type,
                           int row,
                           int col,
                           int number,
                           int relatedRow,
                           int relatedCol,
                           int row2,
                           int col2,
                           UnitType unitType,
                           int unitIndex,
                           int maskBefore,
                           int maskAfter,
                           int removedMask,
                           int depth,
                           const std::string& reason,
                           StepType sourceTechnique) {
    SolveStep step;
    step.type = type;
    step.sourceTechnique = type == StepType::CandidateRemovedByLogic ? sourceTechnique : type;
    step.row = row;
    step.col = col;
    step.number = number;
    step.relatedRow = relatedRow;
    step.relatedCol = relatedCol;
    step.row2 = row2;
    step.col2 = col2;
    step.unitType = static_cast<int>(unitType);
    step.unitIndex = unitIndex;
    step.maskBefore = maskBefore;
    step.maskAfter = maskAfter;
    step.removedMask = removedMask;
    step.depth = depth;
    step.reason = reason;
    steps.push_back(step);
}

void StepRecorder::addStep(const SolveStep& step) {
    steps.push_back(step);
}

void StepRecorder::clear() {
    steps.clear();
}

const std::vector<SolveStep>& StepRecorder::getSteps() const {
    return steps;
}
