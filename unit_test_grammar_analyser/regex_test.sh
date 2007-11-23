#!/bin/sh

SHOULD_PASS_COUNT=27

for (( i = 1 ; i <= $SHOULD_PASS_COUNT; i++ ))
do
echo -n "unit_test_regex_$i ... "
cd unit_test_regex_$i
..//..//debug//wpg_d.exe grammar.gra >>/dev/null 2>&1
if [ $? = 0 ]
then
echo "Pass"
elif [ $? = 1 ]
then
echo "Failed"
elif [ $? = 2 ]
then
echo "Memory leaks"
else
echo "Unknown error code"
fi
cd ..
done
