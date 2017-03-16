#!/bin/bash
set -e

NAME=$1
TRAIN=$2
DIR=tests/$NAME

mkdir ${DIR}
cd ${DIR}

curl -O http://alitrain.cern.ch/train-workdir/${TRAIN}/config/MLTrainDefinition.cfg
curl -O http://alitrain.cern.ch/train-workdir/${TRAIN}/config/env.sh
curl -O http://alitrain.cern.ch/train-workdir/${TRAIN}/config/generator_customization.C
curl -O http://alitrain.cern.ch/train-workdir/${TRAIN}/config/globalvariables.C
curl -O http://alitrain.cern.ch/train-workdir/${TRAIN}/config/handlers.C
curl -O http://alitrain.cern.ch/train-workdir/${TRAIN}/__TRAIN__/lego_train.sh

cp ../../src/generate.C .
cp ../../src/run.sh .
