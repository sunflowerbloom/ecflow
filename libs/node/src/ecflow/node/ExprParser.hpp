/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_ExprParser_HPP
#define ecflow_node_ExprParser_HPP

#include <memory> // for unique_ptr
#include <string>

#include "ecflow/node/ExprAst.hpp"

/// This class will parse a expression and create the abstract syntax tree
/// It will own the AST unless specifically released calling ast();
class ExprParser {
public:
    explicit ExprParser(const std::string& expression);
    // Disable copy (and move) semantics
    ExprParser(const ExprParser&)                  = delete;
    const ExprParser& operator=(const ExprParser&) = delete;

    /// Parse the expression, return true if parse OK false otherwise
    /// if false is returned, and error message is returned
    bool doParse(std::string& errorMsg);

    /// return the Abstract syntax tree, and release memory
    std::unique_ptr<AstTop> ast() { return std::move(ast_); }

    /// return the Abstract syntax tree, without release memory
    AstTop* getAst() const { return ast_.get(); }

private:
    std::unique_ptr<AstTop> ast_;
    std::string expr_;
};

// This class was added to mitigate the slowness of the boost classic spirit parser
// we will recognise very simple expression, and bypass spirit. Very limited
// But the simple expression do form a very large subset
class SimpleExprParser {
public:
    explicit SimpleExprParser(const std::string& expression) : expr_(expression) {}
    // Disable copy (and move) semantics
    SimpleExprParser(const SimpleExprParser&)                  = delete;
    const SimpleExprParser& operator=(const SimpleExprParser&) = delete;

    /// Parse the expression, return true if parse OK false otherwise
    bool doParse();

    /// return the Abstract syntax tree, and release memory
    std::unique_ptr<AstTop> ast() { return std::move(ast_); }

private:
    const std::string& expr_;
    std::unique_ptr<AstTop> ast_;
};

#endif /* ecflow_node_ExprParser_HPP */
