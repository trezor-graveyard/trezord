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

#pragma once

#include <string>
#include <sstream>
#include <curl/curl.h>

namespace trezord
{
namespace http_client
{

std::size_t
write_to_stream(void *data,
                std::size_t size,
                std::size_t nmemb,
                std::stringstream *stream)
{
    stream->write(static_cast<char *>(data), size * nmemb);
    return size * nmemb;
}

void
request_uri_to_stream(std::string const &uri,
                      std::stringstream *stream)
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error{"CURL init failed"};
    }

    CLOG(INFO, "http.client") << "requesting " << uri;

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_stream);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, stream);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error{curl_easy_strerror(res)};
    }
}

std::string
request_uri_to_string(std::string const &uri)
{
    std::stringstream stream;
    request_uri_to_stream(uri, &stream);
    return stream.str();
}

}
}
