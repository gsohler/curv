#include <gtest/gtest.h>
#include <curv/parse.h>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/phrase.h>
#include <curv/string.h>
#include <curv/analyzer.h>
#include <cstdlib>

using namespace std;
using namespace aux;
using namespace curv;

struct CString_Script : public curv::Script
{
    const char* buffer_;

    CString_Script(const char* name, const char* buffer)
    :
        curv::Script(name, buffer, buffer + strlen(buffer)),
        buffer_(buffer)
    {}
};

struct Evaluator
{
    Evaluator(const char* source)
    :
        script_(aux::make_shared<CString_Script>("", source)),
        failure_(nullptr),
        success_(nullptr)
    {
        try {
          #if 0
            auto phrase = curv::parse(*script_);
            if (phrase == nullptr) { // empty expression
                success_ = "";
                return;
            }
            const curv::Definition* def =
                dynamic_cast<curv::Definition*>(phrase.get());
            if (def != nullptr) {
                failure_ = "definition found; expecting expression";
                return;
            }
            Environ env(curv::builtin_namespace);
            auto expr = analyze_expr(*phrase, env);
            success_value_ = curv::eval(*expr);
          #else
            Shared<Module> module{eval_script(*script_, builtin_namespace)};
            if (!module->fields_.empty()) {
                failure_ = "definitions found; expecting only expressions";
                return;
            }
            if (module->elements_->empty()) {
                success_ = "";
                return;
            }
            if (module->elements_->size() > 1) {
                failure_ = "multiple values found";
                return;
            }
            success_value_ = (*module->elements_)[0];
          #endif
            success_str_ = curv::stringify(success_value_);
            success_ = success_str_->c_str();
        } catch (curv::Exception& e) {
            failure_str_ = e.shared_what();
            failure_ = failure_str_->c_str();
        } catch (std::exception& e) {
            failure_str_ = mk_string(e.what());
            failure_ = failure_str_->c_str();
        }
    }

    aux::Shared_Ptr<CString_Script> script_;

    const char* failure_;
    aux::Shared_Ptr<curv::String> failure_str_;

    const char* success_;
    curv::Value success_value_;
    aux::Shared_Ptr<curv::String> success_str_;
};

testing::AssertionResult
evaltest(const char* expr, const char* expected, bool should_succeed)
{
    Evaluator result(expr);
    if (result.failure_) {
        // the test failed
        if (should_succeed) {
            return testing::AssertionFailure()
                << "in expr '" << expr << "'\n"
                << "expected value: '" << expected << "'\n"
                << "  actual error: '" << result.failure_ << "'\n";
        } else {
            if (strcmp(expected, result.failure_) == 0) {
                return testing::AssertionSuccess();
            } else {
                return testing::AssertionFailure()
                    << "in expr '" << expr << "'\n"
                    << "expected error: '" << expected << "'\n"
                    << "  actual error: '" << result.failure_ << "'\n";
            }
        }
    } else {
        // the test succeeded
        if (should_succeed) {
            if (strcmp(expected, result.success_) == 0) {
                return testing::AssertionSuccess();
            } else {
                return testing::AssertionFailure()
                    << "in expr '" << expr << "'\n"
                    << "expected value: '" << expected << "'\n"
                    << "  actual value: '" << result.success_ << "'\n";
            }
        } else {
            return testing::AssertionFailure()
                << "in expr '" << expr << "'\n"
                << "expected error: '" << expected << "'\n"
                << "  actual value: '" << result.success_ << "'\n";
        }
    }
}

testing::AssertionResult
evalgood(
    const char* e1, const char* e2,
    const char* expr, const char* expected)
{
    return evaltest(expr, expected, true);
}

testing::AssertionResult
evalbad(
    const char* e1, const char* e2,
    const char* expr, const char* expected)
{
    return evaltest(expr, expected, false);
}

int reps()
{
    const char* rs = getenv("REPS");
    if (rs == nullptr)
        return 1;
    int r = atoi(rs);
    if (r <= 0)
        return 1;
    return r;
}

#define EVALS_TO(expr,result) EXPECT_PRED_FORMAT2(evalgood,expr,result)
#define EVAL_ERROR(expr,result) EXPECT_PRED_FORMAT2(evalbad,expr,result)

TEST(curv, eval)
{
  int r = reps();
  for (int i = 0; i < r; ++i) {
    // constructors
    EVALS_TO("42.7", "42.7");
    EVALS_TO(".1", "0.1");
    EVALS_TO("1.", "1");
    EVALS_TO(".1e-1", "0.01");
    EVALS_TO("1.e+1", "10");
    EVALS_TO("1e1", "10");
    EVALS_TO("[1,2,3]", "[1,2,3]");
    EVALS_TO("{x=1}", "{x=1}");

    // builtins
    EVALS_TO("pi",  "3.141592653589793");
    EVALS_TO("tau", "6.283185307179586");
    EVALS_TO("inf", "inf");
    EVALS_TO("null", "null");
    EVALS_TO("false", "false");
    EVALS_TO("true", "true");
    EVALS_TO("sqrt", "<function>");

    // runtime operations
    EVALS_TO("-0", "-0");
    EVALS_TO("-inf", "-inf");
    EVAL_ERROR("1+null", "1+null: domain error");
    EVAL_ERROR("0/0", "0/0: domain error");
    EVALS_TO("1/0", "inf");
    EVALS_TO("sqrt(2)", "1.4142135623730951");
    EVALS_TO("sqrt(4,)", "2"); // test syntax: trailing , after last argument
    EVALS_TO("sqrt sqrt 16", "2");
    EVAL_ERROR("sqrt(true)", "sqrt(true): domain error");

    // lexical errors
    EVAL_ERROR("\\foo", "illegal character '\\'");
    EVAL_ERROR("\177", "illegal character 0x7F");
    EVAL_ERROR("42e+", "bad numeral");

    // analysis errors
    EVAL_ERROR("fnord", "fnord: not defined");
    EVAL_ERROR("{x=1,x=2}", "x: multiply defined");
  }
}