/*
 * File: program.cpp
 * -----------------
 * Implementation of the program.h interface.
 */

#include <iostream>
#include "program.hpp"
#include "statement.hpp"
#include "Utils/error.hpp"

Program::Program() = default;

Program::~Program() {
    clear();
}

void Program::clear() {
    for (auto &entry : lines) {
        delete entry.second.stmt;
        entry.second.stmt = nullptr;
    }
    lines.clear();
    jumpPending = false;
    jumpTarget = -1;
    endFlag = false;
}

void Program::addSourceLine(int lineNumber, const std::string &line) {
    auto it = lines.find(lineNumber);
    if (it != lines.end()) {
        delete it->second.stmt;
        it->second.stmt = nullptr;
        it->second.source = line;
        return;
    }

    ProgramLine programLine;
    programLine.source = line;
    lines.emplace(lineNumber, programLine);
}

void Program::removeSourceLine(int lineNumber) {
    auto it = lines.find(lineNumber);
    if (it == lines.end()) return;
    delete it->second.stmt;
    lines.erase(it);
}

std::string Program::getSourceLine(int lineNumber) {
    auto it = lines.find(lineNumber);
    if (it == lines.end()) return "";
    return it->second.source;
}

void Program::setParsedStatement(int lineNumber, Statement *stmt) {
    auto it = lines.find(lineNumber);
    if (it == lines.end()) {
        delete stmt;
        error("SYNTAX ERROR");
    }
    delete it->second.stmt;
    it->second.stmt = stmt;
}

Statement *Program::getParsedStatement(int lineNumber) {
    auto it = lines.find(lineNumber);
    if (it == lines.end()) return nullptr;
    return it->second.stmt;
}

int Program::getFirstLineNumber() {
    if (lines.empty()) return -1;
    return lines.begin()->first;
}

int Program::getNextLineNumber(int lineNumber) {
    auto it = lines.upper_bound(lineNumber);
    if (it == lines.end()) return -1;
    return it->first;
}

void Program::list() const {
    for (const auto &entry : lines) {
        std::cout << entry.second.source << std::endl;
    }
}

void Program::run(EvalState &state) {
    jumpPending = false;
    jumpTarget = -1;
    endFlag = false;

    int lineNumber = getFirstLineNumber();
    while (lineNumber != -1) {
        Statement *stmt = getParsedStatement(lineNumber);
        int nextLine = getNextLineNumber(lineNumber);

        if (stmt != nullptr) {
            stmt->execute(state, *this);
        }

        if (endFlag) break;

        if (jumpPending) {
            if (getSourceLine(jumpTarget).empty()) {
                jumpPending = false;
                jumpTarget = -1;
                error("LINE NUMBER ERROR");
            }
            lineNumber = jumpTarget;
            jumpPending = false;
            jumpTarget = -1;
        } else {
            lineNumber = nextLine;
        }
    }

    jumpPending = false;
    jumpTarget = -1;
    endFlag = false;
}

void Program::setJumpTarget(int lineNumber) {
    jumpPending = true;
    jumpTarget = lineNumber;
}

void Program::setEndFlag() {
    endFlag = true;
}
