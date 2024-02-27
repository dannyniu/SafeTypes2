#!/bin/sh

optimize=debug
testfunc() {
    #lldb \
        $exec | sort -V | uniq -c | tee /dev/tty | wc -l
}

cd "$(dirname "$0")"
unitest_sh=./unitest.sh
. $unitest_sh

src="\
./s2dict-test.c
./s2dict.c
./s2data.c
./s2obj.c
./siphash.c
"

arch_family=defaults
srcset="Plain C"

tests_run
