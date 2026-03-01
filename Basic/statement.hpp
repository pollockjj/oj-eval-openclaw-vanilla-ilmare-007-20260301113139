/*
 * File: statement.h
 * -----------------
 * This file defines the Statement abstract type and statement subclasses.
 */

#ifndef _statement_h
#define _statement_h

#include <string>
#include "evalstate.hpp"
#include "exp.hpp"
#include "Utils/tokenScanner.hpp"

class Program;

class Statement {
public:
    Statement();
    virtual ~Statement();
    virtual void execute(EvalState &state, Program &program) = 0;
};

class RemStatement : public Statement {
public:
    RemStatement();
    void execute(EvalState &state, Program &program) override;
};

class LetStatement : public Statement {
public:
    LetStatement(std::string varName, Expression *exp);
    ~LetStatement() override;
    void execute(EvalState &state, Program &program) override;

private:
    std::string varName;
    Expression *exp;
};

class PrintStatement : public Statement {
public:
    explicit PrintStatement(Expression *exp);
    ~PrintStatement() override;
    void execute(EvalState &state, Program &program) override;

private:
    Expression *exp;
};

class InputStatement : public Statement {
public:
    explicit InputStatement(std::string varName);
    void execute(EvalState &state, Program &program) override;

private:
    std::string varName;
};

class EndStatement : public Statement {
public:
    EndStatement();
    void execute(EvalState &state, Program &program) override;
};

class GotoStatement : public Statement {
public:
    explicit GotoStatement(int targetLineNumber);
    void execute(EvalState &state, Program &program) override;

private:
    int targetLineNumber;
};

class IfStatement : public Statement {
public:
    IfStatement(Expression *lhs, std::string op, Expression *rhs, int targetLineNumber);
    ~IfStatement() override;
    void execute(EvalState &state, Program &program) override;

private:
    Expression *lhs;
    std::string op;
    Expression *rhs;
    int targetLineNumber;
};

Statement *parseStatement(TokenScanner &scanner, bool inProgramLine);

#endif
