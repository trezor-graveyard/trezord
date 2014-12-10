/*
 * This file is part of the TREZOR project.
 *
 * Copyright (C) 2014 SatoshiLabs
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/date_time/posix_time/posix_time.hpp>

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <locale>
#include <map>

namespace trezord
{
namespace http_api
{

/**
 * JSON support
 */

using json_pair = std::pair<std::string, Json::Value>;
using json_list = std::initializer_list<json_pair>;

Json::Value
json_value(json_list const &list)
{
    Json::Value json{Json::objectValue};
    for (auto const &kv: list) {
        json[kv.first] = kv.second;
    }
    return json;
}

http_server::response_data
json_response(int status, Json::Value const &body)
{
    http_server::response_data response{status, body.toStyledString()};
    response.add_header("Content-Type", "application/json");
    return response;
}

http_server::response_data
json_response(int status, json_list const &list)
{
    return json_response(status, json_value(list));
}

/**
 * Generic error support
 */

struct response_error : public std::runtime_error
{
    int status_code;

    response_error(int status, char const *message)
        : status_code{status},
          runtime_error{message}
    { }
};

http_server::response_data
json_error_response(std::exception_ptr eptr)
{
    static const auto default_status_code = 500;
    static const auto default_message = "internal error";

    try {
        if (eptr) {
            std::rethrow_exception(eptr);
        } else {
            throw std::runtime_error{default_message};
        }
    }
    catch (response_error const &e) {
        return json_response(e.status_code, {{"error", e.what()}});
    }
    catch (std::exception const &e) {
        return json_response(default_status_code, {{"error", e.what()}});
    }
    catch (...) {
        return json_response(default_status_code, {{"error", default_message}});
    }
}

/**
 * Device types encoding/decoding
 */

core::device_kernel::device_path_type
decode_device_path(std::string const &hex)
{
    return utils::hex_decode(hex);
}

std::string
encode_device_path(core::device_kernel::device_path_type const &path)
{
    return utils::hex_encode(path);
}

Json::Value
devices_to_json(core::kernel::device_enumeration_type const &devices)
{
    Json::Value nil;
    Json::Value item{Json::objectValue};
    Json::Value list{Json::arrayValue};

    for (auto const &d: devices) {
        auto const &i = d.first;
        auto const &s = d.second;

        item["path"] = encode_device_path(i.path);
        item["vendor"] = i.vendor_id;
        item["product"] = i.product_id;
        item["serialNumber"] = std::string{
            i.serial_number.begin(),
            i.serial_number.end()};
        item["session"] = s.empty() ? nil : s;
        list.append(item);
    }

    return list;
}

/**
 * Request handlers
 */

struct handler
{
    std::unique_ptr<core::kernel> kernel;

    bool
    is_origin_allowed(std::string const &origin)
    {
        return kernel->is_allowed(origin);
    }

    http_server::response_data
    handle_404(http_server::request_data const &request)
    {
        return http_server::response_data{404, "Not Found"};
    }

    http_server::response_data
    handle_index(http_server::request_data const &request)
    {
        try {
            Json::Value nil;

            auto version = kernel->get_version();
            auto configured = kernel->has_config();
            auto valid_until = kernel->get_config().c.has_valid_until()
                ? kernel->get_config().c.valid_until()
                : nil;

            return json_response(200, {
                    {"version", version},
                    {"configured", configured},
                    {"validUntil", valid_until}
                });
        }
        catch (...) {
            return json_error_response(std::current_exception());
        }
    }

    http_server::response_data
    handle_configure(http_server::request_data const &request)
    {
        try {
            auto body = request.body.str();
            auto origin = request.get_header("Origin");

            core::kernel_config config;

            try {
                config.parse_from_signed_string(utils::hex_decode(body));
                LOG(INFO)
                    << "parsed configuration: \n"
                    << config.get_debug_string();
            }
            catch (core::kernel_config::invalid_config const &e) {
                throw response_error{400, e.what()};
            }

            if (!config.is_initialized()) {
                throw response_error{400, "configuration is incomplete"};
            }
            if (!config.is_unexpired()) {
                throw response_error{400, "configuration is expired"};
            }
            if (origin && !config.is_url_allowed(origin)) {
                throw response_error{400, "origin not allowed"};
            }

            kernel->set_config(config);
            return json_response(200, {});
        }
        catch (...) {
            return json_error_response(std::current_exception());
        }
    }

    http_server::response_data
    handle_listen(http_server::request_data const &request)
    {
        static const auto iter_max = 60;
        static const auto iter_delay = boost::posix_time::milliseconds(500);

        try {
            auto executor = kernel->get_enumeration_executor();
            auto devices = executor->add([=] {
                    return kernel->enumerate_devices();
                }).get();

            for (int i = 0; i < iter_max; i++) {
                auto updated_devices = executor->add([=] {
                        return kernel->enumerate_devices();
                    }).get();

                if (updated_devices == devices) {
                    boost::this_thread::sleep(iter_delay);
                }
                else {
                    devices = updated_devices;
                    break;
                }
            }

            return json_response(200, devices_to_json(devices));
        }
        catch (...) {
            return json_error_response(std::current_exception());
        }
    }

    http_server::response_data
    handle_enumerate(http_server::request_data const &request)
    {
        try {
            auto devices = kernel->get_enumeration_executor()->add([=] {
                    return kernel->enumerate_devices();
                }).get();

            return json_response(200, devices_to_json(devices));
        }
        catch (...) {
            return json_error_response(std::current_exception());
        }
    }

    http_server::response_data
    handle_acquire(http_server::request_data const &request)
    {
        try {
            auto device_path = decode_device_path(request.url_params.str(1));

            auto supported = kernel->get_enumeration_executor()->add([=] {
                    return kernel->is_path_supported(device_path);
                }).get();
            if (!supported) {
                throw response_error{404, "device not found or unsupported"};
            }

            auto session_id = kernel->get_device_executor(device_path)->add([=] {
                    kernel->get_device_kernel(device_path)->open();
                    return kernel->acquire_session(device_path);
                }).get();

            return json_response(200, {{"session", session_id}});
        }
        catch (...) {
            return json_error_response(std::current_exception());
        }
    }

    http_server::response_data
    handle_release(http_server::request_data const &request)
    {
        try {
            auto session_id = request.url_params.str(1);

            core::device_kernel *device;
            utils::async_executor *executor;

            try {
                device = kernel->get_device_kernel_by_session_id(session_id);
                executor = kernel->get_device_executor_by_session_id(session_id);
            }
            catch (core::kernel::unknown_session const &e) {
                throw response_error{404, e.what()};
            }

            executor->add([=] {
                    device->close();
                    kernel->release_session(session_id);
                }).get();

            return json_response(200, {});
        }
        catch (...) {
            return json_error_response(std::current_exception());
        }
    }

    http_server::response_data
    handle_call(http_server::request_data const &request)
    {
        try {
            auto session_id = request.url_params.str(1);
            auto body = request.body.str();

            core::device_kernel *device;
            utils::async_executor *executor;

            try {
                device = kernel->get_device_kernel_by_session_id(session_id);
                executor = kernel->get_device_executor_by_session_id(session_id);
            }
            catch (core::kernel::unknown_session const &e) {
                throw response_error{404, e.what()};
            }

            auto json_message = executor->add([=] {
                    wire::message wire_in;
                    wire::message wire_out;

                    Json::Value json;
                    Json::Reader json_reader;
                    json_reader.parse(body, json);

                    kernel->json_to_wire(json, wire_in);
                    device->call(wire_in, wire_out);
                    kernel->wire_to_json(wire_out, json);

                    return json;
                }).get();

            return json_response(200, json_message);
        }
        catch (...) {
            return json_error_response(std::current_exception());
        }
    }
};

}
}
