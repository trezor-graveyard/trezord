#include "wire.hpp"

#include "protobuf/state.hpp"
#include "protobuf/json_codec.hpp"
#include "protobuf/wire_codec.hpp"

#include "fixtures/messages.hpp"

#include <algorithm>
#include <fstream>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ProtobufCodecs

#include <boost/test/unit_test.hpp>

using namespace trezord;

struct empty_state_fixture
{
    protobuf::state protobuf_state;
};

struct loaded_state_fixture
    : public empty_state_fixture
{
    loaded_state_fixture()
    {
        // load config file
        std::string cpath("../test/fixtures/trezor.bin");
        std::ifstream config(cpath, std::ios::in | std::ios::binary);
        BOOST_CHECK(config.good());

        // parse to FileDescriptorSet
        protobuf::pb::FileDescriptorSet descriptor_set;
        descriptor_set.ParseFromIstream(&config);
        BOOST_CHECK(descriptor_set.file_size() > 0);

        // initialize the protobuf state
        protobuf_state.build_from_set(descriptor_set);
    }
};

BOOST_FIXTURE_TEST_CASE(wire_codec_with_empty_state_fails,
                        empty_state_fixture)
{
    // fails because of missing MessageType enum
    BOOST_CHECK_THROW(protobuf::wire_codec wc(protobuf_state),
                      std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(json_to_wire_conversion,
                        loaded_state_fixture)
{
    protobuf::json_codec json_codec(protobuf_state);
    protobuf::wire_codec wire_codec(protobuf_state);

    wire_codec.load_protobuf_state();

    for (auto &row: message_encoding_sample) {
        std::uint16_t expected_wire_id = row.first.first;
        std::string expected_wire_str = row.first.second;
        std::string json_str = row.second;

        Json::Value json;
        Json::Reader reader;
        reader.parse(json_str, json);

        std::unique_ptr<protobuf::pb::Message> pbuf(
            json_codec.typed_json_to_protobuf(json));
        wire::message wire;
        wire_codec.protobuf_to_wire(*pbuf, wire);

        BOOST_REQUIRE_EQUAL(wire.id,
                            expected_wire_id);
        BOOST_REQUIRE_EQUAL(wire.data.size(),
                            expected_wire_str.size());

        std::vector<unsigned char> expected_wire_data(
            expected_wire_str.begin(),
            expected_wire_str.end());

        BOOST_CHECK_EQUAL_COLLECTIONS(
            wire.data.begin(),
            wire.data.end(),
            expected_wire_data.begin(),
            expected_wire_data.end());
    }
}

BOOST_FIXTURE_TEST_CASE(wire_to_json_conversion,
                        loaded_state_fixture)
{
    protobuf::json_codec json_codec(protobuf_state);
    protobuf::wire_codec wire_codec(protobuf_state);

    wire_codec.load_protobuf_state();

    for (auto &row: message_encoding_sample) {
        std::uint16_t wire_id = row.first.first;
        std::string wire_str = row.first.second;
        std::string expected_json_str = row.second;

        wire::message wire{
            wire_id, {wire_str.begin(), wire_str.end()}
        };
        std::unique_ptr<protobuf::pb::Message> pbuf(
            wire_codec.wire_to_protobuf(wire));

        Json::Value json = json_codec.protobuf_to_typed_json(*pbuf);
        Json::Value expected_json;
        Json::Reader reader;
        reader.parse(expected_json_str, expected_json);

        BOOST_REQUIRE_EQUAL(json, expected_json);
    }
}
