#!/bin/sh

testfunc() {
    $exec
}

cd "$(dirname "$0")"
unitest_sh=./unitest.sh
. $unitest_sh

src="\
./s2obj.c
"

cflags="-D UniTest_S2GC_Locking"
arch_family=defaults
srcset="Plain C"

tests_run
