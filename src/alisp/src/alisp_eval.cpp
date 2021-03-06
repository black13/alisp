/*   Alisp - the alisp interpreted language
     Copyright (C) 2020 Stanislav Arnaudov

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any prior version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */


#include "alisp/alisp/alisp_common.hpp"
#include "alisp/alisp/alisp_object.hpp"
#include "alisp/alisp/alisp_env.hpp"
#include "alisp/alisp/alisp_exception.hpp"
#include "alisp/alisp/alisp_factory.hpp"
#include "alisp/alisp/alisp_assertions.hpp"
#include "alisp/alisp/alisp_signature.hpp"

#include "alisp/utility.hpp"

#include <algorithm>

namespace alisp
{

namespace eval
{


void Evaluator::new_evaluation()
{
    ++m_eval_depth;
    if (m_eval_depth > MAX_EAVALUATION_DEPTH)
    {
        throw eval_error("Maximum evaluation depth reached!");
    }
}

void Evaluator::end_evaluation()
{
    --m_eval_depth;
}

Evaluator::Evaluator(env::Environment &env_, parser::ParserBase *t_parser, bool t_defer_el)
  : env(env_), m_eval_depth(0), m_catching_depth(0), m_parser(t_parser), m_async(this, t_defer_el), m_status_flags(0)
{

    m_lock = std::unique_lock<std::mutex>(callback_m, std::defer_lock);
}

Evaluator::~Evaluator()
{
    m_async.dispose();
}

void Evaluator::put_argument(const ALObjectPtr &param, ALObjectPtr arg)
{
    AL_DEBUG("Putting argument: "s += dump(param) + " -> " + dump(arg));

    this->env.put(param, arg);
}

void Evaluator::handle_argument_bindings(const ALObjectPtr &params,
                                         ALObjectPtr eval_args,
                                         std::function<void(ALObjectPtr, ALObjectPtr)> handler)
{

    AL_CHECK(if (params->length() == 0 && eval_args->length() != 0) {
        throw argument_error("Argument\'s lengths do not match.");
    });
    AL_CHECK(if (eval_args->length() != 0 && params->length() == 0) {
        throw argument_error("Argument\'s lengths do not match.");
    });
    AL_CHECK(if (eval_args->length() == 0 && params->length() == 0) { return; });


    auto next_argument = std::begin(*eval_args);
    auto next_param    = std::begin(*params);


    auto end_param = std::end(*params);

    auto arg_cnt = static_cast<ALObject::list_type::difference_type>(eval_args->length());

    ALObject::list_type::difference_type index = 0;
    bool rest                                  = false;
    bool opt                                   = false;
    bool prev_opt_or_rest                      = false;

    while (next_param != end_param)
    {
        if (*next_param == Qoptional)
        {
            opt              = true;
            prev_opt_or_rest = true;
            next_param       = std::next(next_param);
            continue;
        }
        else if (*next_param == Qrest)
        {
            rest             = true;
            prev_opt_or_rest = true;
            next_param       = std::next(next_param);
            continue;
        }
        else
        {

            if (rest)
            {
                handler(*next_param, splice(eval_args, index));
                return;
            }
            else if (index < arg_cnt)
            {
                handler(*next_param, *next_argument);
            }
            else if (!opt)
            {
                throw argument_error(
                  "The function requires more arguments than the provided "
                  "ones.");
            }
            else
            {
                handler(*next_param, Qnil);
            }

            ++index;
            prev_opt_or_rest = false;
            next_argument    = std::next(next_argument);
            next_param       = std::next(next_param);
        }
    }


    AL_CHECK(if (prev_opt_or_rest) { throw argument_error("The argument list ends with &optional or &rest."); });
    AL_CHECK(if (index < arg_cnt) { throw argument_error("Too many arguments provided for the function call."); });
}

ALObjectPtr Evaluator::eval(const ALObjectPtr &obj)
{
    detail::EvalDepthTrack track{ *this };

    if (is_falsy(obj)) return obj;

    switch (obj->type())
    {
        case ALObjectType::STRING_VALUE:

        case ALObjectType::REAL_VALUE:

        case ALObjectType::INT_VALUE: {
            return obj;
        }

        case ALObjectType::SYMBOL: {
            if (obj->to_string().front() == ':')
            {
                return obj;
            }
            AL_DEBUG("Evaluating symbol: "s += dump(obj));
            return env.find(obj);
        }

        case ALObjectType::LIST: {

            auto func = [this, &obj]() {
                if (pprime(obj))
                {
                    return obj;
                }
                else
                {
                    return eval(obj->i(0));
                }
            }();

            AL_DEBUG("Calling funcion: "s += dump(obj->i(0)));

            return eval_callable(func, splice(obj, 1), obj);
        }

        default: {
            eval_error("Unknown object typee");
        }
    }

    return nullptr;
}

ALObjectPtr Evaluator::eval_callable(const ALObjectPtr &callable, const ALObjectPtr &args, const ALObjectPtr &obj)
{
    auto func = callable;
    if (psym(func))
    {
        func = env.find(func);
    }

    AL_CHECK(if (!func->check_function_flag()) { throw eval_error("Head of a list must be bound to function"); });


#ifdef ENABLE_STACK_TRACE
    env::detail::CallTracer tracer{ env };
    if (obj->prop_exists("--line--"))
    {
        tracer.line(obj->get_prop("--line--")->to_int());
        tracer.file(obj->get_prop("--file--")->to_string());
    }
    if (func->prop_exists("--name--"))
    {
        tracer.function_name(func->get_prop("--name--")->to_string(), func->check_prime_flag());
    }
    else
    {
        tracer.function_name("anonymous", false);
    }
    tracer.catch_depth(m_catching_depth);
#endif


    try
    {

        if (func->check_prime_flag())
        {
            return apply_prime(func, args, obj);
        }
        else if (func->check_macro_flag())
        {
            env::detail::MacroCall fc{ env };
            auto expanded = apply_macro(func, args);
            AL_DEBUG("Macro expansion: "s += dump(expanded));
            return eval(expanded);
        }
        else
        {

            auto eval_args = [&]() {
                if (is_truthy(obj))
                {
                    return eval_transform(this, args);
                }
                else
                {
                    return args;
                }
            }();

            env::detail::FunctionCall fc{ env, func };
            return apply_function(func, eval_args);
        }
    }
    catch (al_continue &)
    {
        throw;
    }
    catch (al_break &)
    {
        throw;
    }
    catch (al_exit &)
    {
        throw;
    }
    catch (al_return &)
    {
        throw;
    }
    catch (interrupt_error &)
    {
        throw;
    }
    catch (...)
    {

#ifdef ENABLE_STACK_TRACE
        if (m_catching_depth == 0)
        {
            tracer.dump();
        }
#endif

        throw;
    }
}

ALObjectPtr Evaluator::apply_macro(const ALObjectPtr &func, const ALObjectPtr &args)
{
    auto [params, body] = func->get_function();
    handle_argument_bindings(params, args, [&](auto param, auto arg) { put_argument(param, arg); });
    return eval_list(this, body, 0);
}

ALObjectPtr Evaluator::apply_function(const ALObjectPtr &func, const ALObjectPtr &args)
{

    try
    {
        auto [params, body] = func->get_function();
        handle_argument_bindings(params, args, [&](auto param, auto arg) { put_argument(param, arg); });
        return eval_list(this, body, 0);
    }
    catch (al_return &ret)
    {
        return ret.value();
    }
}

ALObjectPtr Evaluator::apply_prime(const ALObjectPtr &func, const ALObjectPtr &args, const ALObjectPtr &)
{

    auto func_args = [&] {
        if (func->prop_exists("--managed--"))
        {
            auto eval_args = eval_transform(this, args);
            eval_args->set_prop("--evaled--", Qt);
            return eval_args;
        }

        args->set_prop("--evaled--", Qnil);
        return args;
    }();

    if (func->prop_exists("--signature--"))
    {
        size_t cnt                                     = 1;
        ALObject::list_type::difference_type opt_index = -1;
        auto signature                                 = func->get_prop("--signature--");
        auto opt_it                                    = std::find(signature->begin(), signature->end(), Qoptional);

        if (opt_it != std::end(*signature))
        {
            opt_index = std::distance(signature->begin(), opt_it);
        }

        handle_argument_bindings(signature, func_args, [&](const auto &param, auto arg) {
            if (opt_index != -1 and static_cast<ALObject::list_type::difference_type>(cnt) > opt_index + 1
                and arg != Qnil)
            {
                SignatureHandler::handle_signature_element(param, arg, cnt++, signature);
            }
        });
    }

    return func->get_prime()(func_args, &env, this);
}

ALObjectPtr Evaluator::eval_file(const std::string &t_file)
{
    const auto file_path = std::filesystem::absolute(t_file).string();
    AL_DEBUG("Evaluating file: "s += file_path);
    m_current_file    = t_file;
    auto file_content = utility::load_file(file_path);

    auto parse_result = m_parser->parse(file_content, file_path);

    if (parse_result.empty())
    {
        warn::warn_eval("Evaluating an empty file: "s + t_file);
        return Qnil;
    }

    auto res = Qt;
    for (auto sexp : parse_result)
    {
        res = eval(sexp);
    }
    return res;
}

ALObjectPtr Evaluator::eval_string(std::string &t_eval)
{
    AL_DEBUG("Evaluating string: "s += t_eval);

    if (t_eval.empty())
    {
        warn::warn_eval("Evaluating an empty string: ");
        return Qnil;
    }

    auto parse_result = m_parser->parse(t_eval, "--EVAL--");

    auto res = Qt;
    for (auto sexp : parse_result)
    {
        res = eval(sexp);
    }
    return res;
}

void Evaluator::handle_signal(int t_c)
{
    if (t_c == SIGINT)
    {
        AL_DEBUG("Handling a SIGINT"s);

        if (!AL_BIT_CHECK(m_status_flags, ACTIVE_EVALUATION_FLAG))
        {
            throw interrupt_error();
            return;
        }

        m_signal = t_c;
        AL_BIT_ON(m_status_flags, SIGINT_FLAG);
    }
    else if (t_c == SIGTERM)
    {
        AL_DEBUG("Handling a SIGTERM"s);

        m_signal = t_c;
        AL_BIT_ON(m_status_flags, SIGTERM_FLAG);
    }
}

void Evaluator::dispatch_callbacks()
{
    while (m_async.has_callback())
    {
        auto [func, args, internal] = m_async.next_callback();
        auto res                    = eval_callable(func, args);
        if (internal)
        {
            internal(res);
        }
        m_async.spin_loop();
    }
}

void Evaluator::check_status()
{
    if (AL_BIT_CHECK(m_status_flags, SIGTERM_FLAG))
    {
        AL_BIT_OFF(m_status_flags, SIGTERM_FLAG);
        throw al_exit(1);
    }

    if (AL_BIT_CHECK(m_status_flags, SIGINT_FLAG))
    {
        AL_BIT_OFF(m_status_flags, SIGINT_FLAG);
        throw interrupt_error();
    }
}

void Evaluator::eval_lippincott()
{
}

void Evaluator::set_current_file(std::string t_tile)
{
    m_current_file = std::move(t_tile);
}

void Evaluator::lock_evaluation()
{
    // m_lock.lock();
    callback_m.lock();
}

void Evaluator::unlock_evaluation()
{
    // m_lock.unlock();
    callback_m.unlock();
}

const std::string &Evaluator::get_current_file()
{
    return m_current_file;
}

detail::EvalDepthTrack::EvalDepthTrack(Evaluator &t_eval) : m_eval(t_eval)
{
    m_eval.check_status();
    m_eval.new_evaluation();
    m_eval.set_evaluation_flag();
}

detail::EvalDepthTrack::~EvalDepthTrack()
{
    m_eval.end_evaluation();
    if (m_eval.evaluation_depth() == 0)
    {
        m_eval.reset_evaluation_flag();
    }
}

detail::CatchTrack::CatchTrack(Evaluator &t_eval) : m_eval(t_eval)
{
    ++t_eval.m_catching_depth;
}

detail::CatchTrack::~CatchTrack()
{
    --m_eval.m_catching_depth;
}

detail::EvaluationLock::EvaluationLock(Evaluator &t_eval) : m_eval(t_eval)
{
    t_eval.lock_evaluation();
}

detail::EvaluationLock::~EvaluationLock()
{
    m_eval.unlock_evaluation();
}

}  // namespace eval

}  // namespace alisp
