#include "CoreProtocol.h"
#include "SolveTrace.h"

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr const char* CanonicalPuzzle =
    "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
constexpr const char* InitialDuplicatePuzzle =
    "530570000600195000098000060800060003400803001700020006060000280000419005000080079";
constexpr const char* UnsolvablePuzzle =
    "531070000600195000098000060800060003400803001700020006060000280000419005000080079";
constexpr const char* MultipleSolutionPuzzle =
    "534678912672195348198342567859761423426853791713924856961537284000000000000000000";

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw TestFailure(message);
    }
}

void expectContains(const std::string& text,
                    const std::string& expected,
                    const std::string& message) {
    expect(text.find(expected) != std::string::npos,
           message + " (missing " + expected + ")\n" + text);
}

void expectNotContains(const std::string& text,
                       const std::string& unexpected,
                       const std::string& message) {
    expect(text.find(unexpected) == std::string::npos,
           message + " (found " + unexpected + ")\n" + text);
}

std::string request(const std::string& operationFields) {
    return srr::v1::dispatchJson("{\"schemaVersion\":1," + operationFields + "}");
}

void validatesVersionOperationModesTypesAndRanges() {
    expectContains(srr::v1::dispatchJson("{\"schemaVersion\":2,\"operation\":\"solve\"}"),
                   "\"code\":\"unsupported_schema_version\"",
                   "unsupported schema version was accepted");
    expectContains(request("\"operation\":\"erase-disk\""),
                   "\"code\":\"unsupported_operation\"",
                   "unsupported operation was accepted");
    expectContains(request("\"operation\":\"solve\",\"puzzle\":\"" +
                           std::string(CanonicalPuzzle) + "\",\"mode\":\"magic\""),
                   "\"code\":\"invalid_value\"",
                   "unsupported mode was accepted");
    expectContains(request("\"operation\":\"solve\",\"puzzle\":123"),
                   "\"code\":\"wrong_type\"",
                   "wrong puzzle type was accepted");
    expectContains(request("\"operation\":\"solve\",\"puzzle\":\"123\""),
                   "\"code\":\"malformed_puzzle\"",
                   "malformed puzzle was accepted");
    expectContains(request("\"operation\":\"generate\",\"difficulty\":\"easy\",\"seed\":-1"),
                   "\"code\":\"out_of_range\"",
                   "out-of-range seed was accepted");
}

void classifiesAllSolveResultsAndCanExcludeTrace() {
    const auto solve = [](const std::string& puzzle, bool includeTrace) {
        return request("\"operation\":\"solve\",\"puzzle\":\"" + puzzle
            + "\",\"mode\":\"smart\",\"includeTrace\":"
            + (includeTrace ? "true" : "false"));
    };

    const std::string unique = solve(CanonicalPuzzle, false);
    expectContains(unique, "\"result\":\"unique\"", "unique puzzle classification changed");
    expectContains(unique, "\"solution\":\"534678912", "unique solution is missing");
    expectContains(unique, "\"elapsedMicros\":", "solve timing is missing");
    expectNotContains(unique, "\"steps\":", "includeTrace=false still emitted steps");
    expectContains(solve(InitialDuplicatePuzzle, true), "\"result\":\"invalid\"",
                   "invalid puzzle classification changed");
    expectContains(solve(UnsolvablePuzzle, true), "\"result\":\"unsolvable\"",
                   "unsolvable puzzle classification changed");
    expectContains(solve(MultipleSolutionPuzzle, true), "\"result\":\"multiple\"",
                   "multiple puzzle classification changed");
}

SolveStep step(StepType type, int row, int col, int number, int depth) {
    SolveStep value{};
    value.type = type;
    value.row = row;
    value.col = col;
    value.number = number;
    value.depth = depth;
    value.reason = "legacy English must not cross the boundary";
    return value;
}

void mapsLegacyStepsToStableLanguageNeutralTrace() {
    SolveStep placement = step(StepType::NakedSingle, 0, 1, 5, 0);
    const std::vector<srr::v1::TraceStep> mapped = srr::v1::adaptLegacySteps({placement});
    expect(mapped.size() == 1, "placement step was not mapped");
    expect(mapped[0].id == "step-000001", "step id is not stable");
    expect(mapped[0].technique.has_value()
               && mapped[0].technique.value() == srr::v1::TechniqueId::NakedSingle,
           "naked single technique was not mapped");
    expect(mapped[0].action == srr::v1::TraceAction::Place, "placement action was not mapped");
    expect(mapped[0].explanationKey == "trace.nakedSingle", "explanation key was not mapped");

    const std::string first = srr::v1::serializeTrace(mapped);
    const std::string second = srr::v1::serializeTrace(srr::v1::adaptLegacySteps({placement}));
    expect(first == second, "trace JSON is not stable");
    expectContains(first, "\"params\":{\"number\":5}", "numeric localization param missing");
    expectNotContains(first, "legacy English", "legacy prose leaked into v1 JSON");
}

void mapsCandidateEvidenceAndBranchSemantics() {
    SolveStep elimination = step(StepType::CandidateRemovedByLogic, 2, 3, 7, 1);
    elimination.relatedRow = 2;
    elimination.relatedCol = 0;
    elimination.maskBefore = 0x1c0;
    elimination.maskAfter = 0x0c0;
    elimination.removedMask = 0x100;
    SolveStep guess = step(StepType::Guess, 4, 4, 6, 1);
    SolveStep contradiction = step(StepType::Contradiction, 4, 5, 0, 1);
    SolveStep backtrack = step(StepType::Backtrack, 4, 4, 6, 1);
    SolveStep complete = step(StepType::Solved, -1, -1, 0, 0);

    const std::vector<srr::v1::TraceStep> mapped =
        srr::v1::adaptLegacySteps({guess, elimination, contradiction, backtrack, complete});
    const std::string json = srr::v1::serializeTrace(mapped);
    expectContains(json, "\"action\":\"branch\"", "guess did not map to branch");
    expectContains(json, "\"action\":\"eliminate\"", "candidate removal did not map to elimination");
    expectContains(json, "\"beforeMask\":448,\"afterMask\":192,\"removedMask\":256",
                   "candidate masks/removals were not preserved");
    expectContains(json, "\"relation\":\"supports\"", "support evidence missing");
    expectContains(json, "\"relation\":\"excludes\"", "exclusion evidence missing");
    expectContains(json, "\"relation\":\"conflicts\"", "conflict evidence missing");
    expectContains(json, "\"relation\":\"branches\"", "branch evidence missing");
    expectContains(json, "\"relation\":\"reverts\"", "revert evidence missing");
    expectContains(json, "\"depth\":1,\"parentId\":\"step-000001\"",
                   "branch depth/parent IDs were not preserved");
}

void mapsEveryLegacyStepType() {
    const std::vector<StepType> types = {
        StepType::AnalyzeCell, StepType::RemoveCandidate, StepType::NakedSingle,
        StepType::HiddenSingle, StepType::LockedCandidate, StepType::BoxLineReduction,
        StepType::NakedPair, StepType::HiddenPair, StepType::XWing,
        StepType::CandidateRemovedByLogic, StepType::ModeChanged, StepType::Guess,
        StepType::PlaceNumber, StepType::Contradiction, StepType::Backtrack,
        StepType::Solved, StepType::NoSolution, StepType::MultipleSolutions,
        StepType::InvalidInput, StepType::TurboSolved,
    };
    std::vector<SolveStep> legacy;
    for (StepType type : types) {
        legacy.push_back(step(type, 0, 0, 1, 0));
    }
    const auto mapped = srr::v1::adaptLegacySteps(legacy);
    expect(mapped.size() == types.size(), "one or more legacy StepType values were omitted");
    for (const auto& value : mapped) {
        expect(!value.explanationKey.empty(), "legacy StepType has no explanation key");
    }
}

void generateHintAndAnalyzeHaveVersionedSuccessResponses() {
    const std::string generated = request(
        "\"operation\":\"generate\",\"difficulty\":\"easy\",\"seed\":12345");
    expectContains(generated, "\"operation\":\"generate\",\"ok\":true",
                   "generate did not return success");
    expectContains(generated, "\"seed\":12345", "generate did not preserve the seed");
    expectContains(generated, "\"puzzle\":\"", "generate omitted the puzzle");
    expect(generated == request(
        "\"operation\":\"generate\",\"difficulty\":\"easy\",\"seed\":12345"),
        "seeded generate response is not deterministic");

    const std::string hint = request("\"operation\":\"hint\",\"puzzle\":\""
        + std::string(CanonicalPuzzle) + "\",\"level\":\"direct\"");
    expectContains(hint, "\"operation\":\"hint\",\"ok\":true", "hint did not return success");
    expectContains(hint, "\"available\":true", "hint did not return an available move");
    expectNotContains(hint, "Focus around", "legacy hint prose leaked into v1 JSON");

    const std::string analyze = request("\"operation\":\"analyze\",\"puzzle\":\""
        + std::string(CanonicalPuzzle) + "\",\"mode\":\"smart\"");
    expectContains(analyze, "\"operation\":\"analyze\",\"ok\":true",
                   "analyze did not return success");
    expectContains(analyze, "\"difficulty\":{\"grade\":", "analyze omitted difficulty");
    expectNotContains(analyze, "puzzle, score", "legacy difficulty prose leaked into v1 JSON");

    expectContains(request("\"operation\":\"hint\",\"puzzle\":\"bad\",\"level\":\"direct\""),
                   "\"code\":\"malformed_puzzle\"", "hint accepted malformed input");
    expectContains(request("\"operation\":\"analyze\",\"puzzle\":\"bad\""),
                   "\"code\":\"malformed_puzzle\"", "analyze accepted malformed input");
}
}

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"validates version operation modes types and ranges", validatesVersionOperationModesTypesAndRanges},
        {"classifies all solve results and can exclude trace", classifiesAllSolveResultsAndCanExcludeTrace},
        {"maps legacy steps to stable language-neutral trace", mapsLegacyStepsToStableLanguageNeutralTrace},
        {"maps candidate evidence and branch semantics", mapsCandidateEvidenceAndBranchSemantics},
        {"maps every legacy step type", mapsEveryLegacyStepType},
        {"generate hint and analyze have versioned success responses", generateHintAndAnalyzeHaveVersionedSuccessResponses},
    };

    int failed = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "PASS: " << test.first << '\n';
        } catch (const std::exception& error) {
            ++failed;
            std::cerr << "FAIL: " << test.first << ": " << error.what() << '\n';
        }
    }
    if (failed != 0) {
        std::cerr << failed << " test(s) failed\n";
        return 1;
    }
    std::cout << tests.size() << " test(s) passed\n";
    return 0;
}
