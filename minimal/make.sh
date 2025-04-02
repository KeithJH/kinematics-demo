#!/bin/bash

out="./out"

declare -A compilers
compilers["clang"]="clang++"
compilers["gcc"]="g++"

declare -A warningFlags
warningFlags["clang"]="-Werror -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough -Wmisleading-indentation -Wsuggest-override"
warningFlags["gcc"]="${warningFlags['clang']} -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast"

declare -A archFlags
archFlags["default"]=""
archFlags["native"]="-march=native"

declare -A optFlags
optFlags["O3"]="-O3 -DNDEBUG"
optFlags["O2"]="-O2 -DNDEBUG"
optFlags["O1"]="-O1 -DNDEBUG"
optFlags["O0"]="-O0 -DNDEBUG"
optFlags["debug"]="-Og -g"

for compiler in "${!compilers[@]}"
do
	for optFlag in "${!optFlags[@]}"
	do
		for archFlag in "${!archFlags[@]}"
		do
			mkdir -p "${out}/${compiler}/${optFlag}/${archFlag}"
			make $1 OUT="${out}/${compiler}/${optFlag}/${archFlag}" CXX="${compilers[${compiler}]}" CXX_FLAGS="${warningFlags[${compiler}]} ${optFlags[${optFlag}]} ${archFlags[${archFlag}]}" \
				| tee "${out}/${compiler}/${optFlag}/${archFlag}/make.${1-all}.log"
		done
	done
done
