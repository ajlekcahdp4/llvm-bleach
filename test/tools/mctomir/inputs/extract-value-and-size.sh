#!/usr/bin/env bash

llvm-readelf --elf-output-style=JSON -s "$1" | jq '[.[].Symbols[] | select(.Symbol.Name.Name | . == "main" or . == "fact") | { key: .Symbol.Name.Name, value: {Value: .Symbol.Value, Size: .Symbol.Size} } ] | from_entries'
