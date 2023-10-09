#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright 2022 Stéphane Caron
# Copyright 2023 Inria
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse

import msgpack

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", help="MessagePack file to load")
    args = parser.parse_args()
    unpacker = msgpack.Unpacker(raw=False)
    unpacked = 0
    dictionary = {}
    with open(args.file, "rb") as buffer:
        data = buffer.read()
        unpacker.feed(data)
    for new_dict in unpacker:
        dictionary.update(new_dict)
        print(dictionary)
        unpacked += 1
