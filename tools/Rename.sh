#!/bin/bash -
#===============================================================================
#
#          FILE: Replace.sh
#
#         USAGE: ./Replace.sh
#
#   DESCRIPTION: 
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Peng Wang (), pw2191195@gmail.com
#  ORGANIZATION: 
#       CREATED: 03/19/17 09:08:11
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

function ProcessOne() {
  if [ ! $1 == $2 ]; then
    git mv "alpha/$1" "alpha/$2"
  fi
  if [ -e "alpha/$3" ]; then
    if [ ! $3 == $4 ]; then
      git mv "alpha/$3" "alpha/$4"
    fi
  fi
  for file in `find . -name "*.h" -o -name "*.cc"`; do
    echo $file
    OLD_HEADER_REG=${1/./\\.}
    NEW_HEADER_REG=${2/./\\.}
    SOURCE_REG=${3/./\\.}
    sed -i "s/${OLD_HEADER_REG}/$2/g" $file
    sed -i "s/\"${NEW_HEADER_REG}\"/<alpha\/$2>/g" $file
    sed -i "s/${SOURCE_REG}/$4/g" $file
  done
}

for file in `ls alpha/*.h`; do
#for file in `ls alpha/HTTPHeaders.h`; do
  OLD_HEADER=${file#alpha/}
  NEW_HEADER=`./SourceFilename.py $OLD_HEADER`
  OLD_SOURCE="${OLD_HEADER%.h}.cc"
  NEW_SOURCE=`./SourceFilename.py $OLD_SOURCE`
  
  echo $OLD_HEADER "->" $NEW_HEADER
  echo $OLD_SOURCE "->" $NEW_SOURCE

  ProcessOne $OLD_HEADER $NEW_HEADER $OLD_SOURCE $NEW_SOURCE
done
