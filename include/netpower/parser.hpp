#pragma once

#include "netpower/circuit.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace netpower {

struct SourceLocation {
    size_t line{1};
    size_t col{1};
};

class ParseError : public std::runtime_error {
public:
    ParseError(std::string_view file, size_t line, size_t col, std::string_view message)
        : std::runtime_error(format_message(file, line, col, message)),
          file_(file),
          line_(line),
          col_(col),
          raw_message_(message) {}

    [[nodiscard]] const std::string& file() const noexcept { return file_; }
    [[nodiscard]] size_t line() const noexcept { return line_; }
    [[nodiscard]] size_t col() const noexcept { return col_; }
    [[nodiscard]] const std::string& raw_message() const noexcept { return raw_message_; }

private:
    std::string file_;
    size_t line_;
    size_t col_;
    std::string raw_message_;

    static std::string format_message(std::string_view file, size_t line, size_t col, std::string_view msg) {
        return std::string(file) + ":" + std::to_string(line) + ":" + std::to_string(col) + ": " + std::string(msg);
    }
};

enum class TokenType {
    // Keywords
    KW_MODULE,
    KW_ENDMODULE,
    KW_INPUT,
    KW_OUTPUT,
    KW_WIRE,
    
    // Primitives
    GATE_AND,
    GATE_OR,
    GATE_NOT,
    GATE_NAND,
    GATE_NOR,
    GATE_XOR,
    GATE_XNOR,
    GATE_BUF,

    // Literals & Identifiers
    IDENTIFIER,
    CONST_0,  // 1'b0
    CONST_1,  // 1'b1

    // Delimiters
    LPAREN,
    RPAREN,
    COMMA,
    SEMICOLON,

    // Unsupported Verilog constructs (for explicit error reporting)
    UNSUPPORTED,

    END_OF_FILE,
    INVALID
};

struct Token {
    TokenType type{TokenType::INVALID};
    std::string text;
    SourceLocation loc;
};

class Lexer {
public:
    Lexer(std::string_view source, std::string_view filename);

    Token next_token();
    const std::string& filename() const { return filename_; }

private:
    std::string_view source_;
    std::string filename_;
    size_t pos_{0};
    size_t line_{1};
    size_t col_{1};

    char peek() const;
    char get();
    void advance();
    void skip_whitespace_and_comments();
    Token scan_identifier_or_keyword();
    Token scan_number_or_constant();
};

class Parser {
public:
    static Circuit parse_file(const std::string& filepath);
    static Circuit parse_string(std::string_view content, std::string_view filename = "<memory>");

private:
    Parser(Lexer lexer);
    Circuit parse();

    Token current_;
    Token peek_;
    Lexer lexer_;

    void advance();
    bool match(TokenType type);
    Token expect(TokenType type, std::string_view expected_desc);

    void parse_module_header(Circuit& circuit, std::vector<std::pair<std::string, SourceLocation>>& header_ports);
    void parse_declarations_and_gates(Circuit& circuit, const std::vector<std::pair<std::string, SourceLocation>>& header_ports);
};

}  // namespace netpower

