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

#include "alisp/config.hpp"
#include "alisp/alisp/alisp_module_helpers.hpp"
#include "alisp/alisp/alisp_object.hpp"

namespace nargs
{

using namespace alisp;

struct has
{
    inline static const std::string name{ "nargs-has" };

    inline static const std::string doc{ R"()" };

    inline static const Signature signature{ List{}, Sym{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {

        auto l          = arg_eval(eval, obj, 0);
        auto col_string = arg_eval(eval, obj, 1);


        return contains(l, col_string->to_string()) ? Qt : Qnil;
    }
};

struct next
{

    inline static const std::string name{ "nargs-next" };

    inline static const std::string doc{ R"()" };

    inline static const Signature signature{ List{}, Sym{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {

        auto l          = arg_eval(eval, obj, 0);
        auto col_string = arg_eval(eval, obj, 1);


        if (auto [it, succ] = get_next(l, col_string->to_string()); succ)
        {
            return it;
        }

        return Qnil;
    }
};

struct truthy
{

    inline static const std::string name{ "nargs-thruty" };

    inline static const std::string doc{ R"()" };

    inline static const Signature signature{ List{}, Sym{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {

        auto l          = arg_eval(eval, obj, 0);
        auto col_string = arg_eval(eval, obj, 1);


        if (auto [next, succ] = get_next(l, col_string->to_string()); succ)
        {
            return is_truthy(next) ? Qt : Qnil;
        }
        return Qnil;
    }
};

struct falsey
{

    inline static const std::string name{ "nargs-falsey" };

    inline static const std::string doc{ R"()" };

    inline static const Signature signature{ List{}, Sym{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {

        auto l          = arg_eval(eval, obj, 0);
        auto col_string = arg_eval(eval, obj, 1);


        if (auto [next, succ] = get_next(l, col_string->to_string()); succ)
        {
            return !is_truthy(next) ? Qt : Qnil;
        }
        return Qnil;
    }
};


}  // namespace nargs

ALISP_EXPORT alisp::env::ModulePtr init_nargs(alisp::env::Environment *, alisp::eval::Evaluator *)
{
    using namespace alisp;

    auto nargs_module = module_init("nargs");
    auto ngs_ptr      = nargs_module.get();

    module_defun(ngs_ptr, nargs::has::name, nargs::has::func, nargs::has::doc, nargs::has::signature.al());
    module_defun(ngs_ptr, nargs::next::name, nargs::next::func, nargs::next::doc, nargs::next::signature.al());
    module_defun(ngs_ptr, nargs::truthy::name, nargs::truthy::func, nargs::truthy::doc, nargs::truthy::signature.al());
    module_defun(ngs_ptr, nargs::falsey::name, nargs::falsey::func, nargs::falsey::doc, nargs::falsey::signature.al());

    return nargs_module;
}
