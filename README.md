# Operating Systems Exercise 2: Multi-Level Client-Server Network Application

## Overview

This project implements a progressive multi-level client-server network application that simulates an atom warehouse and drinks bar system. The assignment demonstrates various network programming concepts, inter-process communication mechanisms, and system programming techniques across 6 incremental levels.

### Educational Objectives

- **Network Programming**: TCP and UDP socket programming
- **Client-Server Architecture**: Multi-client server implementations
- **Inter-Process Communication**: Unix Domain Sockets (stream and datagram)
- **System Programming**: File I/O, process synchronization, signal handling
- **Protocol Design**: Custom message formats and communication protocols
- **Advanced Features**: I/O multiplexing with `poll()`, file locking with `flock()`

## Project Architecture

The system consists of three main components:

1. **`atom_supplier`** (TCP Client): Supplies atoms to the warehouse
2. **`molecule_requester`** (UDP Client): Requests molecules/drinks from suppliers
3. **Server Components**: 
   - `atom_warehouse` (LVL1)
   - `molecule_supplier` (LVL2)
   - `drinks_bar` (LVL3-6)

## Level-by-Level Feature Progression

### Level 1: Basic TCP Client-Server
- **Components**: `atom_supplier` ↔ `atom_warehouse`
- **Protocol**: TCP connection for atom supply
- **Features**:
  - Basic client-server communication
  - Simple message passing
  - Connection management

### Level 2: UDP + Molecule Synthesis
- **Components**: `atom_supplier` → `molecule_supplier` ← `molecule_requester`
- **New Features**:
  - UDP communication for molecule requests
  - Molecule synthesis from atoms (WATER, CO2, GLUCOSE, ALCOHOL)
  - Dual protocol support (TCP + UDP)
- **Molecules**: 
  - WATER (H2O): 2×H + 1×O
  - CO2: 1×C + 2×O
  - GLUCOSE (C6H12O6): 6×C + 12×H + 6×O
  - ALCOHOL (C2H6O): 2×C + 6×H + 1×O

### Level 3: Complex Drinks Bar
- **Components**: `atom_supplier` → `drinks_bar` ← `molecule_requester`
- **New Features**:
  - Complex drink recipes using molecules
  - Enhanced server logic for drink preparation
- **Drinks**:
  - SOFT_DRINK: 1×WATER + 1×CO2 + 1×GLUCOSE
  - VODKA: 1×WATER + 1×ALCOHOL
  - CHAMPAGNE: 1×WATER + 1×CO2 + 1×ALCOHOL

### Level 4: Command-Line Interface
- **New Features**:
  - `getopt()` for command-line argument parsing
  - Configurable server ports and addresses
  - Enhanced user interface
- **Arguments**:
  - `-p <port>`: Server port configuration
  - `-a <address>`: Server address configuration

### Level 5: Unix Domain Sockets
- **New Features**:
  - Unix Domain Sockets (stream and datagram)
  - Local inter-process communication
  - Reduced network overhead for local communication
- **Socket Types**:
  - Stream sockets (SOCK_STREAM)
  - Datagram sockets (SOCK_DGRAM)

### Level 6: Advanced Features & Persistence
- **New Features**:
  - File-based data persistence
  - Atomic file operations with `flock()`
  - Abstract namespace Unix domain sockets
  - Advanced I/O multiplexing with `poll()`
  - Alarm-based timeout handling
  - Keyboard input processing
- **Storage**: Persistent atom/molecule inventory across server restarts

## Communication Protocols

### TCP Protocol (Atom Supply)
```
ADD <element_type> <quantity>
```
- **Example**: `ADD HYDROGEN 10`
- **Response**: Success/failure acknowledgment

### UDP Protocol (Molecule/Drink Requests)
```
DELIVER <item_type> <quantity>
```
- **Example**: `DELIVER WATER 2`
- **Response**: Success/failure with item delivery

### Status Queries
```
GEN
```
- **Response**: Current warehouse/bar capacity and inventory

## Build Instructions

### Prerequisites
- GCC compiler
- Make utility
- POSIX-compliant system (Linux/macOS)

### Building All Levels
```bash
# Build all levels
make all

# Build specific level
make lvl1    # Level 1
make lvl2    # Level 2
make lvl3    # Level 3
make lvl4    # Level 4
make lvl5    # Level 5
make lvl6    # Level 6
```

### Building Individual Level
```bash
cd LVL1  # or LVL2, LVL3, etc.
make
```

### Clean Build Artifacts
```bash
make clean
```

## File Structure

### Core Headers
- **`const.h`**: Network constants, buffer sizes, connection limits
- **`elements.h`**: Element/molecule type definitions and utilities  
- **`atom_warehouse_funcs.h`**: Server-side processing functions
- **`atom_supplier_funcs.h`**: Client helper functions

### Source Organization
```
LVL[1-6]/
├── include/           # Header files
│   ├── const.h
│   ├── elements.h
│   └── functions/     # Function declarations
├── src/               # Source files
│   ├── atom_supplier.c
│   ├── drinks_bar.c
│   ├── molecule_requester.c
│   └── functions/     # Function implementations
└── makefile
```

## Testing

### Test Suite (test6test/)
The project includes comprehensive testing:

```bash
cd test6test
make
./coverage.sh    # Run code coverage analysis
```

### Coverage Analysis
- **Line Coverage**: Measures code execution coverage
- **Function Coverage**: Ensures all functions are tested
- **Branch Coverage**: Tests different execution paths

### Test Files
- `storage_test.txt`: Test data persistence
- `corrupted_storage.txt`: Error handling tests
- Coverage reports in `coverage_results/`

## Technical Implementation Details

### Network Programming
- **TCP Sockets**: Reliable atom supply connections
- **UDP Sockets**: Fast molecule/drink requests
- **Unix Domain Sockets**: Efficient local IPC

### Concurrency & I/O
- **`poll()`**: I/O multiplexing for handling multiple clients
- **Non-blocking I/O**: Responsive server architecture
- **Signal Handling**: Graceful shutdown and timeout management

### Data Persistence (Level 6)
- **File Locking**: `flock()` for atomic file operations
- **Storage Format**: Human-readable inventory format
- **Error Recovery**: Handles corrupted storage files

### Memory Management
- Dynamic memory allocation for client connections
- Proper cleanup and resource deallocation
- Buffer overflow protection

## Debugging & Development

### Debug Build
```bash
make DEBUG=1    # Enable debug symbols and verbose output
```

### Common Issues
1. **Port Already in Use**: Change port with `-p` argument
2. **Permission Denied**: Check socket file permissions (Unix domain sockets)
3. **Storage File Issues**: Ensure write permissions in LVL6 directory

### Logging
- Server outputs connection status and request processing
- Error messages include system error descriptions
- Debug mode provides detailed protocol information

## Educational Value

This project demonstrates:
- **Progressive Complexity**: Each level builds on previous concepts
- **Real-World Applications**: Network programming patterns used in production
- **System Programming**: Low-level system calls and OS interaction
- **Protocol Design**: Custom application-layer protocols
- **Testing Methodology**: Comprehensive testing and coverage analysis

## Assignment Context

This is Exercise 2 for an Operating Systems course, focusing on:
- Network programming fundamentals
- Client-server architecture patterns
- System-level programming in C
- Inter-process communication mechanisms
- File I/O and persistence strategies

The progressive nature allows students to:
1. Master basic concepts before advancing
2. Understand the evolution of network applications
3. Implement increasingly sophisticated features
4. Debug and test complex multi-component systems

---

Baruh Ifraimov
