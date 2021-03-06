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
#include "alisp/utility/defines.hpp"
#include "alisp/utility/files.hpp"
#include "alisp/utility/string_utils.hpp"

#include <filesystem>
#include <glob.h>
#include <string.h>
#include <fmt/format.h>

namespace alisp
{

auto fileio_signal = alisp::make_symbol("fileio-signal");

namespace detail
{

std::vector<std::string> glob(const std::string &pattern)
{
    using namespace std;


    // glob struct resides on the stack
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));

    // do the glob operation
    int return_value = glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
    if (return_value != 0)
    {
        globfree(&glob_result);
        stringstream ss;
        ss << "glob() failed with return_value " << return_value << endl;
        throw std::runtime_error(ss.str());
    }

    // collect all the filenames into a std::list<std::string>
    vector<string> filenames;
    for (size_t i = 0; i < glob_result.gl_pathc; ++i)
    {
        filenames.push_back(string(glob_result.gl_pathv[i]));
    }

    // cleanup
    globfree(&glob_result);

    // done
    return filenames;
}

std::string expand_user(std::string path)
{
    if (not path.empty() and path[0] == '~')
    {
        assert(path.size() == 1 or path[1] == '/');  // or other error handling
        char const *home = getenv("HOME");
        if (home or ((home = getenv("USERPROFILE"))))
        {
            path.replace(0, 1, home);
        }
        else
        {
            char const *hdrive = getenv("HOMEDRIVE"), *hpath = getenv("HOMEPATH");
            assert(hdrive);  // or other error handling
            assert(hpath);
            path.replace(0, 1, std::string(hdrive) + hpath);
        }
    }
    return path;
}

#ifdef ALISP_WIN
inline constexpr auto separator = "\\";
#else
inline constexpr auto separator = "/";
#endif


struct root
{
    inline static const std::string name{ "f-root" };

    inline static const std::string doc{ R"((f-root)

Return absolute root.
)" };

    inline static const Signature signature{};


    static ALObjectPtr func(const ALObjectPtr &, env::Environment *, eval::Evaluator *)
    {
        namespace fs = std::filesystem;

        try
        {
            return make_string(fs::current_path().root_path());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
        return Qnil;
    }
};

struct directories
{
    inline static const std::string name{ "f-directories" };

    inline static const std::string doc{ R"((f-directories PATH)

Find all directories in `PATH`.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        ALObject::list_type entries;

        try
        {
            for (auto &entr : fs::directory_iterator(path->to_string()))
            {
                if (!entr.is_directory())
                {
                    continue;
                }
                entries.push_back(make_string(entr.path().string()));
            }
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }

        return make_object(entries);
    }
};

struct entries
{

    inline static const std::string name{ "f-entries" };

    inline static const std::string doc{ R"((f-entries PATH)

Find all files and directories in `PATH`.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        ALObject::list_type entries;

        try
        {
            for (auto &entr : fs::directory_iterator(path->to_string()))
            {
                entries.push_back(make_string(entr.path().string()));
            }
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }

        return make_object(entries);
    }
};

struct Sglob
{

    inline static const std::string name{ "f-glob" };

    inline static const std::string doc{ R"((f-glob PATTERN PATH)

Find `PATTERN` in `PATH`.
)" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto pattern = arg_eval(eval, obj, 0);

        if (std::size(*obj) > 1)
        {
            auto path = arg_eval(eval, obj, 1);
            return make_list(glob(path->to_string() + fs::path::preferred_separator + pattern->to_string()));
        }

        return make_list(glob(pattern->to_string()));
    }
};

struct touch
{

    inline static const std::string name{ "f-touch" };

    inline static const std::string doc{ R"((f-touch PATH)

Update `PATH` last modification date or create if it does not exist.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        auto path = arg_eval(eval, obj, 0);

        std::fstream fs;
        fs.open(path->to_string(), std::ios::out | std::ios::app);
        if (!fs.is_open())
        {
            return Qnil;
        }
        fs.close();

        return Qt;
    }
};

struct Sexpand_user
{
    inline static const std::string name{ "f-expand-user" };

    inline static const std::string doc{ R"((f-expand-user PATH)

For unix systems, expand `~` to the location of the home directory of
the current user.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        auto path = arg_eval(eval, obj, 0);

        return make_string(expand_user(path->to_string()));
    }
};

struct copy
{

    inline static const std::string name{ "f-copy" };

    inline static const std::string doc{ R"((f-copy FROM TO)

Copy file or directory `FROM` to `TO`.
)" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;


        auto source = arg_eval(eval, obj, 0);
        auto target = arg_eval(eval, obj, 1);

        try
        {
            fs::copy(source->to_string(), target->to_string());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }

        return Qt;
    }
};

struct move
{

    inline static const std::string name{ "f-move" };

    inline static const std::string doc{ R"((f-move FROM TO)

Move or rename `FROM` to `TO`.
)" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;


        auto source = arg_eval(eval, obj, 0);
        auto target = arg_eval(eval, obj, 1);


        try
        {
            fs::rename(source->to_string(), target->to_string());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }


        return Qt;
    }
};

struct make_symlink
{

    inline static const std::string name{ "f-make-symlink" };


    inline static const std::string doc{ R"((f-make-symlink SOURCE PATH)

Create a symlink to `SOURCE` from `PATH`.
)" };

    inline static const Signature signature{ String{}, String{} };


    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;


        auto link   = arg_eval(eval, obj, 0);
        auto target = arg_eval(eval, obj, 1);


        try
        {
            fs::create_symlink(target->to_string(), link->to_string());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }


        return Qt;
    }
};

struct Sdelete
{

    inline static const std::string name{ "f-delete" };

    inline static const std::string doc{ R"((f-delete PATH)

Delete `PATH`, which can be file or directory.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        try
        {
            if (std::size(*obj) > 1 and is_truthy(arg_eval(eval, obj, 1)))
            {

                bool val = fs::remove_all(path->to_string());
                return val ? Qt : Qnil;
            }
            else
            {
                bool val = fs::remove(path->to_string());
                return val ? Qt : Qnil;
            }
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct mkdir
{


    inline static const std::string name{ "f-mkdir" };

    inline static const std::string doc{ R"((f-mkdir DIR)

Create the directory `DIR`.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        try
        {
            bool val = fs::create_directory(path->to_string());
            return val ? Qt : Qnil;
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct with_temp_file
{

    inline static const std::string name{ "f-with-temp-file" };

    inline static const std::string doc{ R"((f-with-temp-file FILE-SYM BODY)

Bind `FILE-SYM` and execute the forms in `BODY`. `FILE-SYM` will point
to a valid file resource of a temporary file.
)" };

    inline static const Signature signature{ String{}, Rest{}, List{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *env, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto sym = arg_eval(eval, obj, 0);

        auto path = FileHelpers::temp_file_path();
        auto id   = FileHelpers::put_file(path, std::fstream(path, std::ios::out), false, true);

        env::detail::ScopePushPop scope{ *env };
        env->put(sym, id);

        auto res = eval_list(eval, obj, 1);
        FileHelpers::close_file(id);
        fs::remove(path);
        return res;
    }
};

struct temp_file_name
{

    inline static const std::string name{ "f-temp-file-name" };

    inline static const std::string doc{ R"((f-temp-file-name PATH)

Return a path to a temporary file. The file is not created but the
path will be valid for a temporary file.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &, env::Environment *, eval::Evaluator *)
    {
        return make_string(FileHelpers::temp_file_path());
    }
};

struct temp_file
{
    inline static const std::string name{ "f-temp-file" };

    inline static const std::string doc{ R"((f-temp-file PATH)

Return a resource object ot a temporary file. The file is created and
the object can be used for writing to the file.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &, env::Environment *, eval::Evaluator *)
    {
        auto path = FileHelpers::temp_file_path();
        return FileHelpers::put_file(path, std::fstream(path, std::ios::out), false, true);
    }
};

struct read_bytes
{

    inline static const std::string name{ "f-read-bytes" };

    inline static const std::string doc{ R"((f-read-bytes PATH)

Read binary data from `PATH`. Return the binary data as byte array.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;


        auto path = arg_eval(eval, obj, 0);

        if (!fs::exists(path->to_string()))
        {
            return Qnil;
        }
        if (!fs::is_regular_file(path->to_string()))
        {
            return Qnil;
        }

        std::ifstream infile(path->to_string().c_str(), std::ios::in | std::ios::ate | std::ios::binary);
        if (!infile.is_open())
        {
            return Qnil;
        }

        auto size = infile.tellg();
        infile.seekg(0, std::ios::beg);
        assert(size >= 0);

        std::vector<char> v(static_cast<size_t>(size));
        infile.read(&v[0], static_cast<std::streamsize>(size));

        ALObject::list_type bytes;
        for (auto &ch : v)
        {
            bytes.push_back(make_int(static_cast<int>(ch)));
        }

        return make_object(bytes);
    }
};

struct read_text
{

    inline static const std::string name{ "f-read-text" };

    inline static const std::string doc{ R"((f-read-text PATH)

Read the text from the file `PATH` and return the contatns as a string.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);


        if (!fs::exists(path->to_string()))
        {
            return Qnil;
        }
        if (!fs::is_regular_file(path->to_string()))
        {
            return Qnil;
        }

        return make_string(utility::load_file(path->to_string()));
    }
};

struct write_text
{

    inline static const std::string name{ "f-write-text" };

    inline static const std::string doc{ R"((f-write-text PATH TEXT)

Write `TEXT` to the file pointed by `PATH`. Previous content is erased.
)" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;


        auto path = arg_eval(eval, obj, 0);
        auto text = arg_eval(eval, obj, 1);


        if (!fs::exists(path->to_string()))
        {
            return Qnil;
        }
        if (!fs::is_regular_file(path->to_string()))
        {
            return Qnil;
        }

        std::ofstream outfile;
        outfile.open(path->to_string(), std::ios_base::out);
        outfile << text->to_string();
        outfile.close();

        return Qt;
    }
};

struct write_bytes
{

    inline static const std::string name{ "f-write-bytes" };

    inline static const std::string doc{ R"((f-write-bytes PATH BYTES)

Write the bytes `BYTES` to the file pointed by `PATH`. Previous content is erased.
)" };

    inline static const Signature signature{ String{}, ByteArray{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;


        auto path  = arg_eval(eval, obj, 0);
        auto bytes = arg_eval(eval, obj, 1);

        if (!fs::exists(path->to_string()))
        {
            return Qnil;
        }
        if (!fs::is_regular_file(path->to_string()))
        {
            return Qnil;
        }

        std::ofstream outfile;
        outfile.open(path->to_string(), std::ios_base::out | std::ios_base::binary);
        if (outfile.is_open())
        {
            return Qnil;
        }
        for (auto &b : *bytes)
        {
            outfile.put(static_cast<char>(b->to_int()));
        }
        outfile.close();

        return Qt;
    }
};

struct append_text
{

    inline static const std::string name{ "f-append-text" };

    inline static const std::string doc{ R"((f-append-text PATH TEXT)

Append `TEXT` to the file pointed by `PATH`. This function does not
erase the prevous contents of the file.  )" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;


        auto path = arg_eval(eval, obj, 0);
        auto text = arg_eval(eval, obj, 1);


        if (!fs::exists(path->to_string()))
        {
            return Qnil;
        }
        if (!fs::is_regular_file(path->to_string()))
        {
            return Qnil;
        }

        std::ofstream outfile;
        outfile.open(path->to_string(), std::ios_base::app);
        outfile << text->to_string();
        outfile.close();

        return Qt;
    }
};

struct append_bytes
{

    inline static const std::string name{ "f-append-bytes" };

    inline static const std::string doc{ R"((f-append-bytes PATH BYTES)

Append the bytes `BYTES` to the file pointed by `PATH`. This function does not
erase the prevous contents of the file.
)" };

    inline static const Signature signature{ String{}, ByteArray{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;


        auto path  = arg_eval(eval, obj, 0);
        auto bytes = arg_eval(eval, obj, 1);

        if (!fs::exists(path->to_string()))
        {
            return Qnil;
        }
        if (!fs::is_regular_file(path->to_string()))
        {
            return Qnil;
        }

        std::ofstream outfile;
        outfile.open(path->to_string(), std::ios_base::out | std::ios_base::binary | std::ios_base::app);
        if (outfile.is_open())
        {
            return Qnil;
        }
        for (auto &b : *bytes)
        {
            outfile.put(static_cast<char>(b->to_int()));
        }
        outfile.close();

        return Qt;
    }
};

struct join
{

    inline static const std::string name{ "f-join" };

    inline static const std::string doc{ R"((f-join [ARGS] ...)

Join `ARGS` to a single path.
)" };

    inline static const Signature signature{ Rest{}, List{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto paths    = eval_transform(eval, obj);
        auto path_1   = paths->i(0);
        fs::path path = path_1->to_string();

        for (size_t i = 1; i < obj->size(); ++i)
        {
            auto path_n = obj->i(i);
            path /= path_n->to_string();
        }

        return make_string(path.string());
    }
};

struct split
{

    inline static const std::string name{ "f-split" };

    inline static const std::string doc{ R"((f-split PATH)

Split `PATH` and return list containing parts.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        auto parts = utility::split(path->to_string(), fs::path::preferred_separator);

        return make_list(parts);
    }
};

struct expand
{

    inline static const std::string name{ "f-expand" };

    inline static const std::string doc{ R"((f-expand PATH DIR)

Expand `PATH` relative to `DIR`.
)" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        try
        {
            const auto p = fs::absolute(path->to_string());
            return make_string(p);
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct filename
{

    inline static const std::string name{ "f-filename" };

    inline static const std::string doc{ R"((f-filename PATH)

Return the name of `PATH`.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);


        try
        {
            const auto p = fs::path(path->to_string());
            return make_string(p.filename());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct dirname
{

    inline static const std::string name{ "f-dirname" };

    inline static const std::string doc{ R"((f-dirname PATH)

Return the parent directory to `PATH`.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);


        try
        {
            const auto p = fs::path(path->to_string());
            return make_string(p.parent_path());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct common_parent
{
    inline static const std::string name{ "f-common-parent" };

    inline static const std::string doc{ R"((f-common-parent [PATHS] ...)

Return the deepest common parent directory of `PATHS`.
)" };

    inline static const Signature signature{ Rest{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        auto path = arg_eval(eval, obj, 0);

        return Qnil;
    }
};

struct ext
{

    inline static const std::string name{ "f-ext" };

    inline static const std::string doc{ R"((f-ext PATH)

)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        try
        {
            const auto p = fs::path(path->to_string());
            return make_string(p.extension());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct no_ext
{

    inline static const std::string name{ "f-no-ext" };

    inline static const std::string doc{ R"((f-no-ext PATH)

)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        try
        {
            const auto p = fs::path(path->to_string());
            return make_string(p.stem());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct swap_ext
{
    inline static const std::string name{ "f-swap-ext" };

    inline static const std::string doc{ R"((f-swap-ext PATH)

Return the file extension of `PATH`. The extension, in a file name, is
the part that follows the last ’.’, excluding version numbers and
backup suffixes.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto ext  = arg_eval(eval, obj, 1);

        try
        {
            const auto p = fs::path(path->to_string());
            return make_string(p.stem().string() + "." + ext->to_string());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct base
{

    inline static const std::string name{ "f-base" };

    inline static const std::string doc{ R"((f-base PATH)

Return the name of `PATH`, excluding the extension of file.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto ext  = arg_eval(eval, obj, 1);

        const auto p = fs::path(path->to_string());
        if (fs::is_directory(p))
        {
            return Qnil;
        }

        return make_string(p.stem().filename());
    }
};

struct relative
{

    inline static const std::string name{ "f-relative" };

    inline static const std::string doc{ R"((f-relative PATH)

)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        const auto p = fs::path(path->to_string());

        try
        {
            if (std::size(*obj) > 1)
            {
                auto to_path = arg_eval(eval, obj, 1);
                return make_string(fs::relative(p, to_path->to_string()));
            }
            return make_string(path->to_string());
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct Sshort
{
    inline static const std::string name{ "f-short" };

    inline static const std::string doc{ R"((f-short PATH)

Return abbrev of `PATH`.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        auto path = arg_eval(eval, obj, 0);

        return Qnil;
    }
};

struct Slong
{

    inline static const std::string name{ "f-long" };

    inline static const std::string doc{ R"((f-long PATH)

Return long version of `PATH`.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        try
        {
            return make_string(fs::canonical(path->to_string()));
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct canonical
{
    inline static const std::string name{ "f-cannonical" };

    inline static const std::string doc{ R"((f-canonical PATH)

Return the canonical name of `PATH`.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        try
        {
            return make_string(fs::canonical(path->to_string()));
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct full
{

    inline static const std::string name{ "f-full" };

    inline static const std::string doc{ R"((f-full PATH)

Return absolute path to `PATH`, with ending slash.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);

        try
        {
            auto p = fs::path(path->to_string());
            return make_string(fs::absolute(p));
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct exists
{

    inline static const std::string name{ "f-exists" };

    inline static const std::string doc{ R"((f-exists PATH)

Return `t` if `PATH` exists, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path    = arg_eval(eval, obj, 0);
        const auto p = fs::path(path->to_string());
        return fs::exists(p) ? Qt : Qnil;
    }
};

struct direcotry
{
    inline static const std::string name{ "f-directory" };

    inline static const std::string doc{ R"((f-direcotry PATH)

Return `t` if `PATH` is directory, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto p    = fs::path(path->to_string());
        return fs::is_directory(p) ? Qt : Qnil;
    }
};

struct file
{

    inline static const std::string name{ "f-file" };

    inline static const std::string doc{ R"((f-file PATH)

Return `t` if `PATH` is `nil`, false otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto p    = fs::path(path->to_string());
        return fs::is_regular_file(p) ? Qt : Qnil;
    }
};

struct symlink
{

    inline static const std::string name{ "f-symlink" };

    inline static const std::string doc{ R"((f-symlink PATH)

Return `t` if `PATH` is symlink, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto p    = fs::path(path->to_string());
        return fs::is_symlink(p) ? Qt : Qnil;
    }
};

struct readable
{

    inline static const std::string name{ "f-readable" };

    inline static const std::string doc{ R"((f-readable PATH)

Return `t` if `PATH` is readable, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto p    = fs::path(path->to_string());

        return (fs::status(p).permissions() & fs::perms::owner_read) != fs::perms::none ? Qt : Qnil;
    }
};

struct writable
{

    inline static const std::string name{ "f-writable" };

    inline static const std::string doc{ R"((f-writable PATH)

Return `t` if `PATH` is writable, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto p    = fs::path(path->to_string());

        return (fs::status(p).permissions() & fs::perms::owner_write) != fs::perms::none ? Qt : Qnil;
    }
};

struct executable
{

    inline static const std::string name{ "f-executable" };

    inline static const std::string doc{ R"((f-executable PATH)

Return `t` if `PATH` is executable, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto p    = fs::path(path->to_string());

        return (fs::status(p).permissions() & fs::perms::owner_exec) != fs::perms::none ? Qt : Qnil;
    }
};

struct absolute
{

    inline static const std::string name{ "f-absolute" };

    inline static const std::string doc{ R"((f-absolute PATH)

Return `t` if `PATH` is absolute, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto p    = fs::path(path->to_string());
        return p.is_absolute() ? Qt : Qnil;
    }
};

struct prelative
{
    inline static const std::string name{ "f-prelative" };

    inline static const std::string doc{ R"((f-prelative PATH)

Return `t` if `PATH` is relative, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto p    = fs::path(path->to_string());
        return p.is_relative() ? Qt : Qnil;
    }
};

struct is_root
{

    inline static const std::string name{ "f-is-root" };

    inline static const std::string doc{ R"((f-is-root PATH)

Return `t` if `PATH` is root directory, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path = arg_eval(eval, obj, 0);
        auto p    = fs::path(path->to_string());
        return fs::equivalent(p, fs::current_path().root_path()) ? Qt : Qnil;
    }
};

struct same
{

    inline static const std::string name{ "f-same" };

    inline static const std::string doc{ R"((f-same PATH1 PATH2)

Return `t` if `PATH1` and `PATH2` are references to same file.
)" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path1    = arg_eval(eval, obj, 0);
        auto path2    = arg_eval(eval, obj, 1);
        const auto p1 = fs::path(path1->to_string());
        const auto p2 = fs::path(path2->to_string());

        try
        {
            return fs::equivalent(p1, p2) ? Qt : Qnil;
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct parent_of
{
    inline static const std::string name{ "f-parent-of" };

    inline static const std::string doc{ R"((f-parent-of PATH1 PATH2)

Return t if `PATH1` is parent of `PATH2`.
)" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path1    = arg_eval(eval, obj, 0);
        auto path2    = arg_eval(eval, obj, 1);
        const auto p1 = fs::path(path1->to_string());
        const auto p2 = fs::path(path2->to_string());

        try
        {
            return fs::equivalent(p1, p2.parent_path()) ? Qt : Qnil;
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct child_of
{
    inline static const std::string name{ "f-child-of" };

    inline static const std::string doc{ R"((f-child-of PATH1 PATH2)

Return t if `PATH1` is child of `PATH2`.
)" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path1    = arg_eval(eval, obj, 0);
        auto path2    = arg_eval(eval, obj, 1);
        const auto p1 = fs::path(path1->to_string());
        const auto p2 = fs::path(path2->to_string());

        try
        {
            return fs::equivalent(p1, p2.parent_path()) ? Qt : Qnil;
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct ancestor_of
{
    inline static const std::string name{ "f-anscestor-of" };

    inline static const std::string doc{ R"((f-ancestor-of PATH1 PATH2)

Return `t` if `PATH1` is ancestor of `PATH2`.
)" };

    inline static const Signature signature{ String{}, String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path1    = arg_eval(eval, obj, 0);
        auto path2    = arg_eval(eval, obj, 1);
        const auto p1 = (path1->to_string());
        const auto p2 = (path2->to_string());

        auto parts1 = utility::split(p2, fs::path::preferred_separator);
        auto parts2 = utility::split(p1, fs::path::preferred_separator);

        for (size_t i = 0; i < std::size(parts1); ++i)
        {

            if (std::size(parts2) <= i)
            {
                return Qnil;
            }

            if (parts2[i] != parts1[i])
            {
                return Qnil;
            }
        }

        return Qt;
    }
};

struct descendant_of
{
    inline static const std::string name{ "f-descendant-of" };

    inline static const std::string doc{ R"((f-descendant-of PATH1 PATH1)

Return `t` if `PATH1` is desendant of `PATH2`.
)" };

    inline static const Signature signature{ String{}, String{} };


    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;

        auto path1    = arg_eval(eval, obj, 0);
        auto path2    = arg_eval(eval, obj, 1);
        const auto p1 = (path1->to_string());
        const auto p2 = (path2->to_string());

        auto parts1 = utility::split(p1, fs::path::preferred_separator);
        auto parts2 = utility::split(p2, fs::path::preferred_separator);

        for (size_t i = 0; i < std::size(parts1); ++i)
        {

            if (std::size(parts2) <= i)
            {
                return Qnil;
            }

            if (parts2[i] != parts1[i])
            {
                return Qnil;
            }
        }

        return Qt;
    }
};

struct hidden
{

    inline static const std::string name{ "f-hidden" };

    inline static const std::string doc{ R"((f-hidden PATH)

Return `t` if `PATH` is hidden, `nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)

    {
        namespace fs = std::filesystem;
        auto path    = arg_eval(eval, obj, 0);
        const auto p = fs::path(path->to_string());
        return p.filename().string()[0] == '.' ? Qt : Qnil;
    }
};

struct empty
{
    inline static const std::string name{ "f-empty" };

    inline static const std::string doc{ R"((f-empty PATH)

If `PATH` is a file, return `t` if the file in `PATH` is empty, `nil`
otherwise. If `PATH` is directory, return `t` if directory has no files,
`nil` otherwise.
)" };

    inline static const Signature signature{ String{} };

    static ALObjectPtr func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
    {
        namespace fs = std::filesystem;
        auto path    = arg_eval(eval, obj, 0);

        try
        {
            const auto p = fs::path(path->to_string());
            return fs::is_empty(p) ? Qt : Qnil;
        }
        catch (fs::filesystem_error &exc)
        {
            signal(
              fileio_signal,
              fmt::format(
                "Fileio error: {}\nInvolved path(s): {} , {}", exc.what(), exc.path1().string(), exc.path2().string()));
            return Qnil;
        }
    }
};

struct module_doc
{

    inline static const std::string doc{ R"(The `fileio` moudule provides utilities for working with file paths,
files, directories and some basic IO functions.
)" };
};


}  // namespace detail

env::ModulePtr init_fileio(env::Environment *, eval::Evaluator *)
{

    auto Mfileio = module_init("fileio");
    auto fio_ptr = Mfileio.get();

    module_doc(fio_ptr, detail::module_doc::doc);


    using namespace detail;

    module_defun(fio_ptr, root::name, root::func, root::doc, root::signature.al());
    module_defun(fio_ptr, directories::name, directories::func, directories::doc, directories::signature.al());
    module_defun(fio_ptr, entries::name, entries::func, entries::doc, entries::signature.al());
    module_defun(fio_ptr, Sglob::name, Sglob::func, Sglob::doc, Sglob::signature.al());
    module_defun(fio_ptr, touch::name, touch::func, touch::doc, touch::signature.al());
    module_defun(fio_ptr, Sexpand_user::name, Sexpand_user::func, Sexpand_user::doc, Sexpand_user::signature.al());
    module_defun(fio_ptr, copy::name, copy::func, copy::doc, copy::signature.al());
    module_defun(fio_ptr, move::name, move::func, move::doc, move::signature.al());
    module_defun(fio_ptr, make_symlink::name, make_symlink::func, make_symlink::doc, make_symlink::signature.al());
    module_defun(fio_ptr, Sdelete::name, Sdelete::func, Sdelete::doc, Sdelete::signature.al());
    module_defun(fio_ptr, mkdir::name, mkdir::func, mkdir::doc, mkdir::signature.al());
    module_defun(
      fio_ptr, with_temp_file::name, with_temp_file::func, with_temp_file::doc, with_temp_file::signature.al());
    module_defun(
      fio_ptr, temp_file_name::name, temp_file_name::func, temp_file_name::doc, temp_file_name::signature.al());
    module_defun(fio_ptr, temp_file::name, temp_file::func, temp_file::doc, temp_file::signature.al());
    module_defun(fio_ptr, read_bytes::name, read_bytes::func, read_bytes::doc, read_bytes::signature.al());
    module_defun(fio_ptr, read_text::name, read_text::func, read_text::doc, read_text::signature.al());
    module_defun(fio_ptr, write_text::name, write_text::func, write_text::doc, write_text::signature.al());
    module_defun(fio_ptr, write_bytes::name, write_bytes::func, write_bytes::doc, write_bytes::signature.al());
    module_defun(fio_ptr, append_text::name, append_text::func, append_text::doc, append_text::signature.al());
    module_defun(fio_ptr, append_bytes::name, append_bytes::func, append_bytes::doc, append_bytes::signature.al());
    module_defun(fio_ptr, join::name, join::func, join::doc, join::signature.al());
    module_defun(fio_ptr, split::name, split::func, split::doc, split::signature.al());
    module_defun(fio_ptr, expand::name, expand::func, expand::doc, expand::signature.al());
    module_defun(fio_ptr, filename::name, filename::func, filename::doc, filename::signature.al());
    module_defun(fio_ptr, dirname::name, dirname::func, dirname::doc, dirname::signature.al());
    module_defun(fio_ptr, common_parent::name, common_parent::func, common_parent::doc, common_parent::signature.al());
    module_defun(fio_ptr, ext::name, ext::func, ext::doc, ext::signature.al());
    module_defun(fio_ptr, no_ext::name, no_ext::func, no_ext::doc, no_ext::signature.al());
    module_defun(fio_ptr, swap_ext::name, swap_ext::func, swap_ext::doc, swap_ext::signature.al());
    module_defun(fio_ptr, base::name, base::func, base::doc, base::signature.al());
    module_defun(fio_ptr, relative::name, relative::func, relative::doc, relative::signature.al());
    module_defun(fio_ptr, Sshort::name, Sshort::func, Sshort::doc, Sshort::signature.al());
    module_defun(fio_ptr, Slong::name, Slong::func, Slong::doc, Slong::signature.al());
    module_defun(fio_ptr, canonical::name, canonical::func, canonical::doc, canonical::signature.al());
    module_defun(fio_ptr, full::name, full::func, full::doc, full::signature.al());
    module_defun(fio_ptr, exists::name, exists::func, exists::doc, exists::signature.al());
    module_defun(fio_ptr, direcotry::name, direcotry::func, direcotry::doc, direcotry::signature.al());
    module_defun(fio_ptr, file::name, file::func, file::doc, file::signature.al());
    module_defun(fio_ptr, symlink::name, symlink::func, symlink::doc, symlink::signature.al());
    module_defun(fio_ptr, readable::name, readable::func, readable::doc, readable::signature.al());
    module_defun(fio_ptr, writable::name, writable::func, writable::doc, writable::signature.al());
    module_defun(fio_ptr, executable::name, executable::func, executable::doc, executable::signature.al());
    module_defun(fio_ptr, absolute::name, absolute::func, absolute::doc, absolute::signature.al());
    module_defun(fio_ptr, prelative::name, prelative::func, prelative::doc, prelative::signature.al());
    module_defun(fio_ptr, is_root::name, is_root::func, is_root::doc, is_root::signature.al());
    module_defun(fio_ptr, same::name, same::func, same::doc, same::signature.al());
    module_defun(fio_ptr, parent_of::name, parent_of::func, parent_of::doc, parent_of::signature.al());
    module_defun(fio_ptr, child_of::name, child_of::func, child_of::doc, child_of::signature.al());
    module_defun(fio_ptr, ancestor_of::name, ancestor_of::func, ancestor_of::doc, ancestor_of::signature.al());
    module_defun(fio_ptr, descendant_of::name, descendant_of::func, descendant_of::doc, descendant_of::signature.al());
    module_defun(fio_ptr, hidden::name, hidden::func, hidden::doc, hidden::signature.al());
    module_defun(fio_ptr, empty::name, empty::func, empty::doc, empty::signature.al());


    return Mfileio;
}


}  // namespace alisp
