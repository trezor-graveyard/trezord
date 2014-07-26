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
