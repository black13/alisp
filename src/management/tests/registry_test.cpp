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

#include "catch2/catch.hpp"

#include "alisp/management/registry.hpp"

#include <string>
#include <vector>

using Catch::Matchers::Equals;
using namespace Catch::literals;

TEST_CASE("Basic registry test [inlined]", "[registry]")
{
    using namespace alisp;

    management::Registry<std::string, 42> str_registry;
    auto id  = str_registry.put_resource(std::string("new_str"))->id;
    auto res = str_registry.get_resource(id)->res;

    CHECK(res.compare("new_str") == 0);
    CHECK(str_registry[id].compare("new_str") == 0);
}

TEST_CASE("Basic registry test [dynamic]", "[registry]")
{
    using namespace alisp;

    management::Registry<std::string, 42> str_registry;

    for (int i = 0; i < 20; ++i)
    {
        str_registry.put_resource(std::string{ "this is str: " + std::to_string(i) });
    }

    auto id  = str_registry.put_resource(std::string("new_str"))->id;
    auto res = str_registry.get_resource(id)->res;

    CHECK(res.compare("new_str") == 0);
    CHECK(str_registry[id].compare("new_str") == 0);
}

TEST_CASE("Basic registry test [emplace]", "[registry]")
{
    using namespace alisp;

    management::Registry<std::string, 42> str_registry;

    for (int i = 0; i < 20; ++i)
    {
        str_registry.emplace_resource("this is str: " + std::to_string(i));
    }

    auto id  = str_registry.emplace_resource("new_str")->id;
    auto res = str_registry.get_resource(id)->res;

    CHECK(res.compare("new_str") == 0);
    CHECK(str_registry[id].compare("new_str") == 0);
}

TEST_CASE("Basic registry test [belonging 1]", "[registry]")
{
    using namespace alisp;

    management::Registry<std::string, 42> str_registry;

    for (int i = 0; i < 20; ++i)
    {
        auto id = str_registry.emplace_resource("this is str: " + std::to_string(i))->id;
        CHECK(str_registry.belong(id));
    }

    auto id = str_registry.emplace_resource("new_str")->id;

    CHECK(str_registry.belong(id));
    CHECK(!str_registry.belong(213));
}

TEST_CASE("Basic registry test [destroy 1]", "[registry]")
{
    using namespace alisp;

    management::Registry<std::string, 42> str_registry;

    for (int i = 0; i < 20; ++i)
    {
        auto id = str_registry.emplace_resource("this is str: " + std::to_string(i))->id;
        CHECK(str_registry.belong(id));
    }

    auto id = str_registry.emplace_resource("new_str")->id;
    CHECK(str_registry.belong(id));

    str_registry.destroy_resource(id);

    CHECK(!str_registry.belong(id));

    id = str_registry.emplace_resource("new_str")->id;
    CHECK(str_registry.belong(id));

    str_registry.destroy_resource(id);
    CHECK(!str_registry.belong(id));

    CHECK(!str_registry.belong(id + 10));
    CHECK(!str_registry.belong(id + 2));
    CHECK(str_registry.belong(id - 1));
}

TEST_CASE("Basic registry test [destroy 2]", "[registry]")
{
    using namespace alisp;

    management::Registry<std::string, 42> str_registry;

    for (int i = 0; i < 20; ++i)
    {
        auto id = str_registry.emplace_resource("this is str: " + std::to_string(i))->id;
        CHECK(str_registry.belong(id));
        str_registry.destroy_resource(id);
        CHECK(!str_registry.belong(id));
    }

    auto id = str_registry.emplace_resource("")->id;
    CHECK(str_registry.belong(id));
    str_registry.destroy_resource(id);
    CHECK(!str_registry.belong(id));
}

TEST_CASE("Basic registry test [belong 2]", "[registry]")
{
    using namespace alisp;

    management::Registry<std::string, 42> str_registry;

    CHECK(!str_registry.belong(231));
    CHECK(!str_registry.belong(219083));
    CHECK(!str_registry.belong(0));
    CHECK(!str_registry.belong(213213));
    CHECK(!str_registry.belong(123));
    CHECK(!str_registry.belong(12));
    CHECK(!str_registry.belong(5));
}
