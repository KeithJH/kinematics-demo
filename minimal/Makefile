gcc := g++
clang := clang++

common_flags := -std=c++23 -O3 -DNDEBUG
warning_flags := -Werror -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough -Wmisleading-indentation -Wsuggest-override
gcc_warning_flags := $(warning_flags) -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast

all: out/VectorOfStruct.default.gcc out/VectorOfStruct.default.clang out/VectorOfStruct.native.gcc out/VectorOfStruct.native.clang \
	out/VectorOfLargeStruct.default.gcc out/VectorOfLargeStruct.default.clang out/VectorOfLargeStruct.native.gcc out/VectorOfLargeStruct.native.clang \
	out/StructOfVector.default.gcc out/StructOfVector.default.clang out/StructOfVector.native.gcc out/StructOfVector.native.clang \
	out/StructOfPointer.default.gcc out/StructOfPointer.default.clang out/StructOfPointer.native.gcc out/StructOfPointer.native.clang \
	out/Aligned.default.gcc out/Aligned.default.clang out/Aligned.native.gcc out/Aligned.native.clang \
	out/Oversized.default.gcc out/Oversized.default.clang out/Oversized.native.gcc out/Oversized.native.clang
.PHONY: all

out:
	mkdir out

# Compile VectorOfStruct with default arguments
out/VectorOfStruct.default.gcc: VectorOfStruct.cpp | out
	$(gcc) $(common_flags) $(gcc_warning_flags) $< -o $@
out/VectorOfStruct.default.clang: VectorOfStruct.cpp | out
	$(clang) $(common_flags) $(warning_flags) $< -o $@

# Compile VectorOfStruct with native architecture specified
out/VectorOfStruct.native.gcc: VectorOfStruct.cpp | out
	$(gcc) $(common_flags) -march=native $(gcc_warning_flags) $< -o $@
out/VectorOfStruct.native.clang: VectorOfStruct.cpp | out
	$(clang) $(common_flags) -march=native $(warning_flags) $< -o $@

# Compile VectorOfLargeStruct with default arguments
out/VectorOfLargeStruct.default.gcc: VectorOfLargeStruct.cpp | out
	$(gcc) $(common_flags) $(gcc_warning_flags) $< -o $@
out/VectorOfLargeStruct.default.clang: VectorOfLargeStruct.cpp | out
	$(clang) $(common_flags) $(warning_flags) $< -o $@

# Compile VectorOfLargeStruct with native architecture specified
out/VectorOfLargeStruct.native.gcc: VectorOfLargeStruct.cpp | out
	$(gcc) $(common_flags) -march=native $(gcc_warning_flags) $< -o $@
out/VectorOfLargeStruct.native.clang: VectorOfLargeStruct.cpp | out
	$(clang) $(common_flags) -march=native $(warning_flags) $< -o $@

# Compile StructOfVector with default arguments
out/StructOfVector.default.gcc: StructOfVector.cpp | out
	$(gcc) $(common_flags) $(gcc_warning_flags) $< -o $@
out/StructOfVector.default.clang: StructOfVector.cpp | out
	$(clang) $(common_flags) $(warning_flags) $< -o $@

# Compile StructOfVector with native architecture specified
out/StructOfVector.native.gcc: StructOfVector.cpp | out
	$(gcc) $(common_flags) -march=native $(gcc_warning_flags) $< -o $@
out/StructOfVector.native.clang: StructOfVector.cpp | out
	$(clang) $(common_flags) -march=native $(warning_flags) $< -o $@

# Compile StructOfPointer with default arguments
out/StructOfPointer.default.gcc: Aligned.cpp | out
	$(gcc) $(common_flags) $(gcc_warning_flags) $< -o $@
out/StructOfPointer.default.clang: Aligned.cpp | out
	$(clang) $(common_flags) $(warning_flags) $< -o $@

# Compile StructOfPointer with native architecture specified
out/StructOfPointer.native.gcc: Aligned.cpp | out
	$(gcc) $(common_flags) -march=native $(gcc_warning_flags) $< -o $@
out/StructOfPointer.native.clang: Aligned.cpp | out
	$(clang) $(common_flags) -march=native $(warning_flags) $< -o $@

# Compile Aligned with default arguments
out/Aligned.default.gcc: Aligned.cpp | out
	$(gcc) $(common_flags) $(gcc_warning_flags) $< -o $@
out/Aligned.default.clang: Aligned.cpp | out
	$(clang) $(common_flags) $(warning_flags) $< -o $@

# Compile Aligned with native architecture specified
out/Aligned.native.gcc: Aligned.cpp | out
	$(gcc) $(common_flags) -march=native $(gcc_warning_flags) $< -o $@
out/Aligned.native.clang: Aligned.cpp | out
	$(clang) $(common_flags) -march=native $(warning_flags) $< -o $@

# Compile Oversized with default arguments
out/Oversized.default.gcc: Oversized.cpp | out
	$(gcc) $(common_flags) $(gcc_warning_flags) $< -o $@
out/Oversized.default.clang: Oversized.cpp | out
	$(clang) $(common_flags) $(warning_flags) $< -o $@

# Compile Oversized with native architecture specified
out/Oversized.native.gcc: Oversized.cpp | out
	$(gcc) $(common_flags) -march=native $(gcc_warning_flags) $< -o $@
out/Oversized.native.clang: Oversized.cpp | out
	$(clang) $(common_flags) -march=native $(warning_flags) $< -o $@

run: all
	./out/VectorOfStruct.default.gcc
	./out/VectorOfLargeStruct.default.gcc
	./out/StructOfVector.default.gcc
	./out/StructOfPointer.default.gcc
	./out/Aligned.default.gcc
	./out/Oversized.default.gcc

	./out/VectorOfStruct.native.gcc
	./out/VectorOfLargeStruct.native.gcc
	./out/StructOfVector.native.gcc
	./out/StructOfPointer.native.gcc
	./out/Aligned.native.gcc
	./out/Oversized.native.gcc

	./out/VectorOfStruct.default.clang
	./out/VectorOfLargeStruct.default.clang
	./out/StructOfVector.default.clang
	./out/StructOfPointer.default.clang
	./out/Aligned.default.clang
	./out/Oversized.default.clang

	./out/VectorOfStruct.native.clang
	./out/VectorOfLargeStruct.native.clang
	./out/StructOfVector.native.clang
	./out/StructOfPointer.native.clang
	./out/Aligned.native.clang
	./out/Oversized.native.clang

clean:
	rm -r out
