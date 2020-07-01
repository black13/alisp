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


#include "alisp/alisp/alisp_module_helpers.hpp"
#include "alisp/alisp/alisp_asyncs.hpp"
#include "alisp/alisp/alisp_eval.hpp"

#include "alisp/alisp/async/timing.hpp"
#include "alisp/alisp/async/action.hpp"
#include "alisp/config.hpp"
#include "alisp/utility.hpp"

#include <filesystem>

namespace async_fileio
{

using namespace alisp;

namespace detail
{

struct read_file_text
{
    static constexpr bool managed    = true;
    static constexpr bool has_future = false;

    std::string g_file;
    ALObjectPtr callback;

    read_file_text(std::string t_file, ALObjectPtr t_callback)
      : g_file(std::move(t_file)), callback(std::move(t_callback))

    {
    }

    ALObjectPtr operator()(async::AsyncS *async) const
    {
        namespace fs = std::filesystem;

        if (!fs::exists(g_file))
        {
            async->submit_callback(callback, make_list(Qnil));
        }

        auto content = utility::load_file(g_file);
        async->submit_callback(callback, make_list(make_string(content)));

        return Qt;
    }
};

struct write_file_text
{
    static constexpr bool managed    = true;
    static constexpr bool has_future = false;

    std::string g_file;
    std::string g_content;
    bool g_append;
    ALObjectPtr callback;

    write_file_text(std::string t_file, std::string t_content, ALObjectPtr t_callback, bool t_append = false)
      : g_file(std::move(t_file)), g_content(std::move(t_content)), g_append(t_append), callback(std::move(t_callback))

    {
    }

    ALObjectPtr operator()(async::AsyncS *async) const
    {
        namespace fs = std::filesystem;

        if (!fs::exists(g_file))
        {
            async->submit_callback(callback, make_list(Qnil));
        }

        if (!fs::is_regular_file(g_file))
        {
            async->submit_callback(callback, make_list(Qnil));
        }

        utility::dump_file(g_file, g_content, g_append);

        async->submit_callback(callback, make_list(Qt));

        return Qt;
    }
};

}  // namespace detail

struct async_append_text
{
    inline static const std::string name{ "async-append-text" };

    inline static const std::string doc{ R"()" };

    inline static const Signature signature{ String{}, String{}, Function{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        assert_size<2>(obj);

        auto file_name    = AL_EVAL(obj, eval, 0);
        auto file_content = AL_EVAL(obj, eval, 1);
        auto callback     = AL_EVAL(obj, eval, 2);

        AL_CHECK(assert_string(file_name));
        AL_CHECK(assert_string(file_content));
        AL_CHECK(assert_function(callback));

        return async::dispatch<detail::write_file_text>(
          eval->async(), file_name->to_string(), file_content->to_string(), std::move(callback), true);
    }
};

struct async_write_text
{
    inline static const std::string name{ "async-write-text" };

    inline static const std::string doc{ R"()" };

    inline static const Signature signature{ String{}, String{}, Function{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        assert_size<2>(obj);

        auto file_name    = AL_EVAL(obj, eval, 0);
        auto file_content = AL_EVAL(obj, eval, 1);
        auto callback     = AL_EVAL(obj, eval, 2);

        AL_CHECK(assert_string(file_name));
        AL_CHECK(assert_string(file_content));
        AL_CHECK(assert_function(callback));

        return async::dispatch<detail::write_file_text>(
          eval->async(), file_name->to_string(), file_content->to_string(), std::move(callback), false);
    }
};

struct async_read_text
{
    inline static const std::string name{ "async-read-text" };

    inline static const std::string doc{ R"()" };

    inline static const Signature signature{ String{}, Function{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        assert_size<2>(obj);

        auto file_name = AL_EVAL(obj, eval, 0);
        auto callback  = AL_EVAL(obj, eval, 1);

        AL_CHECK(assert_string(file_name));
        AL_CHECK(assert_function(callback));

        auto file = file_name->to_string();

        return async::dispatch<detail::read_file_text>(eval->async(), file, std::move(callback));
    }
};


struct module_doc
{

    inline static const std::string doc{ R"()" };
};

}  // namespace async_fileio

ALISP_EXPORT alisp::env::ModulePtr init_async_fileio(alisp::env::Environment *, alisp::eval::Evaluator *)
{
    using namespace alisp;
    auto M       = alisp::module_init("async-fileio");
    auto aio_ptr = M.get();

    module_doc(aio_ptr, async_fileio::module_doc::doc);

    module_defun<async_fileio::async_append_text>(aio_ptr);
    module_defun<async_fileio::async_write_text>(aio_ptr);
    module_defun<async_fileio::async_read_text>(aio_ptr);

    return M;
}