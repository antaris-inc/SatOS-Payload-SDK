#~/bin/bash

rint_help() {
    echo "$0: Usage is"
    echo "$0 <Dockerfile path> <sample applictaion dir path> <output dir path> "
    exit 0
}

if [[ $# -eq 0 ]]; then
        print_help
        exit
fi


cp -r $2 $3/
cp $1 $3/
cd $3
docker build -f Dockerfile.ubuntu -t $4 .
rm -rf $2
rm $1
