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


#include <algorithm>

#include "alisp/alisp/alisp_common.hpp"
#include "alisp/alisp/alisp_env.hpp"
#include "alisp/alisp/alisp_eval.hpp"
#include "alisp/alisp/alisp_object.hpp"
#include "alisp/alisp/alisp_exception.hpp"
#include "alisp/alisp/alisp_declarations.hpp"
#include "alisp/alisp/alisp_assertions.hpp"

#include "alisp/utility/macros.hpp"


namespace alisp
{


ALObjectPtr Fand(ALObjectPtr obj, env::Environment *, eval::Evaluator *evl)
{
    auto eval_obj = eval_transform(evl, obj);
    bool sum      = reduce<false>(evl, eval_obj, AND_OBJ_FUN, true);
    return sum ? Qt : Qnil;
}

ALObjectPtr For(ALObjectPtr obj, env::Environment *, eval::Evaluator *evl)
{
    auto eval_obj = eval_transform(evl, obj);
    bool sum      = reduce<false>(evl, eval_obj, OR_OBJ_FUN, false);
    return sum ? Qt : Qnil;
}

ALObjectPtr Fnot(ALObjectPtr obj, env::Environment *, eval::Evaluator *evl)
{
    assert_size<1>(obj);

    bool sum = is_truthy(evl->eval(obj->i(0)));
    return !sum ? Qt : Qnil;
}


}  // namespace alisp
