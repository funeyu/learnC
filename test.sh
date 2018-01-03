#!/bin/bash

function testast {
	result="$(echo "$1" | ./cc -a)"
	echo "${result}"
}
make -s cc

testast '"java";'
testast '1+2-6+8;'
testast '1*2+3*4;'
testast '4/2+6/3;'
testast 'int a=3;'
testast 'int a=3+4+8/4;'
testast 's(b,c,d);'
testast 'a();'
testast '"abc";'
testast "'b';"
testast 'int a=3;'
testast 'int a=4+8;'
testast "char c='a';"
testast "int a;int b;a=b=10"
testast "int a;int b;a=b='c'"
testast '1*2+3;1+3;'
testast 'int a=3;*&a;'
testast 'int a=3;&a;'
testast 'int a=3;int *b=&a;'
testast 'char s[4]="abc";'
testast 'char *s="abc"'
testast 'int a[3]={1,2,3};'