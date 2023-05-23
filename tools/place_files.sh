#!/bin/bash

RELEASE_VERSION=`head -1 VERSION`
OUTDIR=release
SANDBOX=$2
file_list=$1
mkdir -p $SANDBOX

# Remove python pycache files
find . | grep -E "(/__pycache__$|\.pyc$|\.pyo$)" | xargs rm -rf

# Copy all required folders and files into SDK_PATH
line_count=`wc -l $file_list | awk '{print $1}' `
for line_no in $(seq 1 1 $line_count)
do
	line=`awk -v line=$line_no 'NR==line {print }' $file_list | sed -e "s/ +/ /g"`
	# echo " line_no : $line_no , line : $line"
	src=`echo $line | awk -F " " '{print $1}' `
	dst=`echo $line | awk -F " " '{print $2}' `
	# echo "src : [$src], dst : [$dst]"
	if [ -d "$src" ]
	then
		mkdir -p "$SANDBOX/$dst"
		cp -r $src/* "$SANDBOX/$dst"
	else	
		parent=`dirname $dst `
		mkdir -p "$SANDBOX/$parent"
		cp "$src" "$SANDBOX/$dst"
	fi
done;

