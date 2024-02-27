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
./s2gc-ref-cyc-test.c
./siphash.c
./s2obj.c
./s2data.c
./s2dict.c
./s2list.c
"

cflags="-D INTERCEPT_MEM_CALLS=1"
arch_family=defaults
srcset="Plain C"

tests_run
