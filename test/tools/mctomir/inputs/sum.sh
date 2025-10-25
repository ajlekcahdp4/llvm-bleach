#!/usr/bin/env bash

val1=$(cat "$1")
val2=$(cat "$2")

expr $val1 + $val2
