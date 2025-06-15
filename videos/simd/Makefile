OUT := ./out
RUN_ARGS := 1000000 10000

build: $(OUT)/01_aos.none $(OUT)/01_aos.O2 $(OUT)/01_aos.O3 $(OUT)/01_aos.native $(OUT)/02_large_structure $(OUT)/03_soa $(OUT)/04_hybrid $(OUT)/05_aligned
results: $(OUT)/01_aos.none.txt $(OUT)/01_aos.O2.txt $(OUT)/01_aos.O3.txt $(OUT)/01_aos.native.txt $(OUT)/02_large_structure.txt $(OUT)/03_soa.txt $(OUT)/04_hybrid.txt $(OUT)/05_aligned.txt
.PHONY: build
.PHONY: results

BASE_FLAGS := -std=c++20

# Warning flags; comment if needed
BASE_FLAGS += -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough -Wmisleading-indentation -Wsuggest-override
BASE_FLAGS += -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast

O2_FLAGS := -O2
O3_FLAGS := -O3
NATIVE_FLAGS := -O3 -march=native

$(OUT):
	mkdir -p $(OUT)

$(OUT)/01_aos.none: 01_aos.cpp | $(OUT)
	$(CXX) $(CXX_FLAGS) $(BASE_FLAGS) $< -o $@

$(OUT)/01_aos.O2: 01_aos.cpp | $(OUT)
	$(CXX) $(CXX_FLAGS) $(BASE_FLAGS) $(O2_FLAGS) $< -o $@

$(OUT)/01_aos.O3: 01_aos.cpp | $(OUT)
	$(CXX) $(CXX_FLAGS) $(BASE_FLAGS) $(O3_FLAGS) $< -o $@

$(OUT)/01_aos.native: 01_aos.cpp | $(OUT)
	$(CXX) $(CXX_FLAGS) $(BASE_FLAGS) $(NATIVE_FLAGS) $< -o $@

$(OUT)/02_large_structure: 02_large_structure.cpp | $(OUT)
	$(CXX) $(CXX_FLAGS) $(BASE_FLAGS) $(NATIVE_FLAGS) $< -o $@

$(OUT)/03_soa: 03_soa.cpp | $(OUT)
	$(CXX) $(CXX_FLAGS) $(BASE_FLAGS) $(NATIVE_FLAGS) $< -o $@

$(OUT)/05_aligned: 05_aligned.cpp | $(OUT)
	$(CXX) $(CXX_FLAGS) $(BASE_FLAGS) $(NATIVE_FLAGS) $< -o $@

$(OUT)/04_hybrid: 04_hybrid.cpp | $(OUT)
	$(CXX) $(CXX_FLAGS) $(BASE_FLAGS) $(NATIVE_FLAGS) $< -o $@

$(OUT)/%.txt: $(OUT)/%
	$< $(RUN_ARGS) | tee $@
	echo >> $@
	perf stat -ddd $< $(RUN_ARGS) >> $@ 2>&1
	perf record -D 100 $< $(RUN_ARGS)
	echo >> $@
	perf annotate -Mintel >> $@

run: build
	$(OUT)/01_aos.none $(RUN_ARGS)
	$(OUT)/01_aos.O2 $(RUN_ARGS)
	$(OUT)/01_aos.O3 $(RUN_ARGS)
	$(OUT)/01_aos.native $(RUN_ARGS)
	$(OUT)/02_large_structure $(RUN_ARGS)
	$(OUT)/03_soa $(RUN_ARGS)
	$(OUT)/04_hybrid $(RUN_ARGS)
	$(OUT)/05_aligned $(RUN_ARGS)

clean-build:
	rm -f $(OUT)/01_aos.none $(OUT)/01_aos.O2 $(OUT)/01_aos.O3 $(OUT)/01_aos.native $(OUT)/02_large_structure $(OUT)/03_soa $(OUT)/05_aligned $(OUT)/04_hybrid $(OUT)/fifth

clean-results:
	rm -f $(OUT)/01_aos.*.txt $(OUT)/02_large_structure.txt $(OUT)/03_soa.txt $(OUT)/fourth.*.txt

clean: clean-build clean-results
