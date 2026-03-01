/*
 * File: statement.cpp
 * -------------------
 * Implements BASIC statements and statement parsing.
 */

#include <climits>
#include <iostream>
#include <memory>
#include <regex>
#include <set>
#include <vector>
#include "statement.hpp"
#include "program.hpp"
#include "parser.hpp"
#include "Utils/error.hpp"
#include "Utils/strlib.hpp"

namespace {

bool isKeyword(const std::string &token) {
    static const std::set<std::string> keywords = {
        "REM", "LET", "PRINT", "INPUT", "END", "GOTO", "IF", "THEN",
        "RUN", "LIST", "CLEAR", "QUIT", "HELP"
    };
    return keywords.count(token) > 0;
}

int parseIntToken(const std::string &token) {
    try {
        size_t consumed = 0;
        long long value = std::stoll(token, &consumed, 10);
        if (consumed != token.size()) error("SYNTAX ERROR");
        if (value < INT_MIN || value > INT_MAX) error("SYNTAX ERROR");
        return static_cast<int>(value);
    } catch (...) {
        error("SYNTAX ERROR");
    }
    return 0;
}

Expression *parseExpressionFromTokens(const std::vector<std::string> &tokens) {
    if (tokens.empty()) error("SYNTAX ERROR");

    std::string exprText;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i) exprText += ' ';
        exprText += tokens[i];
    }

    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exprText);

    try {
        return parseExp(scanner);
    } catch (ErrorException &) {
        error("SYNTAX ERROR");
    }
    return nullptr;
}

bool parseStrictIntLine(const std::string &line, int &value) {
    std::string s = trim(line);
    static const std::regex kPattern(R"([+-]?[0-9]+)");
    if (!std::regex_match(s, kPattern)) return false;

    try {
        size_t consumed = 0;
        long long v = std::stoll(s, &consumed, 10);
        if (consumed != s.size()) return false;
        if (v < INT_MIN || v > INT_MAX) return false;
        value = static_cast<int>(v);
        return true;
    } catch (...) {
        return false;
    }
}

void requireNoMoreTokens(TokenScanner &scanner) {
    if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
}

} // namespace

Statement::Statement() = default;
Statement::~Statement() = default;

RemStatement::RemStatement() = default;

void RemStatement::execute(EvalState &state, Program &program) {
    (void) state;
    (void) program;
}

LetStatement::LetStatement(std::string varName, Expression *exp)
    : varName(std::move(varName)), exp(exp) {}

LetStatement::~LetStatement() {
    delete exp;
}

void LetStatement::execute(EvalState &state, Program &program) {
    (void) program;
    state.setValue(varName, exp->eval(state));
}

PrintStatement::PrintStatement(Expression *exp) : exp(exp) {}

PrintStatement::~PrintStatement() {
    delete exp;
}

void PrintStatement::execute(EvalState &state, Program &program) {
    (void) program;
    std::cout << exp->eval(state) << std::endl;
}

InputStatement::InputStatement(std::string varName) : varName(std::move(varName)) {}

void InputStatement::execute(EvalState &state, Program &program) {
    (void) program;
    while (true) {
        std::cout << " ? ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            error("INVALID NUMBER");
        }

        int value = 0;
        if (!parseStrictIntLine(line, value)) {
            std::cout << "INVALID NUMBER" << std::endl;
            continue;
        }

        state.setValue(varName, value);
        return;
    }
}

EndStatement::EndStatement() = default;

void EndStatement::execute(EvalState &state, Program &program) {
    (void) state;
    program.setEndFlag();
}

GotoStatement::GotoStatement(int targetLineNumber) : targetLineNumber(targetLineNumber) {}

void GotoStatement::execute(EvalState &state, Program &program) {
    (void) state;
    program.setJumpTarget(targetLineNumber);
}

IfStatement::IfStatement(Expression *lhs, std::string op, Expression *rhs, int targetLineNumber)
    : lhs(lhs), op(std::move(op)), rhs(rhs), targetLineNumber(targetLineNumber) {}

IfStatement::~IfStatement() {
    delete lhs;
    delete rhs;
}

void IfStatement::execute(EvalState &state, Program &program) {
    int lv = lhs->eval(state);
    int rv = rhs->eval(state);

    bool cond = false;
    if (op == "=") cond = (lv == rv);
    if (op == "<") cond = (lv < rv);
    if (op == ">") cond = (lv > rv);

    if (cond) {
        program.setJumpTarget(targetLineNumber);
    }
}

Statement *parseStatement(TokenScanner &scanner, bool inProgramLine) {
    try {
        if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");

        std::string keyword = scanner.nextToken();

        if (keyword == "REM") {
            if (!inProgramLine) error("SYNTAX ERROR");
            return new RemStatement();
        }

        if (keyword == "LET") {
            if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
            std::string var = scanner.nextToken();
            TokenType varType = scanner.getTokenType(var);
            if (varType != WORD || isKeyword(var)) error("SYNTAX ERROR");

            if (!scanner.hasMoreTokens() || scanner.nextToken() != "=") error("SYNTAX ERROR");

            std::vector<std::string> exprTokens;
            while (scanner.hasMoreTokens()) exprTokens.push_back(scanner.nextToken());
            Expression *exp = parseExpressionFromTokens(exprTokens);
            return new LetStatement(var, exp);
        }

        if (keyword == "PRINT") {
            std::vector<std::string> exprTokens;
            while (scanner.hasMoreTokens()) exprTokens.push_back(scanner.nextToken());
            Expression *exp = parseExpressionFromTokens(exprTokens);
            return new PrintStatement(exp);
        }

        if (keyword == "INPUT") {
            if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
            std::string var = scanner.nextToken();
            TokenType varType = scanner.getTokenType(var);
            bool validType = (varType == WORD || varType == NUMBER);
            if (!validType) error("SYNTAX ERROR");
            if (varType == WORD && isKeyword(var)) error("SYNTAX ERROR");
            requireNoMoreTokens(scanner);
            return new InputStatement(var);
        }

        if (keyword == "END") {
            if (!inProgramLine) error("SYNTAX ERROR");
            requireNoMoreTokens(scanner);
            return new EndStatement();
        }

        if (keyword == "GOTO") {
            if (!inProgramLine) error("SYNTAX ERROR");
            if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
            std::string lineToken = scanner.nextToken();
            if (scanner.getTokenType(lineToken) != NUMBER) error("SYNTAX ERROR");
            requireNoMoreTokens(scanner);
            return new GotoStatement(parseIntToken(lineToken));
        }

        if (keyword == "IF") {
            if (!inProgramLine) error("SYNTAX ERROR");

            std::vector<std::string> tokens;
            while (scanner.hasMoreTokens()) tokens.push_back(scanner.nextToken());

            int thenPos = -1;
            for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
                if (tokens[i] == "THEN") {
                    thenPos = i;
                    break;
                }
            }
            if (thenPos <= 0 || thenPos + 1 >= static_cast<int>(tokens.size())) error("SYNTAX ERROR");
            if (thenPos + 2 != static_cast<int>(tokens.size())) error("SYNTAX ERROR");

            std::string lineToken = tokens[thenPos + 1];
            TokenScanner lineScanner;
            lineScanner.scanNumbers();
            if (lineScanner.getTokenType(lineToken) != NUMBER) error("SYNTAX ERROR");
            int targetLine = parseIntToken(lineToken);

            int cmpPos = -1;
            int depth = 0;
            for (int i = 0; i < thenPos; ++i) {
                if (tokens[i] == "(") depth++;
                else if (tokens[i] == ")") depth--;
                if (depth < 0) error("SYNTAX ERROR");

                if (depth == 0 && (tokens[i] == "<" || tokens[i] == ">" || tokens[i] == "=")) {
                    if (cmpPos != -1) error("SYNTAX ERROR");
                    cmpPos = i;
                }
            }

            if (depth != 0 || cmpPos <= 0 || cmpPos >= thenPos - 1) error("SYNTAX ERROR");

            std::vector<std::string> left(tokens.begin(), tokens.begin() + cmpPos);
            std::vector<std::string> right(tokens.begin() + cmpPos + 1, tokens.begin() + thenPos);

            std::unique_ptr<Expression> lhs(parseExpressionFromTokens(left));
            std::unique_ptr<Expression> rhs(parseExpressionFromTokens(right));
            return new IfStatement(lhs.release(), tokens[cmpPos], rhs.release(), targetLine);
        }

        error("SYNTAX ERROR");
    } catch (ErrorException &) {
        error("SYNTAX ERROR");
    }
    return nullptr;
}
