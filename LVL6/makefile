# Flags configuration
CXX = gcc
CXXFLAGS = -Wall -g

OBJ = ./obj/
SRC = ./src/
SRCFNC = ./src/functions/


# for gcov() only
GCOV_FLAGS = -fprofile-arcs -ftest-coverage
COVERAGE_DIR = ./coverage/

all: atom_supplier.out drinks_bar.out molecule_requester.out

# GCOV BUILD ONLY
coverage: coverage-build coverage-run coverage-report

coverage-build: coverage_all
	@mkdir -p $(COVERAGE_DIR)

coverage-run:
	@echo "Running test scenario to generate coverage data..."
	@echo "NOTE: You may need to manually run the following in separate terminals:"
	@echo "  ./drinks_bar.out -T 8080 -U 8081 -s /tmp/stream.sock -d /tmp/dgram.sock -f ./storage.txt"
	@echo "  ./atom_supplier.out -h localhost -p 8080"
	@echo "  ./molecule_requester.out -h localhost -p 8080"
	@echo ""
	@echo "Press Enter when you've completed your tests..."
	@read dummy

coverage-report:
	@echo "Generating coverage reports..."
	@mkdir -p $(COVERAGE_DIR)/src
	@mkdir -p $(COVERAGE_DIR)/src/functions
	
	# Generate .gcov files
	gcov -o $(OBJ) $(SRC)*.c
	gcov -o $(OBJ) $(SRCFNC)*.c
	
	# Move gcov files to coverage directory
	mv *.gcov $(COVERAGE_DIR)
	
	@echo ""
	@echo "Coverage report generated in $(COVERAGE_DIR)/html/index.html"
	@echo "Open it with: xdg-open $(COVERAGE_DIR)/html/index.html"

coverage_all: atom_supplier.out drinks_bar.out molecule_requester.out

drinks_bar.out: $(OBJ)drinks_bar.o $(OBJ)atom_warehouse_funcs.o
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) $^ -o $@

atom_supplier.out: $(OBJ)atom_supplier.o $(OBJ)atom_supplier_funcs.o
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) $^ -o $@

molecule_requester.out: $(OBJ)molecule_requester.o $(OBJ)atom_supplier_funcs.o
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) $^ -o $@

# Convert all source files to object files
$(OBJ)%.o: $(SRCFNC)%.c
	mkdir -p obj
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c $< -o $@

$(OBJ)%.o: $(SRC)%.c
	mkdir -p obj
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c $< -o $@


clean:
	rm -f *.o *.out *.txt *.gcov
	rm -r obj
	clear

.PHONY: all clean