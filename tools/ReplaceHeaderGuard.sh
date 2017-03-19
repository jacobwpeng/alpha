#!/bin/bash -
#===============================================================================
#
#          FILE: ReplaceHeaderGuard.sh
#
#         USAGE: ./ReplaceHeaderGuard.sh
#
#   DESCRIPTION: 
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Peng Wang (), pw2191195@gmail.com
#  ORGANIZATION: 
#       CREATED: 03/19/17 11:26:13
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

for file in `find alpha/ -name "*.h" | grep -v -- "-inl.h"`; do
  lines=`grep -l '#ifndef __' $file | wc -l`
  if [[ $lines != 0 ]]; then
    echo $file $lines
    ./HeaderGuard.py $file
  fi
done

for file in `find examples/ -name "*.h" | grep -v -- "-inl.h"`; do
  lines=`grep -l '#ifndef __' $file | wc -l`
  if [[ $lines != 0 ]]; then
    echo $file $lines
    ./HeaderGuard.py $file
  fi
done
