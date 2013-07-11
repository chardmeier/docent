#!/bin/bash

# UPDATE THESE PATHS -- OR USE PARAMS"
nbestScript=/home/staff/sara/utils/docent-clean/docent/scripts/docent2mosesNbest.perl
docent=/home/staff/sara/utils/docent-clean/docent/RELEASE/detailed-docent

infileType=-n

#Process the arguments
while getopts i:c:o:w:mb:x:s:p: opt
do
   case "$opt" in
	   d) docent=$OPTARG;;
	   n) nbestScript=$OPTARG;;
	   i) infile=$OPTARG;;
       c) config=$OPTARG;;
       o) outPath=$OPTARG;;
       w) workingDir=$OPTARG;;
       m) infileType=-m;;
       b) burnIn="-b $OPTARG";;
       x) maxSteps="-x $OPTARG";;
       s) sampleInterval="-i $OPTARG";;
       p) printLastFile="-pl $OPTARG";;
       \?) usage;;
   esac
done


cd $workingDir

#TODO: add decoder config (weights) to docent command
echo "EXECUTING: $docent $infileType $infile  $config $outPath.out $burnIn $maxSteps $sampleInterval $printLastFile"
$docent $infileType $infile  $config $outPath.out $burnIn $maxSteps $sampleInterval $printLastFile


echo "EXECUTING: $nbestScript $config `ls $workingDir/$outPath.out.*.xml` > $outPath.best100.out"
$nbestScript $config `ls $workingDir/$outPath.out.*.xml` > $outPath.best100.out
