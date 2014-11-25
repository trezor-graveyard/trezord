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

#include <google/protobuf/message.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

namespace trezord
{
namespace protobuf
{

namespace pb = google::protobuf;

struct state
{
    pb::DescriptorPool descriptor_pool;
    pb::DynamicMessageFactory message_factory;

    state(state const&) = delete;
    state &operator=(state const&) = delete;

    state()
        : descriptor_pool(
            pb::DescriptorPool::generated_pool())
    {}

    void
    load_from_set(pb::FileDescriptorSet const &set)
    {
        for (int i = 0; i < set.file_size(); i++) {
            descriptor_pool.BuildFile(set.file(i));
        }
    }
};

}
}
