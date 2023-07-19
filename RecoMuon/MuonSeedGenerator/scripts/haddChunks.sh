#!/bin/bash

# Merge EDM files in the (succesful) chunks

# Set paths
inputDir=${1:-.}
echo "INFO: input chunks searched in" $inputDir "($(realpath $inputDir))"
realpath $inputDir | grep -q AAAOK || { echo 'ERROR: you are probably trying to merge unfinished jobs' 1>&2 ; exit 1 ; }
inputFiles=$(ls $inputDir/Chunk_*/seedRebuild.root)
echo "INFO: found" $(echo $inputFiles | wc -w) "file(s)"
inputFiles=$(echo $inputFiles | tr -d '\n' | tr " " "\n" | sed "s/^/file:/g" | tr "\n" "," ; echo)
outputFile=$inputDir/seedRebuild.root
echo "INFO: output will be stored in" $outputFile

set -x
edmCopyPickMerge outputFile=$outputFile inputFiles="$inputFiles" 2>&1 | gzip --fast > edmCopyPickMerge.log.gz
set +x
for i in $(seq 3) ; do  # Ring the bell three times
    printf "\a"
    sleep 0.4
done
