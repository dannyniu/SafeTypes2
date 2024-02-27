#!/bin/sh

optimize=debug
testfunc() {
    #lldb \
        $exec
}

cd "$(dirname "$0")"
unitest_sh=./unitest.sh
. $unitest_sh

src="\
./s2list-sort-test.c
./s2list.c
./s2obj.c
"

arch_family=defaults
srcset="Plain C"

tests_run
