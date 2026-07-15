#include "CoreProtocol.h"

#include "DifficultyAnalyzer.h"
#include "PuzzleGenerator.h"
#include "PuzzleString.h"
#include "SolveTrace.h"
#include "Solver.h"

#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace srr::v1 {
namespace {

struct JsonValue {
    enum class Kind { Null, Boolean, Number, String, Array, Object };
    Kind kind = Kind::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<JsonValue> array;
    std::map<std::string, JsonValue> object;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& input) : input_(input) {}

    JsonValue parse() {
        skipWhitespace();
        JsonValue value = parseValue();
        skipWhitespace();
        if (position_ != input_.size()) fail();
        return value;
    }

private:
    JsonValue parseValue() {
        if (position_ >= input_.size()) fail();
        const char ch = input_[position_];
        if (ch == '{') return parseObject();
        if (ch == '[') return parseArray();
        if (ch == '"') {
            JsonValue value;
            value.kind = JsonValue::Kind::String;
            value.string = parseString();
            return value;
        }
        if (ch == 't') return parseLiteral("true", JsonValue::Kind::Boolean, true);
        if (ch == 'f') return parseLiteral("false", JsonValue::Kind::Boolean, false);
        if (ch == 'n') return parseLiteral("null", JsonValue::Kind::Null, false);
        if (ch == '-' || (ch >= '0' && ch <= '9')) return parseNumber();
        fail();
    }

    JsonValue parseObject() {
        JsonValue value;
        value.kind = JsonValue::Kind::Object;
        ++position_;
        skipWhitespace();
        if (consume('}')) return value;
        while (true) {
            if (position_ >= input_.size() || input_[position_] != '"') fail();
            std::string key = parseString();
            skipWhitespace();
            if (!consume(':')) fail();
            skipWhitespace();
            if (!value.object.emplace(std::move(key), parseValue()).second) fail();
            skipWhitespace();
            if (consume('}')) break;
            if (!consume(',')) fail();
            skipWhitespace();
        }
        return value;
    }

    JsonValue parseArray() {
        JsonValue value;
        value.kind = JsonValue::Kind::Array;
        ++position_;
        skipWhitespace();
        if (consume(']')) return value;
        while (true) {
            value.array.push_back(parseValue());
            skipWhitespace();
            if (consume(']')) break;
            if (!consume(',')) fail();
            skipWhitespace();
        }
        return value;
    }

    JsonValue parseNumber() {
        const size_t start = position_;
        if (input_[position_] == '-') ++position_;
        if (position_ >= input_.size()) fail();
        if (input_[position_] == '0') {
            ++position_;
        } else {
            if (input_[position_] < '1' || input_[position_] > '9') fail();
            while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
                ++position_;
            }
        }
        if (position_ < input_.size() && input_[position_] == '.') {
            ++position_;
            const size_t digits = position_;
            while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
                ++position_;
            }
            if (digits == position_) fail();
        }
        if (position_ < input_.size() && (input_[position_] == 'e' || input_[position_] == 'E')) {
            ++position_;
            if (position_ < input_.size() && (input_[position_] == '+' || input_[position_] == '-')) ++position_;
            const size_t digits = position_;
            while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
                ++position_;
            }
            if (digits == position_) fail();
        }
        JsonValue value;
        value.kind = JsonValue::Kind::Number;
        try {
            value.number = std::stod(input_.substr(start, position_ - start));
        } catch (...) {
            fail();
        }
        if (!std::isfinite(value.number)) fail();
        return value;
    }

    JsonValue parseLiteral(const char* literal, JsonValue::Kind kind, bool boolean) {
        const std::string expected(literal);
        if (input_.compare(position_, expected.size(), expected) != 0) fail();
        position_ += expected.size();
        JsonValue value;
        value.kind = kind;
        value.boolean = boolean;
        return value;
    }

    std::string parseString() {
        if (!consume('"')) fail();
        std::string out;
        while (position_ < input_.size()) {
            const unsigned char ch = static_cast<unsigned char>(input_[position_++]);
            if (ch == '"') return out;
            if (ch < 0x20) fail();
            if (ch != '\\') {
                out.push_back(static_cast<char>(ch));
                continue;
            }
            if (position_ >= input_.size()) fail();
            switch (input_[position_++]) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                unsigned int code = 0;
                for (int i = 0; i < 4; ++i) {
                    if (position_ >= input_.size()) fail();
                    const char digit = input_[position_++];
                    code <<= 4;
                    if (digit >= '0' && digit <= '9') code += static_cast<unsigned int>(digit - '0');
                    else if (digit >= 'a' && digit <= 'f') code += static_cast<unsigned int>(digit - 'a' + 10);
                    else if (digit >= 'A' && digit <= 'F') code += static_cast<unsigned int>(digit - 'A' + 10);
                    else fail();
                }
                if (code <= 0x7f) out.push_back(static_cast<char>(code));
                else if (code <= 0x7ff) {
                    out.push_back(static_cast<char>(0xc0 | (code >> 6)));
                    out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                } else {
                    out.push_back(static_cast<char>(0xe0 | (code >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
                    out.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                }
                break;
            }
            default: fail();
            }
        }
        fail();
    }

    void skipWhitespace() {
        while (position_ < input_.size()) {
            const char ch = input_[position_];
            if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') break;
            ++position_;
        }
    }

    bool consume(char expected) {
        if (position_ < input_.size() && input_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    [[noreturn]] void fail() const { throw std::runtime_error("invalid_json"); }

    const std::string& input_;
    size_t position_ = 0;
};

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

std::string errorResponse(const std::optional<std::string>& operation,
                          const std::string& code,
                          const std::string& path) {
    std::ostringstream out;
    out << "{\"schemaVersion\":1,\"operation\":";
    if (operation) out << jsonString(*operation);
    else out << "null";
    out << ",\"ok\":false,\"error\":{\"code\":" << jsonString(code)
        << ",\"path\":" << jsonString(path) << ",\"params\":{}}}";
    return out.str();
}

const JsonValue* field(const JsonValue& root, const std::string& name) {
    const auto it = root.object.find(name);
    return it == root.object.end() ? nullptr : &it->second;
}

std::optional<std::string> unknownField(const JsonValue& root,
                                        const std::vector<std::string>& allowed) {
    for (const auto& [name, value] : root.object) {
        (void)value;
        if (std::find(allowed.begin(), allowed.end(), name) == allowed.end()) return name;
    }
    return std::nullopt;
}

bool integer(const JsonValue& value, std::int64_t& result) {
    if (value.kind != JsonValue::Kind::Number || std::floor(value.number) != value.number
        || value.number < static_cast<double>(std::numeric_limits<std::int64_t>::min())
        || value.number > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
        return false;
    }
    result = static_cast<std::int64_t>(value.number);
    return true;
}

std::string resultName(SolveResult result) {
    switch (result) {
    case SolveResult::InvalidInput: return "invalid";
    case SolveResult::NoSolution: return "unsolvable";
    case SolveResult::SolvedUnique: return "unique";
    case SolveResult::MultipleSolutions: return "multiple";
    }
    return "invalid";
}

std::string gradeName(DifficultyGrade grade) {
    switch (grade) {
    case DifficultyGrade::Easy: return "easy";
    case DifficultyGrade::Medium: return "medium";
    case DifficultyGrade::Hard: return "hard";
    case DifficultyGrade::Expert: return "expert";
    case DifficultyGrade::Invalid: return "invalid";
    case DifficultyGrade::Multiple: return "multiple";
    case DifficultyGrade::Unsolvable: return "unsolvable";
    }
    return "invalid";
}

int techniqueWeight(StepType type) {
    switch (type) {
    case StepType::NakedSingle: return 1;
    case StepType::HiddenSingle: return 2;
    case StepType::LockedCandidate:
    case StepType::BoxLineReduction: return 4;
    case StepType::NakedPair:
    case StepType::HiddenPair: return 6;
    case StepType::XWing: return 10;
    case StepType::Guess: return 18;
    case StepType::TurboSolved: return 20;
    default: return 0;
    }
}

std::optional<TechniqueId> hardestTechnique(const std::vector<SolveStep>& steps) {
    int hardest = 0;
    std::optional<TechniqueId> result;
    for (const SolveStep& step : steps) {
        const int weight = techniqueWeight(step.type);
        const auto technique = techniqueForStep(step.type);
        if (technique && weight > hardest) {
            hardest = weight;
            result = technique;
        }
    }
    return result;
}

std::string serializeDifficulty(const DifficultyReport& report,
                                const std::vector<SolveStep>& steps) {
    const TechniqueStats& stats = report.stats;
    std::ostringstream out;
    out << "{\"grade\":" << jsonString(gradeName(report.grade))
        << ",\"score\":" << report.score
        << ",\"givens\":" << report.givens
        << ",\"emptyCells\":" << report.emptyCells
        << ",\"maxBranchDepth\":" << report.maxBranchDepth
        << ",\"hardestTechnique\":";
    const auto hardest = hardestTechnique(steps);
    if (hardest) out << jsonString(techniqueIdName(*hardest));
    else out << "null";
    out << ",\"stats\":{\"nakedSingles\":" << stats.nakedSingles
        << ",\"hiddenSingles\":" << stats.hiddenSingles
        << ",\"lockedCandidates\":" << stats.lockedCandidates
        << ",\"boxLineReductions\":" << stats.boxLineReductions
        << ",\"nakedPairs\":" << stats.nakedPairs
        << ",\"hiddenPairs\":" << stats.hiddenPairs
        << ",\"xWings\":" << stats.xWings
        << ",\"guesses\":" << stats.guesses
        << ",\"backtracks\":" << stats.backtracks
        << ",\"contradictions\":" << stats.contradictions
        << ",\"totalSteps\":" << stats.totalSteps << "}}";
    return out.str();
}

bool parsePuzzleValue(const JsonValue& root,
                      const std::optional<std::string>& operation,
                      Board& board,
                      std::string& failure) {
    const JsonValue* puzzle = field(root, "puzzle");
    if (!puzzle) {
        failure = errorResponse(operation, "missing_field", "$.puzzle");
        return false;
    }
    if (puzzle->kind != JsonValue::Kind::String) {
        failure = errorResponse(operation, "wrong_type", "$.puzzle");
        return false;
    }
    if (puzzle->string.size() != 81
        || puzzle->string.find_first_not_of("0123456789") != std::string::npos) {
        failure = errorResponse(operation, "malformed_puzzle", "$.puzzle");
        return false;
    }
    std::string parseError;
    if (!PuzzleString::parse(puzzle->string, board, &parseError)) {
        failure = errorResponse(operation, "malformed_puzzle", "$.puzzle");
        return false;
    }
    return true;
}

bool parseMode(const JsonValue& root,
               const std::optional<std::string>& operation,
               SolverMode& mode,
               std::string& failure) {
    const JsonValue* value = field(root, "mode");
    if (!value) {
        mode = SolverMode::Smart;
        return true;
    }
    if (value->kind != JsonValue::Kind::String) {
        failure = errorResponse(operation, "wrong_type", "$.mode");
        return false;
    }
    if (value->string == "human") mode = SolverMode::HumanLogic;
    else if (value->string == "smart") mode = SolverMode::Smart;
    else if (value->string == "turbo") mode = SolverMode::Turbo;
    else {
        failure = errorResponse(operation, "invalid_value", "$.mode");
        return false;
    }
    return true;
}

std::string solveOperation(const JsonValue& root, const std::optional<std::string>& operation) {
    Board puzzle;
    SolverMode mode;
    std::string failure;
    if (!parsePuzzleValue(root, operation, puzzle, failure) || !parseMode(root, operation, mode, failure)) {
        return failure;
    }
    bool includeTrace = true;
    if (const JsonValue* value = field(root, "includeTrace")) {
        if (value->kind != JsonValue::Kind::Boolean) {
            return errorResponse(operation, "wrong_type", "$.includeTrace");
        }
        includeTrace = value->boolean;
    }

    Solver solver;
    const auto started = std::chrono::steady_clock::now();
    const SolveResult result = solver.solveUniqueOrMultiple(puzzle, mode);
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - started).count();
    DifficultyAnalyzer analyzer;
    const DifficultyReport report = analyzer.analyze(puzzle, result, solver.getSteps());

    std::ostringstream out;
    out << "{\"schemaVersion\":1,\"operation\":\"solve\",\"ok\":true,\"result\":"
        << jsonString(resultName(result));
    if (result == SolveResult::SolvedUnique || result == SolveResult::MultipleSolutions) {
        out << ",\"solution\":" << jsonString(PuzzleString::serialize(solver.getFinalBoard()));
    }
    out << ",\"elapsedMicroseconds\":" << elapsed
        << ",\"difficulty\":" << serializeDifficulty(report, solver.getSteps())
        << ",\"steps\":"
        << (includeTrace ? serializeTrace(adaptLegacySteps(solver.getSteps())) : "[]");
    out << '}';
    return out.str();
}

bool parseDifficulty(const JsonValue& root,
                     const std::optional<std::string>& operation,
                     PuzzleDifficulty& difficulty,
                     std::string& failure) {
    const JsonValue* value = field(root, "difficulty");
    if (!value) {
        failure = errorResponse(operation, "missing_field", "$.difficulty");
        return false;
    }
    if (value->kind != JsonValue::Kind::String) {
        failure = errorResponse(operation, "wrong_type", "$.difficulty");
        return false;
    }
    if (value->string == "easy") difficulty = PuzzleDifficulty::Easy;
    else if (value->string == "medium") difficulty = PuzzleDifficulty::Medium;
    else if (value->string == "hard") difficulty = PuzzleDifficulty::Hard;
    else if (value->string == "expert") difficulty = PuzzleDifficulty::Expert;
    else {
        failure = errorResponse(operation, "invalid_value", "$.difficulty");
        return false;
    }
    return true;
}

std::string puzzleDifficultyName(PuzzleDifficulty difficulty) {
    switch (difficulty) {
    case PuzzleDifficulty::Easy: return "easy";
    case PuzzleDifficulty::Medium: return "medium";
    case PuzzleDifficulty::Hard: return "hard";
    case PuzzleDifficulty::Expert: return "expert";
    }
    return "easy";
}

std::string generateOperation(const JsonValue& root, const std::optional<std::string>& operation) {
    PuzzleDifficulty difficulty;
    std::string failure;
    if (!parseDifficulty(root, operation, difficulty, failure)) return failure;

    std::optional<std::uint32_t> seed;
    if (const JsonValue* value = field(root, "seed")) {
        std::int64_t parsed = 0;
        if (!integer(*value, parsed)) return errorResponse(operation, "wrong_type", "$.seed");
        if (parsed < 0 || parsed > std::numeric_limits<std::uint32_t>::max()) {
            return errorResponse(operation, "out_of_range", "$.seed");
        }
        seed = static_cast<std::uint32_t>(parsed);
    }

    PuzzleGenerator generator;
    const GeneratedPuzzle generated = seed ? generator.generate(difficulty, *seed)
                                           : generator.generate(difficulty);
    std::uint32_t responseSeed = seed.value_or(
        static_cast<std::uint32_t>(std::stoul(generated.seed, nullptr, 16)));
    std::ostringstream out;
    out << "{\"schemaVersion\":1,\"operation\":\"generate\",\"ok\":true"
        << ",\"puzzle\":" << jsonString(PuzzleString::serialize(generated.puzzle))
        << ",\"solution\":" << jsonString(PuzzleString::serialize(generated.solution))
        << ",\"difficulty\":" << jsonString(puzzleDifficultyName(difficulty))
        << ",\"givens\":" << generated.givens
        << ",\"attempts\":" << generated.attempts
        << ",\"seed\":" << responseSeed
        << ",\"report\":" << serializeDifficulty(generated.report, generated.analysisSteps) << '}';
    return out.str();
}

std::string hintOperation(const JsonValue& root, const std::optional<std::string>& operation) {
    Board puzzle;
    std::string failure;
    if (!parsePuzzleValue(root, operation, puzzle, failure)) return failure;
    const JsonValue* level = field(root, "level");
    if (level && level->kind != JsonValue::Kind::String) {
        return errorResponse(operation, "wrong_type", "$.level");
    }
    const std::string levelName = level ? level->string : "gentle";
    if (levelName != "gentle" && levelName != "technique" && levelName != "direct") {
        return errorResponse(operation, "invalid_value", "$.level");
    }

    Solver solver;
    solver.solveUniqueOrMultiple(puzzle, SolverMode::HumanLogic);
    std::optional<SolveStep> selected;
    for (const SolveStep& step : solver.getSteps()) {
        if (Board::isInside(step.row, step.col) && step.number >= 1 && step.number <= 9
            && (step.type == StepType::NakedSingle || step.type == StepType::HiddenSingle
                || step.type == StepType::PlaceNumber)) {
            selected = step;
            break;
        }
    }
    std::ostringstream out;
    out << "{\"schemaVersion\":1,\"operation\":\"hint\",\"ok\":true,\"available\":"
        << (selected ? "true" : "false") << ",\"disclosure\":" << jsonString(levelName)
        << ",\"step\":";
    if (selected) {
        TraceStep hintStep = adaptLegacySteps({*selected}).front();
        if (levelName != "direct") {
            hintStep.action = TraceAction::Analyze;
            hintStep.candidateDeltas.clear();
            hintStep.explanationParams.erase("number");
            for (EvidenceNode& node : hintStep.evidence.nodes) node.digit.reset();
            if (levelName == "gentle") {
                hintStep.technique.reset();
                hintStep.explanationKey = "hint.gentle";
            } else {
                hintStep.explanationKey = "hint.technique";
            }
        }
        const std::string trace = serializeTrace({hintStep});
        out << trace.substr(1, trace.size() - 2);
    } else {
        out << "null";
    }
    out << '}';
    return out.str();
}

std::string analyzeOperation(const JsonValue& root, const std::optional<std::string>& operation) {
    Board puzzle;
    SolverMode mode;
    std::string failure;
    if (!parsePuzzleValue(root, operation, puzzle, failure) || !parseMode(root, operation, mode, failure)) {
        return failure;
    }
    Solver solver;
    const SolveResult result = solver.solveUniqueOrMultiple(puzzle, mode);
    DifficultyAnalyzer analyzer;
    const DifficultyReport report = analyzer.analyze(puzzle, result, solver.getSteps());
    std::ostringstream out;
    out << "{\"schemaVersion\":1,\"operation\":\"analyze\",\"ok\":true,\"result\":"
        << jsonString(resultName(result)) << ",\"difficulty\":"
        << serializeDifficulty(report, solver.getSteps()) << '}';
    return out.str();
}

}  // namespace

std::string dispatchJson(const std::string& requestJson) {
    JsonValue root;
    try {
        root = JsonParser(requestJson).parse();
    } catch (...) {
        return errorResponse(std::nullopt, "invalid_json", "$");
    }
    if (root.kind != JsonValue::Kind::Object) {
        return errorResponse(std::nullopt, "wrong_type", "$");
    }

    const JsonValue* version = field(root, "schemaVersion");
    if (!version) return errorResponse(std::nullopt, "missing_field", "$.schemaVersion");
    std::int64_t parsedVersion = 0;
    if (!integer(*version, parsedVersion)) {
        return errorResponse(std::nullopt, "wrong_type", "$.schemaVersion");
    }
    if (parsedVersion != SchemaVersion) {
        return errorResponse(std::nullopt, "unsupported_schema_version", "$.schemaVersion");
    }

    const JsonValue* operationValue = field(root, "operation");
    if (!operationValue) return errorResponse(std::nullopt, "missing_field", "$.operation");
    if (operationValue->kind != JsonValue::Kind::String) {
        return errorResponse(std::nullopt, "wrong_type", "$.operation");
    }
    const std::optional<std::string> operation = operationValue->string;
    std::vector<std::string> allowed = {"schemaVersion", "operation"};
    if (*operation == "solve") allowed.insert(allowed.end(), {"puzzle", "mode", "includeTrace"});
    else if (*operation == "generate") allowed.insert(allowed.end(), {"difficulty", "seed"});
    else if (*operation == "hint") allowed.insert(allowed.end(), {"puzzle", "level"});
    else if (*operation == "analyze") allowed.insert(allowed.end(), {"puzzle", "mode"});
    if (const auto unknown = unknownField(root, allowed)) {
        return errorResponse(operation, "unknown_field", "$." + *unknown);
    }
    if (*operation == "solve") return solveOperation(root, operation);
    if (*operation == "generate") return generateOperation(root, operation);
    if (*operation == "hint") return hintOperation(root, operation);
    if (*operation == "analyze") return analyzeOperation(root, operation);
    return errorResponse(operation, "unsupported_operation", "$.operation");
}

}  // namespace srr::v1
