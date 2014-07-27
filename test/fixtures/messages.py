#!/usr/bin/env python

import sys
import json


def parse_line(line):
    cols = line.split('\t')
    msg = {'wire': {'type': cols[0].strip(),
                    'message': cols[4].strip()},
           'parsed': {'type': cols[1].strip(),
                      'message': json.loads(cols[3].strip())}}
    return msg


def dump_str(s):
    shex = s.encode('hex')
    pairs = [shex[i:i+2] for i in range(0, len(shex), 2)]
    dump = '\\x'.join([''] + pairs)
    return dump


def dump_wire(wire):
    id = wire['type']
    data = wire['message']
    data = data.decode('hex')
    code = '{%s, std::string("%s", %s)}' % (id, dump_str(data), len(data))
    return code


def dump_parsed(parsed):
    parsed_json = json.dumps(parsed)
    parsed_str = dump_str(parsed_json)
    parsed_len = len(parsed_json)
    code = 'std::string("%s", %s)' % (parsed_str, parsed_len)
    return code


def dump_message(message):
    wire = dump_wire(message['wire'])
    parsed = dump_parsed(message['parsed'])
    code = '{%s, %s}' % (wire, parsed)
    return code


def dump_file(messages):
    content = [dump_message(m) for m in messages]
    content = ",\n".join(content)
    return """
#include <string>
#include <utility>

static const std::pair<
    std::pair<
        std::uint16_t,
        std::string
        >,
    std::string
    > message_encoding_sample[] = {
%s
};
""" % content


messages = [parse_line(l) for l in sys.stdin]

print dump_file(messages)
