#!/bin/bash

# move to script directory
pushd "$(dirname "$0")" > /dev/null


if [ -d "./eigen*/" ] 
then
    echo "Eigen folder already exists!"
else
    git clone https://gitlab.com/libeigen/eigen.git
    cd eigen*/
    git checkout 3.4
fi
