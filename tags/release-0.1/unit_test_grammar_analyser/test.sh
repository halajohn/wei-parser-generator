#!/bin/sh

SHOULD_PASS_COUNT=45
SHOULD_FAIL_COUNT=4

for (( i = 1 ; i <= $SHOULD_PASS_COUNT; i++ ))
do
echo -n "unit_test_should_pass_$i ... "
cd unit_test_should_pass_$i
..//..//debug//wpg_d.exe -cmp_ans grammar.gra >>/dev/null 2>&1
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

for (( i = 1 ; i <= $SHOULD_FAIL_COUNT; i++ ))
do
echo -n "unit_test_should_failed_$i ... "
cd unit_test_should_failed_$i
..//..//debug//wpg_d.exe -cmp_ans grammar.gra >>/dev/null 2>&1
if [ $? = 0 ]
then
echo "Failed"
elif [ $? = 1 ]
then
echo "Pass"
elif [ $? = 2 ]
then
echo "Memory leaks"
else
echo "Unknown error code"
fi
cd ..
done
