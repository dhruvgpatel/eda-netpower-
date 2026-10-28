#include "netpower/parser.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace netpower {

Lexer::Lexer(std::string_view source, std::string_view filename)
    : source_(source), filename_(filename) {}

char Lexer::peek() const {
    if (pos_ < source_.size()) {
        return source_[pos_];
    }
    return '\0';
}

char Lexer::get() {
    if (pos_ < source_.size()) {
        char ch = source_[pos_++];
        if (ch == '\n') {
            line_++;
            col_ = 1;
        } else {
            col_++;
        }
        return ch;
    }
    return '\0';
}

void Lexer::skip_whitespace_and_comments() {
    while (pos_ < source_.size()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            get();
            continue;
        }

        // Check for comments
        if (c == '/' && pos_ + 1 < source_.size()) {
            char next = source_[pos_ + 1];
            if (next == '/') {
                // Line comment
                get(); // /
                get(); // /
                while (pos_ < source_.size() && peek() != '\n') {
                    get();
                }
                continue;
            } else if (next == '*') {
                // Block comment
                size_t start_line = line_;
                size_t start_col = col_;
                get(); // /
                get(); // *
                bool closed = false;
                while (pos_ < source_.size()) {
                    if (peek() == '*' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
                        get(); // *
                        get(); // /
                        closed = true;
                        break;
                    }
                    get();
                }
                if (!closed) {
                    throw ParseError(filename_, start_line, start_col, "unclosed block comment");
                }
                continue;
            }
        }

        break;
    }
}

Token Lexer::scan_identifier_or_keyword() {
    size_t start_line = line_;
    size_t start_col = col_;
    std::string text;

    while (pos_ < source_.size()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '$') {
            text += get();
        } else {
            break;
        }
    }

    TokenType type = TokenType::IDENTIFIER;

    // Check keywords
    if (text == "module") type = TokenType::KW_MODULE;
    else if (text == "endmodule") type = TokenType::KW_ENDMODULE;
    else if (text == "input") type = TokenType::KW_INPUT;
    else if (text == "output") type = TokenType::KW_OUTPUT;
    else if (text == "wire") type = TokenType::KW_WIRE;
    else if (text == "and") type = TokenType::GATE_AND;
    else if (text == "or") type = TokenType::GATE_OR;
    else if (text == "not") type = TokenType::GATE_NOT;
    else if (text == "nand") type = TokenType::GATE_NAND;
    else if (text == "nor") type = TokenType::GATE_NOR;
    else if (text == "xor") type = TokenType::GATE_XOR;
    else if (text == "xnor") type = TokenType::GATE_XNOR;
    else if (text == "buf") type = TokenType::GATE_BUF;
    // Check unsupported Verilog keywords explicitly
    else if (text == "assign" || text == "always" || text == "reg" || text == "inout" ||
             text == "parameter" || text == "defparam" || text == "initial" ||
             text == "posedge" || text == "negedge" || text == "tri" || text == "supply0" ||
             text == "supply1" || text == "wand" || text == "wor" || text == "task" ||
             text == "function" || text == "integer" || text == "real" || text == "time" ||
             text == "fork" || text == "join" || text == "specify" || text == "primitive" ||
             text == "table" || text == "generate" || text == "genvar" || text == "case" ||
             text == "if" || text == "else" || text == "while" || text == "for") {
        type = TokenType::UNSUPPORTED;
    }

    return Token{type, text, SourceLocation{start_line, start_col}};
}

Token Lexer::scan_number_or_constant() {
    size_t start_line = line_;
    size_t start_col = col_;
    std::string text;

    while (pos_ < source_.size()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '\'' || c == '_') {
            text += get();
        } else {
            break;
        }
    }

    if (text == "1'b0") {
        return Token{TokenType::CONST_0, text, SourceLocation{start_line, start_col}};
    }
    if (text == "1'b1") {
        return Token{TokenType::CONST_1, text, SourceLocation{start_line, start_col}};
    }

    throw ParseError(filename_, start_line, start_col, "malformed or unsupported constant '" + text + "' (only 1'b0 and 1'b1 are supported)");
}

Token Lexer::next_token() {
    skip_whitespace_and_comments();

    if (pos_ >= source_.size()) {
        return Token{TokenType::END_OF_FILE, "", SourceLocation{line_, col_}};
    }

    size_t cur_line = line_;
    size_t cur_col = col_;
    char c = peek();

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return scan_identifier_or_keyword();
    }

    if (std::isdigit(static_cast<unsigned char>(c)) || c == '\'') {
        return scan_number_or_constant();
    }

    // Punctuation
    get();
    switch (c) {
        case '(': return Token{TokenType::LPAREN, "(", SourceLocation{cur_line, cur_col}};
        case ')': return Token{TokenType::RPAREN, ")", SourceLocation{cur_line, cur_col}};
        case ',': return Token{TokenType::COMMA, ",", SourceLocation{cur_line, cur_col}};
        case ';': return Token{TokenType::SEMICOLON, ";", SourceLocation{cur_line, cur_col}};
        case '[':
        case ']':
            throw ParseError(filename_, cur_line, cur_col, "vector buses and indexing are not supported");
        case '#':
            throw ParseError(filename_, cur_line, cur_col, "gate delays are not supported");
        case '{':
        case '}':
            throw ParseError(filename_, cur_line, cur_col, "concatenation is not supported");
        default:
            throw ParseError(filename_, cur_line, cur_col, std::string("unexpected character '") + c + "'");
    }
}

Parser::Parser(Lexer lexer) : lexer_(lexer) {
    current_ = lexer_.next_token();
    peek_ = lexer_.next_token();
}

void Parser::advance() {
    current_ = peek_;
    peek_ = lexer_.next_token();
}

bool Parser::match(TokenType type) {
    if (current_.type == type) {
        advance();
        return true;
    }
    return false;
}

Token Parser::expect(TokenType type, std::string_view expected_desc) {
    if (current_.type == type) {
        Token tok = current_;
        advance();
        return tok;
    }
    throw ParseError(lexer_.filename(), current_.loc.line, current_.loc.col,
                     "expected " + std::string(expected_desc) + ", found '" + current_.text + "'");
}

Circuit Parser::parse_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw ParseError(filepath, 1, 1, "cannot open file '" + filepath + "'");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse_string(buffer.str(), filepath);
}

Circuit Parser::parse_string(std::string_view content, std::string_view filename) {
    Lexer lexer(content, filename);
    Parser parser(lexer);
    return parser.parse();
}

void Parser::parse_module_header(Circuit& circuit, std::vector<std::pair<std::string, SourceLocation>>& header_ports) {
    expect(TokenType::KW_MODULE, "'module'");

    Token module_name_tok = expect(TokenType::IDENTIFIER, "module name identifier");
    circuit.module_name = module_name_tok.text;

    expect(TokenType::LPAREN, "'('");

    std::unordered_set<std::string> seen_ports;
    if (current_.type != TokenType::RPAREN) {
        while (true) {
            Token port_tok = expect(TokenType::IDENTIFIER, "port name identifier");
            if (seen_ports.count(port_tok.text)) {
                throw ParseError(lexer_.filename(), port_tok.loc.line, port_tok.loc.col,
                                 "duplicate port '" + port_tok.text + "' in module port list");
            }
            seen_ports.insert(port_tok.text);
            header_ports.emplace_back(port_tok.text, port_tok.loc);

            if (match(TokenType::COMMA)) {
                continue;
            }
            break;
        }
    }

    expect(TokenType::RPAREN, "')'");
    expect(TokenType::SEMICOLON, "';'");
}

void Parser::parse_declarations_and_gates(Circuit& circuit, const std::vector<std::pair<std::string, SourceLocation>>& header_ports) {
    std::unordered_set<std::string> header_port_set;
    for (const auto& [name, loc] : header_ports) {
        header_port_set.insert(name);
    }

    auto add_or_update_net = [&](std::string_view name, NetRole role, SourceLocation loc) -> size_t {
        std::string sname(name);
        auto it = circuit.net_name_to_index.find(sname);
        if (it != circuit.net_name_to_index.end()) {
            size_t idx = it->second;
            Net& existing = circuit.nets[idx];
            // Allow combining INPUT and OUTPUT to form INPUT_OUTPUT (direct alias)
            if ((existing.role == NetRole::INPUT && role == NetRole::OUTPUT) ||
                (existing.role == NetRole::OUTPUT && role == NetRole::INPUT)) {
                existing.role = NetRole::INPUT_OUTPUT;
                return idx;
            }
            throw ParseError(lexer_.filename(), loc.line, loc.col,
                             "duplicate declaration of net '" + sname + "'");
        }

        size_t idx = circuit.nets.size();
        Net net;
        net.name = sname;
        net.role = role;
        net.line = loc.line;
        net.col = loc.col;
        circuit.nets.push_back(std::move(net));
        circuit.net_name_to_index[sname] = idx;
        return idx;
    };

    while (current_.type != TokenType::KW_ENDMODULE && current_.type != TokenType::END_OF_FILE) {
        if (current_.type == TokenType::UNSUPPORTED) {
            throw ParseError(lexer_.filename(), current_.loc.line, current_.loc.col,
                             "unsupported Verilog construct '" + current_.text + "'");
        }

        if (current_.type == TokenType::KW_INPUT || current_.type == TokenType::KW_OUTPUT || current_.type == TokenType::KW_WIRE) {
            TokenType decl_type = current_.type;
            advance();

            NetRole role = NetRole::INTERNAL;
            if (decl_type == TokenType::KW_INPUT) role = NetRole::INPUT;
            else if (decl_type == TokenType::KW_OUTPUT) role = NetRole::OUTPUT;

            while (true) {
                Token net_tok = expect(TokenType::IDENTIFIER, "net identifier");
                
                // Wire cannot be in header port list
                if (role == NetRole::INTERNAL && header_port_set.count(net_tok.text)) {
                    throw ParseError(lexer_.filename(), net_tok.loc.line, net_tok.loc.col,
                                     "wire '" + net_tok.text + "' cannot be declared as a module port");
                }

                // Input or output must be in header port list
                if ((role == NetRole::INPUT || role == NetRole::OUTPUT) && !header_port_set.count(net_tok.text)) {
                    throw ParseError(lexer_.filename(), net_tok.loc.line, net_tok.loc.col,
                                     "signal '" + net_tok.text + "' declared as " +
                                     std::string(decl_type == TokenType::KW_INPUT ? "input" : "output") +
                                     " is not in the module port list");
                }

                add_or_update_net(net_tok.text, role, net_tok.loc);

                if (match(TokenType::COMMA)) {
                    continue;
                }
                break;
            }
            expect(TokenType::SEMICOLON, "';'");
            continue;
        }

        // Gate primitives
        bool is_gate = false;
        GateType gtype = GateType::BUF;
        switch (current_.type) {
            case TokenType::GATE_AND:  is_gate = true; gtype = GateType::AND; break;
            case TokenType::GATE_OR:   is_gate = true; gtype = GateType::OR; break;
            case TokenType::GATE_NOT:  is_gate = true; gtype = GateType::NOT; break;
            case TokenType::GATE_NAND: is_gate = true; gtype = GateType::NAND; break;
            case TokenType::GATE_NOR:  is_gate = true; gtype = GateType::NOR; break;
            case TokenType::GATE_XOR:  is_gate = true; gtype = GateType::XOR; break;
            case TokenType::GATE_XNOR: is_gate = true; gtype = GateType::XNOR; break;
            case TokenType::GATE_BUF:  is_gate = true; gtype = GateType::BUF; break;
            default: break;
        }

        if (is_gate) {
            Token gate_prim_tok = current_;
            advance();

            Token inst_tok = expect(TokenType::IDENTIFIER, "gate instance name");
            if (circuit.gate_name_to_index.count(inst_tok.text)) {
                throw ParseError(lexer_.filename(), inst_tok.loc.line, inst_tok.loc.col,
                                 "duplicate gate instance name '" + inst_tok.text + "'");
            }

            expect(TokenType::LPAREN, "'('");

            // First terminal is gate output
            Token out_net_tok = expect(TokenType::IDENTIFIER, "output net identifier");
            auto out_net_idx = circuit.find_net_index(out_net_tok.text);
            if (!out_net_idx.has_value()) {
                throw ParseError(lexer_.filename(), out_net_tok.loc.line, out_net_tok.loc.col,
                                 "undeclared signal reference '" + out_net_tok.text + "'");
            }

            Net& out_net = circuit.nets[*out_net_idx];
            if (out_net.role == NetRole::INPUT) {
                throw ParseError(lexer_.filename(), out_net_tok.loc.line, out_net_tok.loc.col,
                                 "primary input '" + out_net.name + "' cannot be driven by gate");
            }
            if (out_net.driver_gate_index.has_value()) {
                throw ParseError(lexer_.filename(), out_net_tok.loc.line, out_net_tok.loc.col,
                                 "duplicate driver for net '" + out_net.name + "'");
            }

            Gate gate;
            gate.instance_name = inst_tok.text;
            gate.type = gtype;
            gate.output_net_index = *out_net_idx;
            gate.original_index = circuit.gates.size();
            gate.line = inst_tok.loc.line;
            gate.col = inst_tok.loc.col;

            // Gate inputs
            expect(TokenType::COMMA, "',' after output net");
            while (true) {
                GateInput g_in;
                if (current_.type == TokenType::CONST_0) {
                    g_in.kind = InputKind::CONST_0;
                    g_in.name = "1'b0";
                    advance();
                } else if (current_.type == TokenType::CONST_1) {
                    g_in.kind = InputKind::CONST_1;
                    g_in.name = "1'b1";
                    advance();
                } else if (current_.type == TokenType::IDENTIFIER) {
                    Token in_net_tok = current_;
                    advance();
                    auto in_net_idx = circuit.find_net_index(in_net_tok.text);
                    if (!in_net_idx.has_value()) {
                        throw ParseError(lexer_.filename(), in_net_tok.loc.line, in_net_tok.loc.col,
                                         "undeclared signal reference '" + in_net_tok.text + "'");
                    }
                    g_in.kind = InputKind::NET;
                    g_in.net_index = *in_net_idx;
                    g_in.name = in_net_tok.text;
                } else {
                    throw ParseError(lexer_.filename(), current_.loc.line, current_.loc.col,
                                     "expected net identifier or constant 1'b0/1'b1 as gate input, found '" + current_.text + "'");
                }

                gate.inputs.push_back(g_in);

                if (match(TokenType::COMMA)) {
                    continue;
                }
                break;
            }

            expect(TokenType::RPAREN, "')'");
            expect(TokenType::SEMICOLON, "';'");

            // Validate arity
            if (!is_valid_gate_arity(gate.type, gate.inputs.size())) {
                throw ParseError(lexer_.filename(), gate.line, gate.col,
                                 "invalid arity for gate '" + gate.instance_name + "' of primitive '" +
                                 std::string(gate_type_to_string(gate.type)) + "': expected " +
                                 (gate.type == GateType::NOT || gate.type == GateType::BUF ? "1 input" : ">= 2 inputs") +
                                 ", got " + std::to_string(gate.inputs.size()));
            }

            size_t gate_idx = circuit.gates.size();
            out_net.driver_gate_index = gate_idx;
            circuit.gate_name_to_index[gate.instance_name] = gate_idx;
            circuit.gates.push_back(std::move(gate));
            continue;
        }

        // Any other token
        throw ParseError(lexer_.filename(), current_.loc.line, current_.loc.col,
                         "unexpected token '" + current_.text + "' in module body");
    }

    expect(TokenType::KW_ENDMODULE, "'endmodule'");

    if (current_.type == TokenType::KW_MODULE) {
        throw ParseError(lexer_.filename(), current_.loc.line, current_.loc.col,
                         "multiple module definitions are not supported");
    }

    if (current_.type != TokenType::END_OF_FILE) {
        throw ParseError(lexer_.filename(), current_.loc.line, current_.loc.col,
                         "unexpected tokens after endmodule: '" + current_.text + "'");
    }

    // Check that every port in header was declared
    for (const auto& [port_name, loc] : header_ports) {
        auto it = circuit.find_net_index(port_name);
        if (!it.has_value() || (circuit.nets[*it].role != NetRole::INPUT &&
                                circuit.nets[*it].role != NetRole::OUTPUT &&
                                circuit.nets[*it].role != NetRole::INPUT_OUTPUT)) {
            throw ParseError(lexer_.filename(), loc.line, loc.col,
                             "port '" + port_name + "' declared in module header is not declared as input or output");
        }
    }

    // Populate primary inputs and outputs, and fanouts
    for (size_t net_idx = 0; net_idx < circuit.nets.size(); ++net_idx) {
        Net& net = circuit.nets[net_idx];
        if (net.role == NetRole::INPUT || net.role == NetRole::INPUT_OUTPUT) {
            circuit.primary_inputs.push_back(net_idx);
        }
        if (net.role == NetRole::OUTPUT || net.role == NetRole::INPUT_OUTPUT) {
            circuit.primary_outputs.push_back(net_idx);
        }
    }

    // Link fanouts and validate gate input sources
    for (size_t gate_idx = 0; gate_idx < circuit.gates.size(); ++gate_idx) {
        const Gate& gate = circuit.gates[gate_idx];
        for (const auto& gin : gate.inputs) {
            if (gin.kind == InputKind::NET) {
                Net& src_net = circuit.nets[gin.net_index];
                src_net.fanout_gate_indices.push_back(gate_idx);
                // Validate that src_net has a legal source (primary input or gate driver)
                if (src_net.role != NetRole::INPUT &&
                    src_net.role != NetRole::INPUT_OUTPUT &&
                    !src_net.driver_gate_index.has_value()) {
                    throw ParseError(lexer_.filename(), gate.line, gate.col,
                                     "signal '" + src_net.name + "' has no driver and is not a primary input");
                }
            }
        }
    }

    // Check that every output net has a driver or is an input alias
    for (size_t out_idx : circuit.primary_outputs) {
        const Net& net = circuit.nets[out_idx];
        if (net.role == NetRole::OUTPUT && !net.driver_gate_index.has_value()) {
            throw ParseError(lexer_.filename(), net.line, net.col,
                             "primary output '" + net.name + "' has no driver");
        }
    }

    // Check that every internal wire has a driver
    for (const Net& net : circuit.nets) {
        if (net.role == NetRole::INTERNAL && !net.driver_gate_index.has_value()) {
            throw ParseError(lexer_.filename(), net.line, net.col,
                             "wire net '" + net.name + "' has no driver");
        }
    }
}

Circuit Parser::parse() {
    Circuit circuit;
    std::vector<std::pair<std::string, SourceLocation>> header_ports;
    parse_module_header(circuit, header_ports);
    parse_declarations_and_gates(circuit, header_ports);
    return circuit;
}

}  // namespace netpower
