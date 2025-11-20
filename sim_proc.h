#ifndef SIM_PROC_H
#define SIM_PROC_H

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

typedef struct instruction{
    int ready_A = 0;    // Is SRC A ready to compute
    int ready_B = 0;    // Is SRC B ready to compute

    int rob_valA = 0;   // Is reg A in the ROB
    int rob_valB = 0;   // Is reg B in the ROB
    int rob_A = 0;      // Where in the ROB is src A
    int rob_B = 0;      // Where in the ROB is src B
    int dest_rob = 0;   // Where is this instruction placed in the ROB

    int executed = 0;

    int op_type = 0;    // Type of operation
    int valid = 0;      // For IQ, is it in the IQ?

    int dest_reg = 0;
    int src_regA = 0; int src_regB = 0;   // destination & src registers

    int timing[10] = {0,0,0,0,0,0,0,0,0, 0};
    unsigned long int p_counter, line_number;
}instruction;

typedef struct rob_entry{
    int dest = 0;   // destination register
    int ready = 0;  // ready to be retired
    int executed = 0;
    instruction hold_instruction;
}rob_entry;

typedef struct RMT_entry{
    int valid = 0;
    int rob_tag = 0;
    int executed = 0;
};

// Put additional data structures here as per your requirement

#endif
