#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sim_proc.h"
#include <queue>
using namespace std;


uint64_t cycle_count = 0;
uint64_t instruction_count = 0;
int total_width;
int total_rob_size;
int total_iq_size;
int rob_head = 0;
int rob_tail = 0;
queue<instruction> fe_queue;
queue<instruction> de_queue;
queue<instruction> rn_queue;
queue<instruction> rr_queue;
queue<instruction> di_queue;
queue<instruction> is_queue;
queue<instruction> ex_queue;
queue<instruction> wb_queue;
queue<instruction> rt_queue;

queue<instruction> FU_5_cyc;
queue<instruction> FU_4_cyc;
queue<instruction> FU_3_cyc;
queue<instruction> FU_2_cyc;
queue<instruction> FU_1_cyc;
//queue<int> oldest_issues;
int wakeup = -1;
int wakeup_value = 0;
//int wakeup_rob;
RMT_entry RMT[67];
int num_in_IQ = 0;
int num_in_rob = 0;
bool end_of_file = false;

void Fetch(FILE *FP)
{
    if(de_queue.empty()) {
        for(int i = 0; i < total_width; i++)
        {
            uint64_t pc;
            int op_type, dest, src1, src2;
            if (fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
            {
                instruction vars;
                vars.dest_reg = dest;
                vars.op_type = op_type;
                vars.ready_A = 0;
                vars.rob_A = 0;
                vars.rob_valA = 0;
                vars.src_regA = src1;
                vars.ready_B = 0;
                vars.rob_B = 0;
                vars.rob_valB = 0;
                vars.src_regB = src2;
                vars.timing[0] = cycle_count;
                vars.timing[1] = cycle_count;
                vars.p_counter = pc;
                vars.line_number = instruction_count;
                vars.valid = 0;
                de_queue.push(vars);
                instruction_count++;
            }
            else {
                end_of_file = true;
            }
        }
    }

}

void Decode() {
    if(rn_queue.empty() && !de_queue.empty())
    {
        int width = de_queue.size();
        for(int i = 0; i < width; i++) {
            instruction var = de_queue.front();
            var.timing[2] = cycle_count;
            rn_queue.push(var);
            de_queue.pop();
        }
    }
}

void Rename(rob_entry rob[])
{
    if( rr_queue.empty() && (total_rob_size - num_in_rob >= rn_queue.size())  && !rn_queue.empty() )
    {
        int width = rn_queue.size();
        for(int i = 0; i < width; i++)
        {
            instruction entry = rn_queue.front();

            if(entry.src_regA != -1)
            {
                if(RMT[entry.src_regA].valid == 1)
                {
                    if (rob[RMT[entry.src_regA].rob_tag].executed == 1) {
                        entry.rob_valA = 0;
                        entry.ready_A = 1;
                    }
                    else {
                        entry.rob_A = RMT[entry.src_regA].rob_tag;
                        entry.rob_valA = 1;
                        entry.ready_A = 0;
                    }
                }
                else {
                    entry.rob_valA = 0;
                    entry.ready_A = 1;
                }
            }
            else {
                entry.rob_valA = 0;
                entry.ready_A = 1;
            }

            if(entry.src_regB != -1)
            {
                if(RMT[entry.src_regB].valid == 1)
                {
                    if (rob[RMT[entry.src_regB].rob_tag].executed == 1) {
                        entry.rob_valB = 0;
                        entry.ready_B = 1;
                    }
                    else {
                        entry.rob_B = RMT[entry.src_regB].rob_tag;
                        entry.rob_valB = 1;
                        entry.ready_B = 0;
                    }
                }
                else {
                    entry.rob_valB = 0;
                    entry.ready_B = 1;
                }
            }
            else {
                entry.rob_valB = 0;
                entry.ready_B = 1;
            }

            rob[rob_tail].dest = entry.dest_reg;
            rob[rob_tail].ready = 0;
            rob[rob_tail].executed = 0;

            if(entry.dest_reg != -1)
            {
                RMT[entry.dest_reg].valid = 1;
                RMT[entry.dest_reg].rob_tag = rob_tail;
            }
            entry.dest_rob = rob_tail;
            rob_tail++;
            num_in_rob++;

            if(rob_tail >= total_rob_size) rob_tail = 0;
            entry.timing[3] = cycle_count;
            rr_queue.push(entry);
            rn_queue.pop();
        }
    }
}

void RegRead(rob_entry rob[]){
    if(di_queue.empty() && !rr_queue.empty())
    {
        int width = rr_queue.size();
        for(int i = 0; i < width; i++)
        {
            instruction inst = rr_queue.front();

            if(inst.rob_valA) {
                if(rob[inst.rob_A].executed == 1) {
                    inst.rob_valA = 0;
                    inst.ready_A = 1;
                }
            }
            if(inst.rob_valB) {
                if(rob[inst.rob_B].executed == 1) {
                    inst.rob_valB = 0;
                    inst.ready_B = 1;
                }
            }

            inst.timing[4] = cycle_count;
            di_queue.push(inst);
            rr_queue.pop();
        }
    }
}

bool room_in_IQ(int size, instruction IQ[]) {
    if(size <= total_iq_size-num_in_IQ) return true;
    else return false;
}

void Dispatch(instruction IQ[]) {
    if( room_in_IQ(di_queue.size(), IQ) && !di_queue.empty() )
    {
        for(int i = 0; i < total_iq_size; i++)
        {
            if(di_queue.empty()) break;
            if(IQ[i].valid == 0)
            {
                num_in_IQ++;
                instruction inst = di_queue.front();
                inst.valid = 1;
                inst.timing[5] = cycle_count;
                IQ[i] = inst;
                di_queue.pop();
            }
        }
    }
}

int num_ready_for_execution(instruction IQ[]) {
    int count = 0;
    for(int i = 0; i < total_iq_size; i++) // Counts num of valid instructions ready for execution
    {
        if(IQ[i].valid == 1)
        {
            if(IQ[i].ready_A == 1 && IQ[i].ready_B == 1) count++;
        }
    }
    return count;
}

void issue(instruction IQ[]) {
    int count = 0;
    for(int i = 0; i < total_width; i++) {
        for(int j = 0; j < num_in_IQ; j++)
        {
            if (count >= total_width) break;
            if(IQ[j].valid == 1)
            {
                if(IQ[j].ready_A == 1 && IQ[j].ready_B == 1)
                {
                    count++;
                    IQ[j].valid = 0;
                    IQ[j].timing[6] = cycle_count;
                    is_queue.push(IQ[j]);
                    num_in_IQ--;

                    for(int k = j; k < total_iq_size; k++)
                    {
                        if(k+1 > total_iq_size) break;
                        if(IQ[k + 1].valid == 1) IQ[k]=IQ[k+1];
                        else {
                            IQ[k].valid = 0;
                        }
                    }
                    break;
                }
            }

        }
    }
}

void Issue(instruction IQ[]) {
    issue(IQ);
    int width = is_queue.size();
    for(int i = 0; i < width; i++)
    {
        instruction inst = is_queue.front();

        if(inst.op_type == 0) {
            FU_1_cyc.push(inst);
        } else if(inst.op_type == 1) {
            FU_2_cyc.push(inst);
        } else {
            FU_5_cyc.push(inst);
        }
        is_queue.pop();
    }
}


void wake_up(instruction IQ[], instruction wakeup)  // Sends wake up values from execute to dispatch, regread, and IQ
{
    queue<instruction> final_di_queue;
    queue<instruction> final_rr_queue;
    queue<instruction> final_rn_queue;
    queue<instruction> copy_of_dispatch;

    if(wakeup.dest_reg == -1) return;

    copy_of_dispatch = di_queue;
    for(int j = 0; j < di_queue.size(); j++) {
        instruction inst = copy_of_dispatch.front();
        if(inst.rob_valA == 1 && wakeup.dest_rob == inst.rob_A)
        {
            inst.ready_A = 1;
            inst.rob_valA = 0;
        } else if(inst.rob_valA == 0) {
            inst.ready_A = 1;
        }
        if(inst.rob_valB == 1 && wakeup.dest_rob == inst.rob_B)
        {
            inst.ready_B = 1;
            inst.rob_valB = 0;
        } else if(inst.rob_valB == 0) {
            inst.ready_B = 1;
        }
        final_di_queue.push(inst);
        copy_of_dispatch.pop();
    }

    if(!final_di_queue.empty()) di_queue = final_di_queue;

    queue<instruction> copy_of_regread = rr_queue;
    for(int j = 0; j < rr_queue.size(); j++) {
        instruction inst = copy_of_regread.front();
        if(inst.rob_valA == 1 && wakeup.dest_rob == inst.rob_A)
        {
            inst.ready_A = 1;
            inst.rob_valA = 0;
        } else if(inst.rob_valA == 0) {
            inst.ready_A = 1;
        }
        if(inst.rob_valB == 1 && wakeup.dest_rob == inst.rob_B)
        {
            inst.ready_B = 1;
            inst.rob_valB = 0;
        } else if(inst.rob_valB == 0) {
            inst.ready_B = 1;
        }
        final_rr_queue.push(inst);
        copy_of_regread.pop();
    }
    if(!final_rr_queue.empty()) rr_queue = final_rr_queue;



    queue<instruction> copy_of_rename = rn_queue;
    for(int j = 0; j < rn_queue.size(); j++) {
        instruction inst = copy_of_rename.front();
        if(inst.rob_valA == 1 && wakeup.dest_rob == inst.rob_A)
        {
            inst.ready_A = 1;
            inst.rob_valA = 0;
        } else if(inst.rob_valA == 0) {
            inst.ready_A = 1;
        }
        if(inst.rob_valB == 1 && wakeup.dest_rob == inst.rob_B)
        {
            inst.ready_B = 1;
            inst.rob_valB = 0;
        } else if(inst.rob_valB == 0) {
            inst.ready_B = 1;
        }
        final_rn_queue.push(inst);
        copy_of_rename.pop();
    }
    if(!final_rn_queue.empty()) rn_queue = final_rn_queue;


    for (int j = 0; j < total_iq_size; j++)
    {
        instruction inst = IQ[j];
        if(inst.valid == 1)
        {
            if(inst.rob_valA == 1 && wakeup.dest_rob == inst.rob_A)
            {
                inst.ready_A = 1;
                inst.rob_valA = 0;
            } else if(inst.rob_valA == 0) {
                inst.ready_A = 1;
            }

            if(inst.rob_valB == 1 && wakeup.dest_rob == inst.rob_B)
            {
                inst.ready_B = 1;
                inst.rob_valB = 0;
            } else if(inst.rob_valB == 0) {
                inst.ready_B = 1;
            }
        }
        IQ[j] = inst;
    }

}

void pipeline_execute(rob_entry rob[], instruction IQ[]) {
    int size = FU_1_cyc.size();
    for(int i = 0; i < size; i++) {
        instruction inst = FU_1_cyc.front();
        inst.timing[7] = cycle_count;

        // if (inst.dest_reg == 14) {
        //     printf("");
        // }

        rob[inst.dest_rob].executed = 1;

        wb_queue.push(inst);
        wake_up(IQ, inst);

        FU_1_cyc.pop();
    }
    size = FU_2_cyc.size();
    for(int i = 0; i < size; i++)
    {
        instruction inst = FU_2_cyc.front();
        FU_1_cyc.push(inst);
        FU_2_cyc.pop();
    }
    size = FU_3_cyc.size();
    for(int i = 0; i < size; i++)
    {
        instruction inst = FU_3_cyc.front();
        FU_2_cyc.push(inst);
        FU_3_cyc.pop();
    }
    size = FU_4_cyc.size();
    for(int i = 0; i < size; i++)
    {
        instruction inst = FU_4_cyc.front();
        FU_3_cyc.push(inst);
        FU_4_cyc.pop();
    }
    size = FU_5_cyc.size();
    for(int i = 0; i < size; i++)
    {
        instruction inst = FU_5_cyc.front();
        FU_4_cyc.push(inst);
        FU_5_cyc.pop();
    }
}

void Execute(instruction IQ[], rob_entry rob[])
{

    pipeline_execute(rob, IQ);
//    wake_up(IQ,);
}

void Writeback(rob_entry rob[])
{
    if(!wb_queue.empty())
    {
        int size = wb_queue.size();
        for(int i = 0; i < size; i++)
        {
            instruction inst = wb_queue.front();
            inst.timing[8] = cycle_count;
            rob[inst.dest_rob].ready = 1;
            rob[inst.dest_rob].hold_instruction = inst;
            wb_queue.pop();
        }
    }
}

void Retire(rob_entry rob[], instruction IQ[])
{
    for(int i = 0; i < total_width; i++){
        if(rob[rob_head].ready == 1)
        {
            instruction inst = rob[rob_head].hold_instruction;
            inst.timing[9] = cycle_count;
            rob[rob_head].ready = 0;
//        rob[rob_head].executed = 0;
            if (rob[rob_head].dest != -1){
                if(RMT[ rob[rob_head].dest ].rob_tag == rob_head) RMT[ rob[rob_head].dest ].valid = 0;
            }
            wake_up(IQ, inst);


            printf("%d fu{%d} src{%d,%d}", inst.line_number, inst.op_type, inst.src_regA, inst.src_regB);
            printf(" dst{%d} FE{%d,%d}", inst.dest_reg, inst.timing[0], 1);
            printf(" DE{%d,%d} RN{%d,%d}", inst.timing[1]+1, inst.timing[2]-inst.timing[1], inst.timing[2]+1, inst.timing[3]-inst.timing[2]);
            printf(" RR{%d,%d} DI{%d,%d}", inst.timing[3]+1, inst.timing[4]-inst.timing[3], inst.timing[4]+1, inst.timing[5]-inst.timing[4]);
            printf(" IS{%d,%d} EX{%d,%d}", inst.timing[5]+1, inst.timing[6]-inst.timing[5], inst.timing[6]+1, inst.timing[7]-inst.timing[6]);
            printf(" WB{%d,1} RT{%d,%d}\n", inst.timing[7]+1, inst.timing[7]+2, inst.timing[9]-inst.timing[8]);

            rob_head++;
            num_in_rob--;
            if(rob_head >= total_rob_size) rob_head = 0;
        }
    }
}

bool Advance_Cycle()
{

    cycle_count++;
    if ( rob_head == rob_tail
    && de_queue.empty()
    && rn_queue.empty()
    && rr_queue.empty()
    && di_queue.empty()
    && is_queue.empty()
    && FU_1_cyc.empty()
    && FU_2_cyc.empty()
    && FU_3_cyc.empty()
    && FU_4_cyc.empty()
    && FU_5_cyc.empty()
    && wb_queue.empty()
    && rt_queue.empty()
    && end_of_file) return false;
    else return true;
}


int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2;  // Variables are read from trace file
    uint64_t pc; // Variable holds the pc read from input file

    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }

    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
//    printf("rob_size:%lu "
//           "iq_size:%lu "
//           "width:%lu "
//           "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////

//    instruction_count = 0;
//    cycle_count = 0;
    total_width = params.width;
    total_rob_size = params.rob_size;
    total_iq_size = params.iq_size;
    rob_entry rob[params.rob_size];
    instruction issue_queue[params.iq_size];

    do {
        Retire(rob, issue_queue);
        Writeback(rob);
        Execute(issue_queue, rob);
        Issue(issue_queue);
        Dispatch(issue_queue);
        RegRead(rob);
        Rename(rob);
        Decode();
        Fetch(FP);
    } while (Advance_Cycle());

    printf("# === Simulator Command =========");
    printf("\n# ./sim %d %d %d %s", total_rob_size, total_iq_size, total_width, trace_file);
    printf("\n# === Processor Configuration ===");
    printf("\n# ROB_SIZE = %d", total_rob_size);
    printf("\n# IQ_SIZE  = %d", total_iq_size);
    printf("\n# WIDTH    = %d", total_width);
    printf("\n# === Simulation Results ========");
    printf("\n# Dynamic Instruction Count    = %d", instruction_count);
    printf("\n# Cycles                       = %d", cycle_count);
    printf("\n# Instructions Per Cycle (IPC) = %.2f", (double) instruction_count /(double) cycle_count);

    return 0;
}
