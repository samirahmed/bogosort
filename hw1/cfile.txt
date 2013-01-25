#!/bin/bash
# Copyright (C) 2011 by Jonathan Appavoo, Boston University
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

typeset -r USAGE="$0 <client> <server:port> [file]"

client=$1
server=$2
file=$3

if [[ -z $client || ! -e $client || ! -x $client ]]
then
  echo "ERROR: must specify client executable"
  echo "$USAGE"
  exit -1;
fi

if [[ -z $server ]]
then
  echo "ERROR: must specify server:port string"
  echo "$USAGE"
  exit -1;
fi

if [[ -z $file ]]
then
  file=/etc/passwd
fi

if [[ ! -r $file ]]
then
  echo "ERROR: $file does not seem to be a good data file"
  echo "$USAGE"
  exit -1;
fi

$client > ctest.$$.out <<EOF
connect $server
send $(echo $(cat $file))
quit
EOF

echo $(cat $file) > ctest.$$.in



