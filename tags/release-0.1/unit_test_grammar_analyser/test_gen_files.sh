#!/bin/sh

SOURCE=.//unit_test_should_pass_27
TARGET=.//test_gen_files

cd $SOURCE

rm *.cpp
rm *.hpp
..//..//debug//wpg.exe grammar.gra >>/dev/null 2>&1

rm ..//$TARGET//*.cpp
rm ..//$TARGET//*.hpp
cp * ..//$TARGET//

cd ..
