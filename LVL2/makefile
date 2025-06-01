# Flags configuration
CXX = gcc
CXXFLAGS = -Wall -g

OBJ = ./obj/
SRC = ./src/
SRCFNC = ./src/functions/


all: atom_supplier.out molecule_supplier.out molecule_requester.out

molecule_supplier.out: $(OBJ)molecule_supplier.o $(OBJ)atom_warehouse_funcs.o
	$(CXX) $(CXXFLAGS) $^ -o $@

atom_supplier.out: $(OBJ)atom_supplier.o $(OBJ)atom_supplier_funcs.o
	$(CXX) $(CXXFLAGS) $^ -o $@

molecule_requester.out: $(OBJ)molecule_requester.o $(OBJ)atom_supplier_funcs.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Convert all source files to object files
$(OBJ)%.o: $(SRCFNC)%.c
	mkdir -p obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)%.o: $(SRC)%.c
	mkdir -p obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.out
	clear

.PHONY: all clean