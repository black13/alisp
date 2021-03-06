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

#include "alisp/config.hpp"
#include "alisp/alisp/alisp_module_helpers.hpp"
#include "alisp/alisp/alisp_common.hpp"
#include "alisp/alisp/alisp_asyncs.hpp"
#include "alisp/alisp/declarations/constants.hpp"
#include "alisp/alisp/alisp_object.hpp"
#include "alisp/alisp/alisp_eval.hpp"

#include "http/definitions.hpp"
#include "http/language_space.hpp"
#include "http/response_handling.hpp"
#include "http/request_handling.hpp"


#include <memory>
#include <vector>
#include <unordered_map>

#include <restbed>

namespace http
{

using namespace alisp;

ALObjectPtr server::func(const ALObjectPtr &, env::Environment *, eval::Evaluator *)
{
    auto new_id = detail::server_registry.emplace_resource()->id;

    auto &server = detail::server_registry[new_id];

    server.g_settings = std::make_unique<restbed::Settings>();
    server.g_server   = std::make_unique<restbed::Service>();

    server.g_settings->set_default_header("Server", "ALR");

    return resource_to_object(new_id);
}

ALObjectPtr server_port::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    auto id = arg_eval(eval, obj, 0);

    auto port = arg_eval(eval, obj, 1);

    detail::server_registry[object_to_resource(id)].g_settings->set_port(static_cast<uint16_t>(port->to_int()));

    return Qt;
}

ALObjectPtr server_root::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    auto id = arg_eval(eval, obj, 0);

    auto root = arg_eval(eval, obj, 1);

    detail::server_registry[object_to_resource(id)].g_settings->set_root(root->to_string());

    return Qt;
}

ALObjectPtr server_static_root::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    auto id                                                     = arg_eval(eval, obj, 0);
    auto root                                                   = arg_eval(eval, obj, 1);
    detail::server_registry[object_to_resource(id)].static_root = root->to_string();
    return Qt;
}

ALObjectPtr server_static_route::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    namespace fs = std::filesystem;
    auto id      = arg_eval(eval, obj, 0);
    auto path    = arg_eval(eval, obj, 1);

    auto server_id = object_to_resource(id);
    auto &server   = detail::server_registry[server_id];

    auto res = std::make_shared<restbed::Resource>();
    res->set_path(path->to_string() + "/{path: TO_END}");
    res->set_method_handler("GET", [&server, eval, path, server_id](const std::shared_ptr<restbed::Session> session) {
        const auto request          = session->get_request();
        const size_t content_length = request->get_header("Content-Length", size_t{ 0 });

        session->fetch(content_length,
                       [&server, request, path, eval, server_id](
                         const std::shared_ptr<restbed::Session> fetched_session, const restbed::Bytes &) {
                           const std::string file_path{ utility::trim(
                             utility::replace(request->get_path(), path->to_string() + "/", "")) };
                           const fs::path full_path =
                             fs::canonical(fs::absolute(fs::path{ server.static_root } / file_path));

                           if (fs::exists(full_path) and fs::is_regular_file(full_path))
                           {
                               AL_DEBUG("Serving static file:"s += full_path.string());
                               restbed::Response response{};
                               detail::attach_file(server, full_path, response);
                               detail::send_response(server_id, response, request, fetched_session);
                           }
                           else
                           {
                               AL_DEBUG("File not found: "s += full_path.string());
                               auto req_obj = detail::handle_request(*request.get());
                               detail::callback_response(
                                 server.not_found_handler, req_obj, server_id, eval, fetched_session, request);
                           }
                       });
    });


    server.g_resources.push_back(std::move(res));


    return Qt;
}

ALObjectPtr server_templates_root::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    auto id               = arg_eval(eval, obj, 0);
    auto root             = arg_eval(eval, obj, 1);
    auto &server          = detail::server_registry[object_to_resource(id)];
    server.templates_root = root->to_string();
    server.setup_template_env();
    return Qt;
}

ALObjectPtr server_address::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    auto id = arg_eval(eval, obj, 0);

    auto address = arg_eval(eval, obj, 1);

    auto address_string = address->to_string();
    if (address_string.compare("localhost") == 0)
    {
        address_string = "127.0.0.1";
    }

    detail::server_registry[object_to_resource(id)].g_settings->set_bind_address(address_string);

    return Qt;
}

ALObjectPtr server_default_header::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    auto id = arg_eval(eval, obj, 0);

    auto header = arg_eval(eval, obj, 1);

    auto value = arg_eval(eval, obj, 2);

    detail::server_registry[object_to_resource(id)].g_settings->set_default_header(header->to_string(),
                                                                                   value->to_string());

    return Qt;
}

ALObjectPtr server_default_headers::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    const auto id = arg_eval(eval, obj, 0);

    const auto headers = arg_eval(eval, obj, 1);

    auto &server = detail::server_registry[object_to_resource(id)];

    for (auto &header : *headers)
    {

        server.g_settings->set_default_header(header->i(0)->to_string(), header->i(1)->to_string());
    }

    return Qt;
}

ALObjectPtr server_worker_limit::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    const auto id = arg_eval(eval, obj, 0);

    const auto limit = arg_eval(eval, obj, 1);

    auto &server = detail::server_registry[object_to_resource(id)];

    server.g_settings->set_worker_limit(static_cast<unsigned int>(limit->to_int()));

    return Qt;
}

ALObjectPtr server_connection_limit::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    const auto id = (arg_eval(eval, obj, 0));

    const auto limit = arg_eval(eval, obj, 1);

    auto &server = detail::server_registry[object_to_resource(id)];

    server.g_settings->set_connection_limit(static_cast<unsigned int>(limit->to_int()));

    return Qt;
}

ALObjectPtr server_ci_uris::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    const auto id = arg_eval(eval, obj, 0);

    const auto value = arg_eval(eval, obj, 1);

    auto &server = detail::server_registry[object_to_resource(id)];

    server.g_settings->set_case_insensitive_uris(is_truthy(value));

    return Qt;
}

ALObjectPtr server_connection_timeout::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    const auto id = arg_eval(eval, obj, 0);

    const auto value = arg_eval(eval, obj, 1);

    auto &server = detail::server_registry[object_to_resource(id)];

    server.g_settings->set_connection_timeout(std::chrono::seconds{ value->to_int() });

    return Qt;
}

ALObjectPtr server_status_msg::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    const auto id = arg_eval(eval, obj, 0);

    const auto status = arg_eval(eval, obj, 1);

    const auto msg = arg_eval(eval, obj, 2);

    auto &server = detail::server_registry[object_to_resource(id)];

    server.g_settings->set_status_message(static_cast<int>(status->to_int()), msg->to_string());

    return Qt;
}

ALObjectPtr server_property::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    const auto id = arg_eval(eval, obj, 0);

    const auto name = arg_eval(eval, obj, 1);

    const auto value = arg_eval(eval, obj, 2);

    auto &server = detail::server_registry[object_to_resource(id)];

    server.g_settings->set_property(name->to_string(), value->to_string());

    return Qt;
}

ALObjectPtr server_not_found_handler::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    const auto id               = arg_eval(eval, obj, 0);
    const auto handler_callback = arg_eval(eval, obj, 1);

    auto s_id    = object_to_resource(id);
    auto &server = detail::server_registry[s_id];

    server.not_found_handler = handler_callback;

    server.g_server->set_not_found_handler(
      [eval, handler_callback, s_id](const std::shared_ptr<restbed::Session> session) {
          const auto request          = session->get_request();
          const size_t content_length = request->get_header("Content-Length", size_t{ 0 });
          session->fetch(content_length,
                         [eval, handler_callback, request, s_id](
                           const std::shared_ptr<restbed::Session> fetched_session, const restbed::Bytes &) {
                             auto req_obj = detail::handle_request(*request.get());
                             detail::callback_response(handler_callback, req_obj, s_id, eval, fetched_session, request);
                         });
      });

    return Qt;
}

ALObjectPtr server_start::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    auto id = arg_eval(eval, obj, 0);
    return async::dispatch<detail::server_start>(eval->async(), id);
}

ALObjectPtr server_stop::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    auto id = arg_eval(eval, obj, 0);
    return async::dispatch<detail::server_stop>(eval->async(), id);
}

ALObjectPtr server_restart::func(const ALObjectPtr &obj, env::Environment *, eval::Evaluator *eval)
{
    auto id = arg_eval(eval, obj, 0);
    return async::dispatch<detail::server_restart>(eval->async(), id);
}

}  // namespace http
