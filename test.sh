#!/bin/bash

function testast {
	result="$(echo "$1" | ./cc -a)"
	echo "${result}"
}
make -s cc

testast '"java"'
testast '1+2-3+4'
testast '1*2+3*4'
testast '4/2+6/3'
testast 'a=3'
testast 's(b,c,d)'
testast 'a()'
testast '"abc"'
testast "'b'"
