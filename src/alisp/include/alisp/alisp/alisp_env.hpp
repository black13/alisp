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


#pragma once

#include <unordered_map>
#include <vector>
#include <array>
#include <iostream>
#include <string_view>
#include <string>

#include "alisp/alisp/alisp_common.hpp"
#include "alisp/alisp/alisp_macros.hpp"
#include "alisp/alisp/alisp_exception.hpp"
#include "alisp/alisp/alisp_eval.hpp"
#include "alisp/alisp/alisp_modules.hpp"
#include "alisp/alisp/alisp_loadable_modules.hpp"
#include "alisp/alisp/alisp_warnings.hpp"

#include "alisp/utility/helpers.hpp"
#include "alisp/utility/macros.hpp"

namespace alisp::eval
{
class Evaluator;
}

namespace alisp
{


namespace env
{

extern ALObjectPtr intern(std::string name);

extern void update_prime(ALObjectPtr t_sym, ALObjectPtr t_val);


namespace detail
{

struct CellStack
{
  public:
    using Scope = std::unordered_map<std::string, ALObjectPtr>;
    // using StackFrame = std::vector<std::pair<, std::vector<std::function<void()>>>>;
    using StackFrame = std::vector<Scope>;
    using Stack      = std::vector<StackFrame>;

    CellStack() { push_frame(); }

    void push_frame() { stacks.emplace_back(1); }
    void pop_frame() { stacks.pop_back(); }

    void push_scope() { stacks.back().emplace_back(); }
    void pop_scope() { stacks.back().pop_back(); }

    // Scope& root_scope(){ return stacks.front().front(); }
    StackFrame &root_frame() { return stacks.front(); }

    StackFrame &current_frame() { return stacks.back(); }
    Scope &current_scope() { return stacks.back().back(); }

    Stack stacks;
};

}  // namespace detail

using AlispDynModulePtr = std::unique_ptr<dynmoduels::AlispDynModule>;

class Module;

class Environment;

using ModulePtr = std::shared_ptr<Module>;

class Module
{
  public:
  private:
    detail::CellStack::Scope m_root_scope;
    std::unordered_map<std::string, ModulePtr> m_modules;
    std::string m_name;
    std::vector<std::string> m_evals;
    std::vector<ALObjectPtr> m_eval_obj;

  public:
    Module(std::string t_name) : m_name(std::move(t_name)) {}

    detail::CellStack::Scope &root_scope() { return m_root_scope; }

    std::vector<std::string> &eval_strings() { return m_evals; }
    std::vector<ALObjectPtr> &eval_objs() { return m_eval_obj; }

    void eval_string(std::string t_eval) { m_evals.push_back(std::move(t_eval)); }
    void eval_obj(ALObjectPtr t_obj) { m_eval_obj.push_back(std::move(t_obj)); }


    const std::string &name() { return m_name; }

    void add_module(ModulePtr t_module, const std::string &t_alias)
    {
        m_modules.insert({ std::move(t_alias), t_module });
    }

    detail::CellStack::Scope &get_root() { return m_root_scope; }

    bool has_symbol(const std::string &t_name) { return m_root_scope.count(t_name) != 0; }
    bool has_module(const std::string &t_name) { return m_modules.count(t_name) != 0; }

    ALObjectPtr get_symbol(const std::string &t_name)
    {
        auto sym = m_root_scope.find(t_name);
        if (sym != std::end(m_root_scope))
        {
            return sym->second;
        };
        warn::warn_env(("Referencing non existent symbol "s += t_name) += " in module"s += m_name);
        return nullptr;
    }

    Module *get_module(const std::string &t_name)
    {
        auto mod = m_modules.find(t_name);
        if (mod != std::end(m_modules))
        {
            return mod->second.get();
        };
        warn::warn_env("Referencing non existen module:"s += t_name);
        return nullptr;
    }
};

struct ModuleImport
{
    using module_import = ModulePtr (*)(env::Environment *env, eval::Evaluator *eval);
    ModulePtr (*function)(env::Environment *env, eval::Evaluator *eval);
};

class Environment
{

  public:
    static inline std::unordered_map<std::string, ALObjectPtr> g_symbol_table;
    static inline std::unordered_map<std::string, ALObjectPtr> g_global_symbol_table;
    static inline std::unordered_map<std::string, ALObjectPtr> g_prime_values;

    static inline std::unordered_map<std::string, ModuleImport> g_builtin_modules;

  private:
    detail::CellStack m_stack;
    std::unordered_map<std::string, ModulePtr> m_modules;
    std::unordered_map<std::string, AlispDynModulePtr> m_loaded_modules;
    std::reference_wrapper<Module> m_active_module;

    size_t m_call_depth;

    std::vector<std::tuple<size_t, size_t, std::function<void()>>> m_deferred_calls;

#ifdef ENABLE_STACK_TRACE
    std::vector<std::tuple<std::string, bool>> m_stack_trace;  // name, is_prime
    size_t m_unwind_defers{ 0 };
#endif

  public:
    Environment()
      : m_modules{ { "--main--", std::make_shared<Module>("--main--") } }
      , m_active_module({ *m_modules.at("--main--").get() })
      , m_call_depth(0)
    {
    }

    ~Environment() { g_symbol_table.clear(); }

    void define_module(const std::string t_name, const std::string)
    {
        AL_DEBUG("Creating a new module: "s += t_name);
        auto new_mod = std::make_shared<Module>(t_name);
        m_modules.insert({ std::move(t_name), new_mod });
    }

    void define_module(const std::string t_name, ModulePtr t_mod)
    {
        AL_DEBUG("Adding a new module: "s += t_name);
        m_modules.insert({ std::move(t_name), std::move(t_mod) });
    }

    void load_module(eval::Evaluator *eval, const std::string t_file, const std::string &t_name);

    void alias_module(const std::string &t_name, const std::string &t_alias)
    {
        if (m_modules.find(t_name) == m_modules.end())
        {
            AL_CHECK(warn::warn_env("Referencing non existen module:"s += t_name););
            return;
        }
        AL_DEBUG("Aliasing a module: "s += t_name + " -> " + t_alias);
        m_active_module.get().add_module(m_modules.at(t_name), t_alias);
    }

    void activate_module(const std::string &t_name);

    const std::string &current_module() { return m_active_module.get().name(); }

    Module &current_module_ref() { return m_active_module; }

    Module *get_module(const std::string &t_name)
    {
        auto mod = m_modules.find(t_name);
        if (mod != std::end(m_modules))
        {
            return mod->second.get();
        };
        warn::warn_env("Referencing non existen module:"s += t_name);
        return nullptr;
    }

    bool module_loaded(const std::string &t_module_name) { return m_modules.count(t_module_name) != 0; }

    void import_root_scope(const std::string &t_from, const std::string &t_to)
    {

        AL_DEBUG("Importing module symbols: "s += t_from + " -> " + t_to);
        auto &from_root = m_modules.at(t_from)->get_root();
        auto &to_root   = m_modules.at(t_to)->get_root();

        if (from_root.empty())
        {
            warn::warn_import("Importing an empty root scope.");
        }

        for (auto &[name, sym] : from_root)
        {
            AL_DEBUG("Adding a symbol: "s += name);
            if (!sym->prop_exists("--module--"))
            {
                continue;
            }
            if (sym->get_prop("--module--")->to_string().compare(t_from) == 0)
            {
                to_root.insert({ name, sym });
            }
        }

        // m_modules.at(t_to)->get_root() = m_modules.at(t_from)->get_root();
    }

    bool load_builtin_module(const std::string &t_module_name, eval::Evaluator *eval);

    ALObjectPtr find(const ALObjectPtr t_sym);

    void define_variable(const ALObjectPtr t_sym, ALObjectPtr t_value, std::string t_doc = {});

    void define_function(const ALObjectPtr t_sym, ALObjectPtr t_params, ALObjectPtr t_body, std::string t_doc = {});

    void define_macro(const ALObjectPtr t_sym, ALObjectPtr t_params, ALObjectPtr t_body, std::string t_doc = {});

    void put(const ALObjectPtr t_sym, ALObjectPtr t_val);

    void update(const ALObjectPtr t_sym, ALObjectPtr t_value);

    void defer_callback(std::function<void()> t_callback);

    void resolve_callbacks();

    void new_scope() { m_stack.push_scope(); }

    void destroy_scope() { m_stack.pop_scope(); }

    void call_function()
    {
        ++m_call_depth;
        if (m_call_depth > MAX_FUNCTION_CALL_DEPTH)
        {
            throw environment_error("Maximum function calldepth reached!");
        }
        m_stack.push_frame();
    }

    void finish_function()
    {
        --m_call_depth;
        m_stack.pop_frame();
    }

    size_t call_depth() { return m_call_depth; }

    bool in_function() { return m_call_depth != 0; }

    bool in_root() { return (!in_function()) && (std::size(m_stack.root_frame()) == 1); }

    void stack_dump() const;

    void callstack_dump() const;

    void env_dump() const;

    auto get_modules() const { return m_modules; }

#ifdef ENABLE_STACK_TRACE

    void trace_call(std::string t_trace, bool is_prime = false)
    {
        m_stack_trace.push_back({ std::move(t_trace), is_prime });
    }

    void trace_unwind()
    {
        if (m_unwind_defers > 0)
        {

            for (size_t i = 0; i < m_unwind_defers; ++i)
            {
                if (!std::empty(m_stack_trace))
                {
                    m_stack_trace.pop_back();
                }
            }
            m_unwind_defers = 0;
        }

        if (!std::empty(m_stack_trace))
        {
            m_stack_trace.pop_back();
            return;
        }

        warn::warn_env("Unwinding an empty stack.");
    }

    void defer_unwind() { ++m_unwind_defers; }

    auto get_stack_trace() -> auto & { return m_stack_trace; }

#endif
};


namespace detail
{

#ifdef ENABLE_STACK_TRACE

struct CallTracer
{
  private:
    std::string m_function;
    std::string m_args;
    bool m_is_prime      = false;
    size_t m_line        = 0;
    size_t m_catch_depth = 0;

  public:
    explicit CallTracer(Environment &t_env) : m_env(t_env) {}

    ~CallTracer()
    {
        if (m_catch_depth == 0)
        {
            m_env.trace_unwind();
        }
        else
        {
            m_env.defer_unwind();
        }
    }

    void line(ALObject::int_type t_line) { m_line = static_cast<size_t>(t_line); }

    void catch_depth(size_t t_catch_depth) { m_catch_depth = t_catch_depth; }

    void function_name(std::string t_func, bool t_is_prime = false)
    {
        m_is_prime = t_is_prime;
        m_function = std::move(t_func);
        m_env.trace_call("(" + m_function + "):" + std::to_string(m_line), t_is_prime);
    }

    void dump()
    {

        if (ALISP_UNLIKELY(!std::empty(m_env.get_stack_trace())))
        {
            m_env.callstack_dump();
            m_env.get_stack_trace().clear();
        }
        else
        {
            return;
        }
    }


    CallTracer(CallTracer &&) = default;
    CallTracer &operator=(CallTracer &&) = default;
    CallTracer(const CallTracer &)       = delete;
    CallTracer &operator=(const CallTracer &) = delete;


  private:
    Environment &m_env;
};

#endif

struct FunctionCall
{
  public:
    explicit FunctionCall(Environment &t_env, ALObjectPtr t_func) : m_env(t_env)
    {
        m_env.call_function();

        if (t_func->prop_exists("--module--"))
        {
            auto func_module = t_func->get_prop("--module--")->to_string();
            if (m_env.current_module().compare(func_module) != 0)
            {
                m_prev_mod = m_env.current_module();
                t_env.activate_module(func_module);
            }

            return;
        }

        warn::warn_env("Calling a function without --module-- property.");
    }
    ~FunctionCall()
    {
        if (!m_prev_mod.empty())
        {
            m_env.activate_module(m_prev_mod);
        }
        m_env.finish_function();
    }

    FunctionCall(FunctionCall &&) = default;
    FunctionCall &operator=(FunctionCall &&) = default;
    FunctionCall(const FunctionCall &)       = delete;
    FunctionCall &operator=(const FunctionCall &) = delete;


  private:
    Environment &m_env;
    std::string m_prev_mod;
};

struct ScopePushPop
{
  public:
    explicit ScopePushPop(Environment &t_env) : m_env(t_env) { m_env.new_scope(); }
    ~ScopePushPop()
    {
        m_env.resolve_callbacks();
        m_env.destroy_scope();
    }

    ALISP_RAII_OBJECT(ScopePushPop);

  private:
    Environment &m_env;
};

struct MacroCall
{
  public:
    explicit MacroCall(Environment &t_env) : m_env(t_env) { m_env.new_scope(); }
    ~MacroCall() { m_env.destroy_scope(); }

    ALISP_RAII_OBJECT(MacroCall);

  private:
    Environment &m_env;
};

struct ModuleChange
{
  private:
    Environment &m_env;
    const std::string &m_prev_mod;

  public:
    ModuleChange(Environment &t_env, const std::string &t_module) : m_env(t_env), m_prev_mod(m_env.current_module())
    {

        m_env.activate_module(t_module);
    }

    ~ModuleChange() { m_env.activate_module(m_prev_mod); }

    ModuleChange(ModuleChange &&) = default;
    ModuleChange &operator=(ModuleChange &&) = default;
    ModuleChange(const ModuleChange &)       = delete;
    ModuleChange &operator=(const ModuleChange &) = delete;

    const std::string &old_module() const { return m_prev_mod; }
};

}  // namespace detail


}  // namespace env


}  // namespace alisp
