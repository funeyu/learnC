#!/bin/bash

function testast {
	result="$(echo "$1" | ./cc -a)"
	echo "${result}"
}
make -s cc

testast '1+2-3+4'