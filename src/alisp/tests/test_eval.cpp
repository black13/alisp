#include "catch2/catch.hpp"

#include "alisp/alisp/alisp_common.hpp"
#include "alisp/alisp/alisp_parser.hpp"
#include "alisp/alisp/alisp_eval.hpp"
#include "alisp/alisp/alisp_env.hpp"

#include <string>
#include <vector>
#include <iostream>

using Catch::Matchers::Equals;
using namespace Catch::literals;


TEST_CASE("Evaluator Test [simple]", "[eval]")
{
    using namespace alisp;

    env::Environment env;
    eval::Evaluator eval(env);
    parser::ALParser<alisp::env::Environment> pars(env);

    SECTION ("int") {
        std::string input{"42"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->is_int() );
        CHECK ( res->to_int() == 42 );
    }

    SECTION ("double") {
        std::string input{"42.32"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->is_real() );
        CHECK ( res->to_real() == 42.32_a );
    }

    SECTION ("string") {

        std::string input{"\"string\""};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->is_string() );
        CHECK ( res->to_string().compare("string") == 0 );

    }

    SECTION ("sym") {
        std::string input{"nil"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->is_sym() );
    }

}


TEST_CASE("Evaluator Test [language]", "[eval]")
{
    using namespace alisp;

    env::Environment env;
    eval::Evaluator eval(env);
    parser::ALParser<alisp::env::Environment> pars(env);

    std::cout.setstate(std::ios_base::failbit);
    
    SECTION ( "defun" ) {

        std::string input{"(defun fun (a) \'a)"};
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( env.find(make_symbol("fun"))->check_function_flag() );

    }

    SECTION ( "defmacro" ) {


        std::string input{"(defmacro fun (a) \'a)"};
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( env.find(make_symbol("fun"))->check_macro_flag() );


    }

    SECTION ( "defvar" ) {

        std::string input{"(defvar a 42)"};
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( env.find(make_symbol("a"))->is_int() );

    }

    SECTION ( "setq" ) {

        std::string input{"(defvar a 42)"};
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        input = "(setq a 43)";
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( env.find(make_symbol("a"))->is_int() );
        CHECK ( env.find(make_symbol("a"))->to_int() == 43 );

    }

    SECTION ( "set" ) {

        std::string input{"(defvar a 42)"};
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        input = "(set \'a 44)";
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( env.find(make_symbol("a"))->is_int() );
        CHECK ( env.find(make_symbol("a"))->to_int() == 44 );

    }

    SECTION ( "quote" ) {
        std::string input{"'sym"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->is_sym() );
    }

    SECTION ( "function" ) {
        std::string input{"(function a)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->is_sym() );
    }

    SECTION ( "lambda" ) {

        std::string input{"(lambda (a) \'a)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->check_function_flag() );

    }

    SECTION ( "if[1]" ) {
        std::string input{"(if 'nil 42 32)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 32 );
    }

    SECTION ( "if[2]" ) {
        std::string input{"(if 't 42 32)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 42 );
    }

    SECTION ( "while" ) {
        std::string input{R"raw(
(let ((cnt 0))
  (while (< cnt 10)
    (println "cnt: " cnt)
    (setq cnt (+ 1 cnt)))))raw"};
        eval.eval(pars.parse(&input, "__TEST__")[0]);
    }

    SECTION ( "dolist" ) {
        std::string input{R"raw(
(dolist (element '(1 2 3 4 5 "s"))
  (println "element: " element)))raw"};
        eval.eval(pars.parse(&input, "__TEST__")[0]);
    }

    SECTION ( "cond" ) {
        std::string input{R"raw(
(cond
    ((== 10 11) "ten is eleven") 
    ((== 10 10) "ten is ten")
    ('t (println "final"))))raw"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->is_string() );
        CHECK ( res->to_string().compare("ten is ten") == 0 );           
    }

    SECTION ( "unless" ) {

        std::string input{"(unless 'nil 42 32)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 32 );
    }

    SECTION ( "when" ) {

        std::string input{"(when 't 42 32)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 32 );
    }

    SECTION ( "progn" ) {
        std::string input{"(progn 42 32)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 32 );
    }

    SECTION ( "let*" ) {
        std::string input{"(let* ((a 42) b) a)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 42 );

    }

    SECTION ( "let" ) {
        std::string input{"(let ((a 42) b) a)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 42 );
    }

    SECTION ( "funcall" ) {
        std::string input{R"raw((funcall + 7 3))raw"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 10 );
    }

    SECTION ( "signal" ) {}

    SECTION ( "exit" ) {}

    std::cout.clear();

}


TEST_CASE("Evaluator Test [math]", "[eval]")
{
    using namespace alisp;

    env::Environment env;
    eval::Evaluator eval(env);
    parser::ALParser<alisp::env::Environment> pars(env);

    SECTION ( "+" ) {
        std::string input{"(+ 10 10)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 20 );

    }
    SECTION ( "-" ) {

        std::string input{"(- 10 5)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 5 );

    }
    SECTION ( "/" ) {

        std::string input{"(/ 10 2)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 5 );

    }
    SECTION ( "*" ) {

        std::string input{"(* 10 10)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_int() == 100 );
    }


    SECTION ( "+ real" ) {
        std::string input{"(+ 10 10.4)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_real() == 20.4_a );

    }
    SECTION ( "- real" ) {

        std::string input{"(- 10.3 5)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_real() == 5.3_a );

    }
    SECTION ( "/ real" ) {

        std::string input{"(/ 2.7 10)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_real() == 0.27_a );

    }
    SECTION ( "* real" ) {

        std::string input{"(* 10.23 10)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( res->to_real() == 102.3_a );
    }

    SECTION ( ">" ) {

        std::string input{"(> 10 11)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( is_falsy(res) );


    }
    SECTION ( ">=" ) {


        std::string input{"(>= 10 11)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( is_falsy(res) );

    }
    SECTION ( "<" ) {
        std::string input{"(< 10 11)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( is_truthy(res) );
    }
    SECTION ( "<=" ) {
        std::string input{"(<= 10 11)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( is_truthy(res) );
    }
    SECTION ( "==" ) {
        std::string input{"(== 10 10)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( is_truthy(res) );
    }
    SECTION ( "!=" ) {
        std::string input{"(!= 11 11)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( is_falsy(res) );
    }

    SECTION ( "<<" ) {
        std::string input{"(<< 2 1)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK (res->is_int() );
        CHECK (res->to_int() == 4 );
    }


    SECTION ( ">>" ) {
        std::string input{"(>> 8 1)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK (res->is_int() );
        CHECK (res->to_int() == 4 );
    }

}


TEST_CASE("Evaluator Test [printing]", "[eval]")
{
    using namespace alisp;

    env::Environment env;
    eval::Evaluator eval(env);
    parser::ALParser<alisp::env::Environment> pars(env);

    std::cout.setstate(std::ios_base::failbit);
    
    SECTION ( "println,print" ) {
        std::string input{"(print 1)"};
        eval.eval(pars.parse(&input, "__TEST__")[0]);
        
        input = "(print 1.2)";
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        input = "(print \"sadasd\")";
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        input = "(println 1.2)";
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        input = "(println \"sadasd\")";
        eval.eval(pars.parse(&input, "__TEST__")[0]);


        input = "(println 1.2)";
        eval.eval(pars.parse(&input, "__TEST__")[0]);
        
    }

    SECTION ( "println,print" ) {
        std::string input{"(dump 1)"};
        eval.eval(pars.parse(&input, "__TEST__")[0]);
        
        input = "(dump 1.2)";
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        input = "(dump \"sadasd\")";
        eval.eval(pars.parse(&input, "__TEST__")[0]);

        input = "(dump 1.2)";
        eval.eval(pars.parse(&input, "__TEST__")[0]);
    }


    std::cout.clear();

}

TEST_CASE("Evaluator Test [logic]", "[eval]")
{
    using namespace alisp;

    env::Environment env;
    eval::Evaluator eval(env);
    parser::ALParser<alisp::env::Environment> pars(env);

    SECTION ( "or" ) {
        std::string input{"(or t nil)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( is_truthy(res) );
    }
    SECTION ( "and" ) {

        std::string input{"(and t nil)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( is_falsy(res) );
    }
    SECTION ( "not" ) {
        std::string input{"(not (and t nil))"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);

        CHECK ( is_truthy(res) );
    }

}



TEST_CASE("Evaluator Test [predicates]", "[eval]")
{
    using namespace alisp;

    env::Environment env;
    eval::Evaluator eval(env);
    parser::ALParser<alisp::env::Environment> pars(env);


    SECTION ( "pfunction" ) {
        std::string input{"(pfunction (lambda () 12))"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);
        CHECK ( is_truthy(res) );
    }
    SECTION ( "psym" ) {
        std::string input{"(psym 'sym)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);
        CHECK ( is_truthy(res) );
    }
    SECTION ( "plist" ) {
        std::string input{"(plist '(1 2 3 4))"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);
        CHECK ( is_truthy(res) );
    }
    SECTION ( "pint" ) {
        std::string input{"(pint 2)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);
        CHECK ( is_truthy(res) );
    }
    SECTION ( "preal" ) {
        std::string input{"(preal 2.3)"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);
        CHECK ( is_truthy(res) );
    }
    SECTION ( "pstring" ) {
        std::string input{"(pstring \"string\")"};
        auto res = eval.eval(pars.parse(&input, "__TEST__")[0]);
        CHECK ( is_truthy(res) );
    }

}


TEST_CASE("Evaluator Test [lists]", "[eval]")
{
    using namespace alisp;

    env::Environment env;
    eval::Evaluator eval(env);
    parser::ALParser<alisp::env::Environment> pars(env);

    SECTION ( "mapc" ) {}

}


TEST_CASE("Evaluator Test [function call]", "[eval]")
{
    using namespace alisp;

    env::Environment env;
    eval::Evaluator eval(env);
    parser::ALParser<alisp::env::Environment> pars(env);
    
    std::string input{"(defun fun (a &optional b &rest c) a) (fun 42)"};
    auto par_res = pars.parse(&input, "__TEST__");
    
    eval.eval(par_res[0]);
    auto res = eval.eval(par_res[1]);
        
    CHECK ( res->is_int() );
    CHECK ( res->to_int() == 42 );
}