#!/bin/bash

echo "Execute all test cases"
expect_pass_resp="Test PASSED"

total_count=0
pass_count=0
fail_count=0

this_script=$(basename $0)

echo "Will exclude self (${this_script}) from execution list"

for test in *.sh
do 
        echo "******************************************************************************************"
        echo
        echo "Test name : $test"
        echo
        echo "******************************************************************************************"
        
        if [ "$test" != "$this_script" ]
        then
                echo "Now executing $test"
                output=$(./$test)
                exit_status=$?
                echo "$output"
                echo $exit_status
                test_resp=`echo "$output" | tail -1`
                
                if [ "$exit_status" == "0" ] && [ "$expect_pass_resp" == "$test_resp" ]
                then
                        echo "Test case : $test executed successfully with exit_status : $exit_status"
                        pass_count=$(($pass_count + 1))
                else
                        fail_count=$(($fail_count + 1))
                fi

                total_count=$(($total_count + 1))
        else
            echo "Not executing $test, because $this_script should be skipped"
        fi
done


echo "******************************************************************************************"
echo
echo
echo "Total test cases run  : $total_count"
echo "Total PASS test case count : $pass_count"
echo "Total FAIL test case count : $fail_count"
echo
echo
echo "******************************************************************************************"