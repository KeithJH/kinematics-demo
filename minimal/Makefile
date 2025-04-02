OUT := ./out

all: $(OUT)/VectorOfStruct $(OUT)/VectorOfLargeStruct $(OUT)/StructOfVector $(OUT)/StructOfPointer $(OUT)/Aligned $(OUT)/Oversized
.PHONY: all

$(OUT):
	mkdir -p $(OUT)

$(OUT)/%: %.cpp | $(OUT)
	$(CXX) $(CXX_FLAGS) $< -o $@

run: all
	$(OUT)/VectorOfStruct
	$(OUT)/VectorOfLargeStruct
	$(OUT)/StructOfVector
	$(OUT)/StructOfPointer
	$(OUT)/Aligned
	$(OUT)/Oversized

clean:
	rm -f $(OUT)/VectorOfStruct $(OUT)/VectorOfLargeStruct $(OUT)/StructOfVector $(OUT)/StructOfPointer $(OUT)/Aligned $(OUT)/Oversized
