# Flags configuration
CXX = gcc
CXXFLAGS = -Wall -g

OBJ = obj
SRC = src
SRCFNC = src/functions

# for gcov() only
GCOV_FLAGS = -fprofile-arcs -ftest-coverage

all: atom_supplier.out drinks_bar.out molecule_requester.out

# GCOV BUILD ONLY
coverage: coverage-build coverage-run coverage-report

coverage-build: coverage_all

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
	@mkdir -p $(OBJ)
	gcov -o $(OBJ) $(SRC)*.c
	mv *.gcov $(OBJ)
	@echo "Coverage reports are in $(OBJ)"

coverage_all: atom_supplier.out drinks_bar.out molecule_requester.out

drinks_bar.out: $(OBJ)/drinks_bar.o $(OBJ)/atom_warehouse_funcs.o $(OBJ)/elements.o
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) $^ -o $@

atom_supplier.out: $(OBJ)/atom_supplier.o $(OBJ)/atom_supplier_funcs.o $(OBJ)/elements.o
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) $^ -o $@

molecule_requester.out: $(OBJ)/molecule_requester.o $(OBJ)/atom_supplier_funcs.o $(OBJ)/elements.o
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) $^ -o $@

# Compile individual source files into object files
$(OBJ)/atom_supplier_funcs.o: $(SRCFNC)/atom_supplier_funcs.c
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c $< -o $@
$(OBJ)/atom_warehouse_funcs.o: $(SRCFNC)/atom_warehouse_funcs.c
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c $< -o $@
$(OBJ)/elements.o: $(SRC)/elements.c
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c $< -o $@
$(OBJ)/drinks_bar.o: $(SRC)/drinks_bar.c
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c $< -o $@
$(OBJ)/atom_supplier.o: $(SRC)/atom_supplier.c
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c $< -o $@
$(OBJ)/molecule_requester.o: $(SRC)/molecule_requester.c
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c $< -o $@

clean:
	rm -f *.o *.out *.txt *.gcov
	rm -r obj
	clear

.PHONY: all clean