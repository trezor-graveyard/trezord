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

#include "protobuf/state.hpp"

#include <stdexcept>

#include <json/json.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/repeated_field.h>

namespace trezord
{
namespace protobuf
{

namespace pb = google::protobuf;

struct json_codec
{
    json_codec(state *s)
        : protobuf_state(s)
    {}

    Json::Value
    protobuf_to_typed_json(pb::Message const &msg)
    {
        Json::Value val(Json::objectValue);
        val["type"] = msg.GetDescriptor()->name();
        val["message"] = protobuf_to_json(msg);
        return val;
    }

    pb::Message *
    typed_json_to_protobuf(Json::Value const &val)
    {
        auto name = val["type"];
        auto data = val["message"];

        if (!name.isString()) {
            throw std::invalid_argument("expecting JSON string");
        }

        auto descriptor = protobuf_state->descriptor_pool
            .FindMessageTypeByName(name.asString());
        if (!descriptor) {
            throw std::invalid_argument("unknown message");
        }

        auto prototype = protobuf_state->message_factory
            .GetPrototype(descriptor);

        pb::Message *msg = prototype->New();
        json_to_protobuf(data, *msg);
        return msg;
    }

private:

    state *protobuf_state;

    Json::Value
    protobuf_to_json(pb::Message const &msg)
    {
        Json::Value val(Json::objectValue);

        auto md = msg.GetDescriptor();
        auto ref = msg.GetReflection();

        for (int i = 0; i < md->field_count(); i++) {
            auto fd = md->field(i);
            auto fname = fd->name();

            try {
                if (fd->is_repeated()) {
                    val[fname] = serialize_repeated_field(msg, *ref, *fd);
                    // no empty arrays for repeated fields
                    if (val[fname].empty()) {
                        val.removeMember(fname);
                    }
                }
                else if (ref->HasField(msg, fd)) {
                    val[fname] = serialize_single_field(msg, *ref, *fd);
                }
            }
            catch (std::exception const &e) {
                throw std::invalid_argument("error while serializing "
                                            + fd->full_name()
                                            + ", caused by: "
                                            + e.what());
            }
        }

        return val;
    }

    void
    json_to_protobuf(Json::Value const &val,
                     pb::Message &msg)
    {
        if (!val.isObject()) {
            throw std::invalid_argument("expecting JSON object");
        }

        auto md = msg.GetDescriptor();
        auto ref = msg.GetReflection();

        for (int i = 0; i < md->field_count(); i++) {
            auto fd = md->field(i);
            auto fname = fd->name();

            if (!val.isMember(fname)) {
                continue;
            }
            try {
                if (fd->is_repeated()) {
                    ref->ClearField(&msg, fd);
                    parse_repeated_field(msg, *ref, *fd, val[fname]);
                }
                else {
                    parse_single_field(msg, *ref, *fd, val[fname]);
                }
            }
            catch (std::exception const &e) {
                throw std::invalid_argument("error while parsing "
                                            + fd->full_name()
                                            + ", caused by: "
                                            + e.what());
            }
        }
    }

    Json::Value
    serialize_single_field(const pb::Message &msg,
                           const pb::Reflection &ref,
                           const pb::FieldDescriptor &fd)
    {
        switch (fd.type()) {

        case pb::FieldDescriptor::TYPE_DOUBLE:
            return ref.GetDouble(msg, &fd);

        case pb::FieldDescriptor::TYPE_FLOAT:
            return ref.GetFloat(msg, &fd);

        case pb::FieldDescriptor::TYPE_INT64:
        case pb::FieldDescriptor::TYPE_SFIXED64:
        case pb::FieldDescriptor::TYPE_SINT64:
            return Json::Value::Int64(ref.GetInt64(msg, &fd));

        case pb::FieldDescriptor::TYPE_UINT64:
        case pb::FieldDescriptor::TYPE_FIXED64:
            return Json::Value::UInt64(ref.GetUInt64(msg, &fd));

        case pb::FieldDescriptor::TYPE_INT32:
        case pb::FieldDescriptor::TYPE_SFIXED32:
        case pb::FieldDescriptor::TYPE_SINT32:
            return ref.GetInt32(msg, &fd);

        case pb::FieldDescriptor::TYPE_UINT32:
        case pb::FieldDescriptor::TYPE_FIXED32:
            return ref.GetUInt32(msg, &fd);

        case pb::FieldDescriptor::TYPE_BOOL:
            return ref.GetBool(msg, &fd);

        case pb::FieldDescriptor::TYPE_STRING:
            return ref.GetString(msg, &fd);

        case pb::FieldDescriptor::TYPE_BYTES:
            return utils::hex_encode(ref.GetString(msg, &fd));

        case pb::FieldDescriptor::TYPE_ENUM:
            return ref.GetEnum(msg, &fd)->name();

        case pb::FieldDescriptor::TYPE_MESSAGE:
            return protobuf_to_json(ref.GetMessage(msg, &fd));

        default:
            throw std::invalid_argument("field of unsupported type");
        }
    }

    Json::Value
    serialize_repeated_field(const pb::Message &msg,
                             const pb::Reflection &ref,
                             const pb::FieldDescriptor &fd)
    {
        Json::Value result(Json::arrayValue);
        int field_size = ref.FieldSize(msg, &fd);
        result.resize(field_size);

        for (int i = 0; i < field_size; i++) {
            result[i] = serialize_repeated_field_item(msg, ref, fd, i);
        }

        return result;
    }

    Json::Value
    serialize_repeated_field_item(const pb::Message &msg,
                                  const pb::Reflection &ref,
                                  const pb::FieldDescriptor &fd,
                                  int i)
    {
        switch (fd.type()) {

        case pb::FieldDescriptor::TYPE_DOUBLE:
            return ref.GetRepeatedDouble(msg, &fd, i);

        case pb::FieldDescriptor::TYPE_FLOAT:
            return ref.GetRepeatedFloat(msg, &fd, i);

        case pb::FieldDescriptor::TYPE_INT64:
        case pb::FieldDescriptor::TYPE_SFIXED64:
        case pb::FieldDescriptor::TYPE_SINT64:
            return Json::Value::Int64(ref.GetRepeatedInt64(msg, &fd, i));

        case pb::FieldDescriptor::TYPE_UINT64:
        case pb::FieldDescriptor::TYPE_FIXED64:
            return Json::Value::UInt64(ref.GetRepeatedUInt64(msg, &fd, i));

        case pb::FieldDescriptor::TYPE_INT32:
        case pb::FieldDescriptor::TYPE_SFIXED32:
        case pb::FieldDescriptor::TYPE_SINT32:
            return ref.GetRepeatedInt32(msg, &fd, i);

        case pb::FieldDescriptor::TYPE_UINT32:
        case pb::FieldDescriptor::TYPE_FIXED32:
            return ref.GetRepeatedUInt32(msg, &fd, i);

        case pb::FieldDescriptor::TYPE_BOOL:
            return ref.GetRepeatedBool(msg, &fd, i);

        case pb::FieldDescriptor::TYPE_STRING:
            return ref.GetRepeatedString(msg, &fd, i);

        case pb::FieldDescriptor::TYPE_BYTES:
            return utils::hex_encode(ref.GetRepeatedString(msg, &fd, i));

        case pb::FieldDescriptor::TYPE_ENUM:
            return ref.GetRepeatedEnum(msg, &fd, i)->name();

        case pb::FieldDescriptor::TYPE_MESSAGE:
            return protobuf_to_json(ref.GetRepeatedMessage(msg, &fd, i));

        default:
            throw std::invalid_argument("field of unsupported type");
        }
    }

    void
    parse_single_field(pb::Message &msg,
                       const pb::Reflection &ref,
                       const pb::FieldDescriptor &fd,
                       const Json::Value &val)
    {
        switch (fd.type()) {

        case pb::FieldDescriptor::TYPE_DOUBLE:
            ref.SetDouble(&msg, &fd, val.asDouble());
            break;

        case pb::FieldDescriptor::TYPE_FLOAT:
            ref.SetFloat(&msg, &fd, val.asFloat());
            break;

        case pb::FieldDescriptor::TYPE_INT64:
        case pb::FieldDescriptor::TYPE_SFIXED64:
        case pb::FieldDescriptor::TYPE_SINT64:
            ref.SetInt64(&msg, &fd, val.asInt64());
            break;

        case pb::FieldDescriptor::TYPE_UINT64:
        case pb::FieldDescriptor::TYPE_FIXED64:
            ref.SetUInt64(&msg, &fd, val.asUInt64());
            break;

        case pb::FieldDescriptor::TYPE_INT32:
        case pb::FieldDescriptor::TYPE_SFIXED32:
        case pb::FieldDescriptor::TYPE_SINT32:
            ref.SetInt32(&msg, &fd, val.asInt());
            break;

        case pb::FieldDescriptor::TYPE_UINT32:
        case pb::FieldDescriptor::TYPE_FIXED32:
            ref.SetUInt32(&msg, &fd, val.asUInt());
            break;

        case pb::FieldDescriptor::TYPE_BOOL:
            ref.SetBool(&msg, &fd, val.asBool());
            break;

        case pb::FieldDescriptor::TYPE_STRING:
            ref.SetString(&msg, &fd, val.asString());
            break;

        case pb::FieldDescriptor::TYPE_BYTES:
            ref.SetString(&msg, &fd, utils::hex_decode(val.asString()));
            break;

        case pb::FieldDescriptor::TYPE_ENUM: {
            auto ed = fd.enum_type();
            auto evd = ed->FindValueByName(val.asString());
            if (!evd) {
                throw std::invalid_argument("unknown enum value");
            }
            ref.SetEnum(&msg, &fd, evd);
            break;
        }

        case pb::FieldDescriptor::TYPE_MESSAGE: {
            auto mf = &protobuf_state->message_factory;
            auto fm = ref.MutableMessage(&msg, &fd, mf);
            json_to_protobuf(val, *fm);
            break;
        }

        default:
            throw std::invalid_argument("field of unsupported type");
        }
    }

    void
    parse_repeated_field(pb::Message &msg,
                         const pb::Reflection &ref,
                         const pb::FieldDescriptor &fd,
                         const Json::Value &val)
    {
        if (!val.isArray()) {
            throw std::invalid_argument("expecting JSON array");
        }
        for (auto v: val) {
            parse_repeated_field_item(msg, ref, fd, v);
        }
    }

    void
    parse_repeated_field_item(pb::Message &msg,
                              const pb::Reflection &ref,
                              const pb::FieldDescriptor &fd,
                              const Json::Value &val)
    {
        switch (fd.type()) {

        case pb::FieldDescriptor::TYPE_DOUBLE:
            ref.AddDouble(&msg, &fd, val.asDouble());
            break;

        case pb::FieldDescriptor::TYPE_FLOAT:
            ref.AddFloat(&msg, &fd, val.asFloat());
            break;

        case pb::FieldDescriptor::TYPE_INT64:
        case pb::FieldDescriptor::TYPE_SFIXED64:
        case pb::FieldDescriptor::TYPE_SINT64:
            ref.AddInt64(&msg, &fd, val.asInt64());
            break;

        case pb::FieldDescriptor::TYPE_UINT64:
        case pb::FieldDescriptor::TYPE_FIXED64:
            ref.AddUInt64(&msg, &fd, val.asUInt64());
            break;

        case pb::FieldDescriptor::TYPE_INT32:
        case pb::FieldDescriptor::TYPE_SFIXED32:
        case pb::FieldDescriptor::TYPE_SINT32:
            ref.AddInt32(&msg, &fd, val.asInt());
            break;

        case pb::FieldDescriptor::TYPE_UINT32:
        case pb::FieldDescriptor::TYPE_FIXED32:
            ref.AddUInt32(&msg, &fd, val.asUInt());
            break;

        case pb::FieldDescriptor::TYPE_BOOL:
            ref.AddBool(&msg, &fd, val.asBool());
            break;

        case pb::FieldDescriptor::TYPE_STRING:
            ref.AddString(&msg, &fd, val.asString());
            break;

        case pb::FieldDescriptor::TYPE_BYTES:
            ref.AddString(&msg, &fd, utils::hex_decode(val.asString()));
            break;

        case pb::FieldDescriptor::TYPE_ENUM: {
            auto ed = fd.enum_type();
            auto evd = ed->FindValueByName(val.asString());
            if (!evd) {
                throw std::invalid_argument("unknown enum value");
            }
            ref.AddEnum(&msg, &fd, evd);
            break;
        }

        case pb::FieldDescriptor::TYPE_MESSAGE: {
            auto mf = &protobuf_state->message_factory;
            auto fm = ref.AddMessage(&msg, &fd, mf);
            json_to_protobuf(val, *fm);
            break;
        }

        default:
            throw std::invalid_argument("field of unsupported type");
        }
    }
};

}
}
