#!/bin/bash

if [ $# -eq 0 ]
then
    bash install_boost.sh
else
    installPrefix="$1"
    bash install_boost.sh "$installPrefix"
fi


