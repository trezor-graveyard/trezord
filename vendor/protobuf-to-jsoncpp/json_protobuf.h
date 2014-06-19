/*

Copyright (c) 2013, EMC Corporation (Isilon Division)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

-- Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

-- Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef __JSON_PROTOBUF_H__
#define __JSON_PROTOBUF_H__

#include <json/json.h>
#include <google/protobuf/message.h>

namespace json_protobuf {

	/**
	 * Convert a protobuf message to a JSON object, storing
	 * the result in a Json::Value object.
	 *
	 * @param msg the protobuf message to convert
	 * @param value JSON object to hold the converted value
	 * @throw std::invalid_argumnet protobuf message contains fields
	 * that can not be converted to JSON
	 */
	void convert_to_json(const google::protobuf::Message& message,
	    Json::Value& value);

	/**
	 * Convert a protobuf message to a JSON object, storing the
	 * result in a std::string.
	 *
	 * @param msg the protobuf message to convert
	 * @param value JSON object to hold the converted value
	 * @throw std::invalid_argument protobuf message contains fields that
	 * can not be converted to JSON
	 */
	void convert_to_json(const google::protobuf::Message& message,
	    std::string& value);

	/**
	 * Convert a JSON object to a protobuf message, reading the
	 * JSON value from a Json::Value object.
	 *
	 * @param value JSON object to convert
	 * @param message protobuf message to hold the converted value
	 * @throw std::invalid_argument a JSON field does not match an existing protobuf
	 * message field
	 */
	void update_from_json(const Json::Value& value,
	    google::protobuf::Message& message);

	/**
	 * Convert a JSON object to a protobuf message, reading the
	 * JSON value from a std::string.
	 *
	 * @param value JSON object to convert
	 * @param message protobuf message to hold the converted value
	 * @throw std::invalid_argument  value does not represent a valid
	 * JSON object or a JSON field does not match an existing protobuf
	 * message field
	 */
	void update_from_json(const std::string& value,
	    google::protobuf::Message& message);

} // namespace json_protobuf

#endif // __JSON_PROTOBUF_H__
