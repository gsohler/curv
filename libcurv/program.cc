// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/analyser.h>
#include <libcurv/definition.h>
#include <libcurv/builtin.h>
#include <libcurv/program.h>
#include <libcurv/parser.h>
#include <libcurv/scanner.h>
#include <libcurv/system.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>

namespace curv {

void
Program::compile(const Namespace* names)
{
    if (names == nullptr)
        names_ = &system_.std_namespace();
    else
        names_ = names;

    phrase_ = parse_program(scanner_);

    Builtin_Environ env{*names_, system_, scanner_.eval_frame_};
    if (auto def = phrase_->as_definition(env)) {
        module_ = analyse_module(*def, env);
    } else {
        meaning_ = phrase_->analyse(env);
    }

    frame_ = {Frame::make(env.frame_maxslots_,
        system_, scanner_.eval_frame_, nullptr, nullptr)};
}

const Phrase&
Program::nub()
const
{
    return *nub_phrase(phrase_);
}

Location
Program::location()
const
{
    if (phrase_ == nullptr) {
        return Location{
            *scanner_.source_,
            Token{unsigned(scanner_.ptr_ - scanner_.source_->begin()),
                  unsigned(scanner_.source_->size())}};
    } else {
        return nub_phrase(phrase_)->location();
    }
}

Value
Program::eval()
{
    if (module_ != nullptr) {
        throw Exception(At_Phrase(*phrase_, scanner_.eval_frame_),
            "definition found; expecting an expression");
    } else {
        auto expr = meaning_->to_operation(scanner_.eval_frame_);
        return expr->eval(*frame_);
    }
}

std::pair<Shared<Module>, Shared<List>>
Program::denotes()
{
    Shared<Module> module = nullptr;
    Shared<List> list = nullptr;
    if (module_) {
        module = module_->eval_module(*frame_);
    } else {
        List_Builder lb;
        auto gen = meaning_->to_operation(scanner_.eval_frame_);
        gen->generate(*frame_, lb);
        list = lb.get_list();
    }
    return {module, list};
}

} // namespace curv
