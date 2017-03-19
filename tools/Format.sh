#!/bin/bash -
#===============================================================================
#
#          FILE: Format.sh
#
#         USAGE: ./Format.sh
#
#   DESCRIPTION: 
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Peng Wang (), pw2191195@gmail.com
#  ORGANIZATION: 
#       CREATED: 03/19/17 10:41:01
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

for file in `find . -name "*.h" -o -name "*.cc"`; do
  clang-format -i -style=file $file
done
