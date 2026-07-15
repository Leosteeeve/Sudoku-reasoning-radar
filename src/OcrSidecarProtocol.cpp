#include "OcrSidecarProtocol.h"

#include <cctype>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>

namespace {

struct JsonValue {
    bool stringValue = false;
    std::string text;
    long long number = 0;
};

void appendUtf8(std::string& output, unsigned int codePoint) {
    if (codePoint <= 0x7f) output.push_back(static_cast<char>(codePoint));
    else if (codePoint <= 0x7ff) {
        output.push_back(static_cast<char>(0xc0 | (codePoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
    } else {
        output.push_back(static_cast<char>(0xe0 | (codePoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
    }
}

class FlatJsonParser {
public:
    explicit FlatJsonParser(const std::string& source) : source_(source) {}

    bool parse(std::map<std::string, JsonValue>& values, std::string& error) {
        skipWhitespace();
        if (!consume('{')) return fail("request must be a JSON object", error);
        skipWhitespace();
        if (consume('}')) return finish(error);
        while (true) {
            std::string key;
            if (!parseString(key, error)) return false;
            if (values.count(key) != 0) return fail("duplicate request field", error);
            skipWhitespace();
            if (!consume(':')) return fail("request field needs a value", error);
            skipWhitespace();
            JsonValue value;
            if (peek() == '"') {
                value.stringValue = true;
                if (!parseString(value.text, error)) return false;
            } else if (!parseInteger(value.number)) {
                return fail("request values must be strings or integers", error);
            }
            values.emplace(key, value);
            skipWhitespace();
            if (consume('}')) return finish(error);
            if (!consume(',')) return fail("request fields must be comma separated", error);
            skipWhitespace();
        }
    }

private:
    char peek() const { return position_ < source_.size() ? source_[position_] : '\0'; }
    bool consume(char expected) {
        if (peek() != expected) return false;
        ++position_;
        return true;
    }
    void skipWhitespace() {
        while (position_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[position_]))) ++position_;
    }
    bool finish(std::string& error) {
        skipWhitespace();
        return position_ == source_.size() ? true : fail("unexpected trailing request data", error);
    }
    bool fail(const std::string& message, std::string& error) {
        error = message;
        return false;
    }
    bool parseInteger(long long& output) {
        const size_t start = position_;
        if (peek() == '-') ++position_;
        if (!std::isdigit(static_cast<unsigned char>(peek()))) { position_ = start; return false; }
        long long value = 0;
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            value = value * 10 + (source_[position_++] - '0');
        }
        output = source_[start] == '-' ? -value : value;
        return true;
    }
    bool parseString(std::string& output, std::string& error) {
        if (!consume('"')) return fail("request field names and paths must be strings", error);
        while (position_ < source_.size()) {
            const unsigned char current = static_cast<unsigned char>(source_[position_++]);
            if (current == '"') return true;
            if (current < 0x20) return fail("unescaped control character in request", error);
            if (current != '\\') { output.push_back(static_cast<char>(current)); continue; }
            if (position_ >= source_.size()) return fail("incomplete JSON escape", error);
            const char escaped = source_[position_++];
            switch (escaped) {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case '/': output.push_back('/'); break;
            case 'b': output.push_back('\b'); break;
            case 'f': output.push_back('\f'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            case 'u': {
                if (position_ + 4 > source_.size()) return fail("incomplete Unicode escape", error);
                unsigned int codePoint = 0;
                for (int index = 0; index < 4; ++index) {
                    const char digit = source_[position_++];
                    codePoint <<= 4;
                    if (digit >= '0' && digit <= '9') codePoint |= static_cast<unsigned int>(digit - '0');
                    else if (digit >= 'a' && digit <= 'f') codePoint |= static_cast<unsigned int>(digit - 'a' + 10);
                    else if (digit >= 'A' && digit <= 'F') codePoint |= static_cast<unsigned int>(digit - 'A' + 10);
                    else return fail("invalid Unicode escape", error);
                }
                appendUtf8(output, codePoint);
                break;
            }
            default: return fail("invalid JSON escape", error);
            }
        }
        return fail("unterminated JSON string", error);
    }

    const std::string& source_;
    size_t position_ = 0;
};

std::string escapeJson(const std::string& value) {
    std::ostringstream output;
    for (const unsigned char byte : value) {
        switch (byte) {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (byte < 0x20) output << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(byte) << std::dec;
            else output << static_cast<char>(byte);
        }
    }
    return output.str();
}

} // namespace

bool parseOcrSidecarRequest(const std::string& line,
                            OcrSidecarRequest& request,
                            std::string& error) {
    std::map<std::string, JsonValue> values;
    if (!FlatJsonParser(line).parse(values, error)) return false;
    if (values.size() != 3 || values.count("version") == 0 || values.count("operation") == 0 || values.count("imagePath") == 0) {
        error = "request must contain exactly version, operation, and imagePath";
        return false;
    }
    const JsonValue& version = values.at("version");
    const JsonValue& operation = values.at("operation");
    const JsonValue& imagePath = values.at("imagePath");
    if (version.stringValue || version.number != 1) { error = "unsupported request version"; return false; }
    if (!operation.stringValue || operation.text != "recognize") { error = "unsupported request operation"; return false; }
    if (!imagePath.stringValue || imagePath.text.empty()) { error = "imagePath must be a non-empty string"; return false; }
    request = OcrSidecarRequest{1, operation.text, imagePath.text};
    return true;
}

std::string serializeOcrSidecarSuccess(const std::string& puzzle,
                                       const std::vector<OcrSidecarCell>& cells) {
    if (puzzle.size() != 81 || cells.size() != 81) return serializeOcrSidecarError("invalid OCR result");
    std::ostringstream output;
    output << "{\"version\":1,\"ok\":true,\"puzzle\":\"" << escapeJson(puzzle) << "\",\"cells\":[";
    for (size_t index = 0; index < cells.size(); ++index) {
        if (index != 0) output << ',';
        const OcrSidecarCell& cell = cells[index];
        output << "{\"digit\":" << cell.digit
               << ",\"confidence\":" << std::setprecision(6) << cell.confidence
               << ",\"lowConfidence\":" << (cell.lowConfidence ? "true" : "false") << '}';
    }
    output << "]}";
    return output.str();
}

std::string serializeOcrSidecarError(const std::string& error) {
    return "{\"version\":1,\"ok\":false,\"error\":\"" + escapeJson(error) + "\"}";
}
