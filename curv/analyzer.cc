// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/phrase.h>
#include <curv/analyzer.h>
#include <curv/shared.h>
#include <curv/exception.h>
using namespace curv;

Shared<Expression>
curv::analyze_expr(Phrase& ph, const Environ& env)
{
    Shared<Meaning> m = analyze(ph, env);
    Expression* e = dynamic_cast<Expression*>(&*m);
    if (e == nullptr)
        throw Phrase_Error(ph, "not an expression");
    return Shared<Expression>(e);
}

Shared<Meaning>
curv::Identifier::analyze(const Environ& env) const
{
    Atom id(location().range());
    auto p = env.names.find(id);
    if (p != env.names.end())
        return aux::make_shared<Constant>(
            Shared<const Phrase>(this), p->second);
    else
        throw Phrase_Error(*this, stringify(id,": not defined"));
}

Shared<Meaning>
curv::Numeral::analyze(const Environ& env) const
{
    std::string str(location().range());
    char* endptr;
    double n = strtod(str.c_str(), &endptr);
    assert(endptr == str.c_str() + str.size());
    return aux::make_shared<Constant>(Shared<const Phrase>(this), n);
}

Shared<Meaning>
curv::Unary_Phrase::analyze(const Environ& env) const
{
    return aux::make_shared<Prefix_Expr>(
        Shared<const Phrase>(this),
        op_.kind,
        curv::analyze_expr(*arg_, env));
}

Shared<Meaning>
curv::Binary_Phrase::analyze(const Environ& env) const
{
    return aux::make_shared<Infix_Expr>(
        Shared<const Phrase>(this),
        op_.kind,
        curv::analyze_expr(*left_, env),
        curv::analyze_expr(*right_, env));
}

Shared<Meaning>
curv::Definition::analyze(const Environ& env) const
{
    throw Phrase_Error(*this, "not an expression");
}

Shared<Meaning>
curv::Paren_Phrase::analyze(const Environ& env) const
{
    if (args_.size() == 1
        && args_[0].comma_.kind == Token::k_missing)
    {
        return curv::analyze_expr(*args_[0].expr_, env);
    } else {
        throw Phrase_Error(*this, "not an expression");
    }
}

Shared<Meaning>
curv::List_Phrase::analyze(const Environ& env) const
{
    Shared<List_Expr> list =
        List_Expr::make(args_.size(), Shared<const Phrase>(this));
    for (size_t i = 0; i < args_.size(); ++i)
        (*list)[i] = analyze_expr(*args_[i].expr_, env);
    return list;
}

Shared<Meaning>
curv::Call_Phrase::analyze(const Environ& env) const
{
    auto fun = curv::analyze_expr(*function_, env);
    std::vector<Shared<Expression>> args;

    auto patom = dynamic_cast<Paren_Phrase*>(&*args_);
    if (patom != nullptr) {
        // argument phrase is a variable-length parenthesized argument list
        args.reserve(patom->args_.size());
        for (auto a : patom->args_)
            args.push_back(curv::analyze_expr(*a.expr_, env));
    } else {
        // argument phrase is a unitary expression
        args.reserve(1);
        args.push_back(curv::analyze_expr(*args_, env));
    }

    return aux::make_shared<Call_Expr>(
        Shared<const Phrase>(this),
        std::move(fun),
        args_,
        std::move(args));
}

void
analyze_definition(
    const Definition& def,
    Atom_Map<Shared<const Expression>>& dict,
    const Environ& env)
{
    auto id = dynamic_cast<const Identifier*>(def.left_.get());
    if (id == nullptr)
        throw Phrase_Error(*def.left_, "not an identifier");
    Atom name = id->location().range();
    if (dict.find(name) != dict.end())
        throw Phrase_Error(*def.left_, stringify(name, ": multiply defined"));
    dict[name] = curv::analyze_expr(*def.right_, env);
}

Shared<Meaning>
curv::Module_Phrase::analyze(const Environ& env) const
{
    auto self = Shared<const Phrase>(this);
    auto module = aux::make_shared<Module_Expr>(self);
    std::vector<Shared<Expression>> elements;

    for (auto st : stmts_) {
        const Definition* def =
            dynamic_cast<Definition*>(st.stmt_.get());
        if (def != nullptr) {
            analyze_definition(*def, module->fields_, env);
        } else {
            elements.push_back(curv::analyze_expr(*st.stmt_, env));
        }
    }

    module->elements_ = List_Expr::make_elements(std::move(elements), self);
    return module;
}

Shared<Meaning>
curv::Record_Phrase::analyze(const Environ& env) const
{
    Shared<Record_Expr> record =
        aux::make_shared<Record_Expr>(Shared<const Phrase>(this));
    for (auto i : args_) {
        const Definition* def =
            dynamic_cast<Definition*>(i.expr_.get());
        if (def != nullptr) {
            analyze_definition(*def, record->fields_, env);
        } else {
            throw Phrase_Error(*i.expr_, "not a definition");
        }
    }
    return record;
}