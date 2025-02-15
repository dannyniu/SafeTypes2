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
./s2list-test.c
./s2list.c
./s2data.c
./s2obj.c
"

arch_family=defaults
srcset="Plain C"
cflags="-D SAFETYPES2_BUILD_WITHOUT_GC"

tests_run
