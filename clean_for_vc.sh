#!/bin/sh

rm -f WeiParserGenerator.ncb
rm -f WeiParserGenerator.suo

rm -rf `find . -name "Debug" -or -name "Release" -or -name "debug" -or -name "release"`
rm -rf `find . -name "*.user"`
