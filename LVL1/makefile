# Flags configuration
CXX = gcc
CXXFLAGS = -Wall -g

all: atom_supplier.out atom_warehouse.out

atom_warehouse.out: atom_warehouse.o atom_warehouse_funcs.o
	$(CXX) $(CXXFLAGS) $^ -o $@

atom_supplier.out: atom_supplier.o atom_supplier_funcs.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Convert all source files to object files
%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.out
	clear

.PHONY: all clean