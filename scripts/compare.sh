#!/bin/bash

echo "Comparing two files :"
echo "    Expected file is :    ${1}"
echo "    Resulted file is :    ${2}"

diff --brief <(sort ${1}) <(sort ${2}) >/dev/null
comp_value=$?

if [ $comp_value -eq 1 ]
then
    echo "ERROR => ${2} differs from ${1}"
else
    echo "SUCCESS => ${2} is identical to ${1}"
fi