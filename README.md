# Superscalar Out-of-Order Processor Simulator

A cycle-accurate simulator implementing a modern superscalar out-of-order processor microarchitecture with register renaming, reorder buffer, and issue queue. This project demonstrates deep understanding of processor design principles, pipeline optimization, and hardware resource management—core skills essential for FPGA development in high-performance computing systems.

## 🎯 Project Overview

This simulator models a complete out-of-order execution engine with configurable parameters for ROB (Reorder Buffer) size, Issue Queue size, and processor width. It accurately simulates instruction flow through all pipeline stages, handling data dependencies, register renaming, and out-of-order execution scheduling.

### Key Features

- **Complete Pipeline Implementation**: 9-stage pipeline (Fetch → Decode → Rename → Register Read → Dispatch → Issue → Execute → Writeback → Retire)
- **Register Renaming**: Implements Register Map Table (RMT) for eliminating false dependencies (WAR/WAW hazards)
- **Out-of-Order Execution**: Issue Queue with wakeup and select logic for dynamic instruction scheduling
- **Reorder Buffer (ROB)**: Maintains program order for precise exception handling and retirement
- **Multiple Functional Units**: Supports functional units with varying latencies (1, 2, 3, 4, 5 cycles)
- **Cycle-Accurate Simulation**: Tracks instruction timing through all pipeline stages
- **Performance Metrics**: Calculates IPC (Instructions Per Cycle) and cycle counts

## Architecture

### Pipeline Stages

1. **Fetch**: Retrieves instructions from trace file (up to processor width per cycle)
2. **Decode**: Decodes instructions and prepares for renaming
3. **Rename**: Maps architectural registers to physical ROB entries, resolves dependencies
4. **Register Read**: Checks operand availability, handles data forwarding
5. **Dispatch**: Allocates instructions to Issue Queue when space available
6. **Issue**: Selects ready instructions for execution (oldest-first scheduling)
7. **Execute**: Simulates execution in functional units with appropriate latencies
8. **Writeback**: Writes results back and wakes up dependent instructions
9. **Retire**: Commits instructions in program order, frees ROB entries

### Data Structures

- **Register Map Table (RMT)**: Maps architectural registers to ROB tags
- **Reorder Buffer (ROB)**: Circular buffer tracking in-flight instructions
- **Issue Queue (IQ)**: Array-based queue for ready-to-execute instructions
- **Functional Unit Queues**: Separate queues for different execution latencies

## Technical Highlights

### Skills Demonstrated

- **Microarchitecture Design**: Deep understanding of modern processor design principles
- **Pipeline Optimization**: Efficient stage-by-stage instruction processing
- **Resource Management**: Dynamic allocation and deallocation of ROB and IQ entries
- **Dependency Resolution**: Handling RAW, WAR, and WAW hazards through renaming
- **Out-of-Order Scheduling**: Wakeup and select logic for instruction issue
- **Cycle-Accurate Modeling**: Precise timing simulation for performance analysis
- **C++ Systems Programming**: Efficient data structures and queue management

### FPGA Relevance

This project demonstrates skills directly applicable to FPGA development:

- **Pipeline Design**: Understanding of multi-stage pipelines essential for FPGA datapath design
- **Resource Constraints**: Managing limited ROB/IQ resources mirrors FPGA resource optimization
- **Timing Analysis**: Cycle-accurate simulation translates to FPGA timing closure
- **Parallel Processing**: Superscalar width concepts apply to FPGA parallelism
- **State Machine Design**: Pipeline control logic similar to FPGA state machines
- **Memory Management**: Efficient buffer management critical for FPGA BRAM usage

## Build & Usage

### Prerequisites

- C++ compiler (g++ recommended)
- Make

### Building

```bash
make
```

This creates the `sim` executable.

### Running

```bash
./sim <ROB_SIZE> <IQ_SIZE> <WIDTH> <TRACE_FILE>
```

**Parameters:**
- `ROB_SIZE`: Size of the Reorder Buffer (e.g., 64, 128, 256)
- `IQ_SIZE`: Size of the Issue Queue (e.g., 16, 32, 64)
- `WIDTH`: Processor width - instructions fetched/retired per cycle (e.g., 2, 4, 8)
- `TRACE_FILE`: Path to instruction trace file

**Example:**
```bash
./sim 128 32 4 proj3-traces/val_trace_gcc1
```

### Output

The simulator outputs:
- Processor configuration (ROB size, IQ size, width)
- Dynamic instruction count
- Total cycles
- Instructions Per Cycle (IPC)

## Trace File Format

Trace files contain one instruction per line:
```
<PC> <OP_TYPE> <DEST_REG> <SRC1_REG> <SRC2_REG>
```

Where:
- `PC`: Program counter (hex)
- `OP_TYPE`: Operation type (0=1 cycle, 1=2 cycles, 2=5 cycles)
- `DEST_REG`: Destination register (-1 if none)
- `SRC1_REG`: Source register 1 (-1 if none)
- `SRC2_REG`: Source register 2 (-1 if none)

## Project Structure

```
463_proj3/
├── sim_proc.h          # Data structure definitions (ROB, IQ, RMT, instruction)
├── sim_proc.cpp        # Main simulator implementation
├── Makefile            # Build configuration
├── proj3-traces/       # Instruction trace files
│   ├── val_trace_gcc1
│   └── val_trace_perl1
└── README.md           # This file
```

## Future Enhancements

Potential improvements for future implementation:
- Multi-threaded execution support
- Hardware description language (HDL) translation
- Real-time performance profiling

## Notes

- The simulator processes instructions in reverse pipeline order (retire → fetch) to handle dependencies correctly
- Wakeup logic propagates execution results to dependent instructions
- ROB and IQ use circular buffer implementations for efficient space utilization


