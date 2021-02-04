#!/bin/bash

INFILE=$1
NEVENTS=$2

BASEPATH=/afs/cern.ch/user/g/gauzinge/rd53stream

echo ' Using datafile ' $INFILE
echo ' Running over ' $NEVENTS
echo $BASEPATH/bin/makestream $INFILE $NEVENTS
$BASEPATH/bin/makestream $INFILE $NEVENTS
