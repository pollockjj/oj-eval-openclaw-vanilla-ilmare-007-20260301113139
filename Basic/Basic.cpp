/*
 * File: Basic.cpp
 * ---------------
 * Main loop and command processing for the BASIC interpreter.
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include "program.hpp"
#include "statement.hpp"
#include "Utils/error.hpp"
#include "Utils/tokenScanner.hpp"

void processLine(std::string line, Program &program, EvalState &state);

int main() {
    EvalState state;
    Program program;

    while (true) {
        try {
            std::string input;
            if (!getline(std::cin, input)) break;
            if (input.empty()) continue;
            processLine(input, program, state);
        } catch (ErrorException &ex) {
            std::cout << ex.getMessage() << std::endl;
        }
    }
    return 0;
}

void processLine(std::string line, Program &program, EvalState &state) {
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(line);

    if (!scanner.hasMoreTokens()) return;

    std::string firstToken = scanner.nextToken();

    if (scanner.getTokenType(firstToken) == NUMBER) {
        int lineNumber;
        try {
            lineNumber = std::stoi(firstToken);
        } catch (...) {
            error("SYNTAX ERROR");
        }

        if (!scanner.hasMoreTokens()) {
            program.removeSourceLine(lineNumber);
            return;
        }

        std::unique_ptr<Statement> stmt(parseStatement(scanner, true));
        program.addSourceLine(lineNumber, line);
        program.setParsedStatement(lineNumber, stmt.release());
        return;
    }

    if (firstToken == "RUN") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        program.run(state);
        return;
    }

    if (firstToken == "LIST") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        program.list();
        return;
    }

    if (firstToken == "CLEAR") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        program.clear();
        state.Clear();
        return;
    }

    if (firstToken == "QUIT") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        std::exit(0);
    }

    if (firstToken == "HELP") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        std::cout << "Commands: LET PRINT INPUT RUN LIST CLEAR QUIT" << std::endl;
        return;
    }

    scanner.saveToken(firstToken);
    std::unique_ptr<Statement> stmt(parseStatement(scanner, false));
    stmt->execute(state, program);
}
