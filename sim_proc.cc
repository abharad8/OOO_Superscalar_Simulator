#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include "sim_proc.h"

using namespace std;



bool RMT_Valid_Bit[67];
int RMT_Tag[67];

bool compare_instructions(const Instruction &a, const Instruction &b) 
{
    if(a.id < b.id) return true;
    else return false;
}

void instruction_details(Instruction &inst, string text) 
{
    bool is_single_digit_dest = false;
    int dest_offset_type = 0;

    if (text[9] == '-') 
    {
        inst.dest = -1;
    } 
    else if (text[10] == ' ') 
    {
        inst.dest = text[9] - '0';
        is_single_digit_dest = true; 
    } 
    else 
    {
        inst.dest = atoi(text.substr(9, 2).c_str());
    }


    if (is_single_digit_dest == false) 
    {
        if (text[12] == '-') 
        {
            inst.src1 = -1;
            dest_offset_type = 4;
        } 
        else if (text[13] == ' ') 
        {
            inst.src1 = text[12] - '0';
            dest_offset_type = 3;
        } 
        else 
        {
            inst.src1 = atoi(text.substr(12, 2).c_str());
            dest_offset_type = 4;
        }
    } 
    else 
    { 
        if (text[11] == '-') 
        {
            inst.src1 = -1;
            dest_offset_type = 3;
        } 
        else if (text[12] == ' ') 
        {
            inst.src1 = text[11] - '0';
            dest_offset_type = 2;
        } 
        else 
        {
            inst.src1 = atoi(text.substr(11, 2).c_str());
            dest_offset_type = 3;
        }
    }
    inst.reg1_orig = inst.src1;

    if (dest_offset_type == 2) 
    {
        inst.src2 = atoi(text.substr(13, 2).c_str());
    } 
    else if (dest_offset_type == 3) 
    {
        inst.src2 = atoi(text.substr(14, 2).c_str());
    } 
    else 
    {
        inst.src2 = atoi(text.substr(15, 2).c_str());
    }
    inst.reg2_orig = inst.src2;
}

void instr_fetch_stage(fstream &file) 
{
    if (DecodeList.empty() && !file.eof()) 
    {
        string line;
        while (getline(file, line)) 
        {
            Instruction inst;
            inst.id = total_instructions;
            inst.op_code = line[7] - '0';
            
            inst.s1_ready = false; inst.s2_ready = false;
            inst.s1_in_rob = false; inst.s2_in_rob = false;

            instruction_details(inst, line);

            inst.fe_start = current_cycle;
            inst.fe_duration = 1;
            inst.de_start = current_cycle + 1;

            if (inst.op_code == 0) inst.latency = 1;
            else if (inst.op_code == 1) inst.latency = 2;
            else if (inst.op_code == 2) inst.latency = 5;

            DecodeList.push_back(inst);
            total_instructions++;

            if (DecodeList.size() == GLOBAL_WIDTH) break;
        }
    }
}

void instr_decode_stage() 
{
    if (!DecodeList.empty()) 
    {
        if (RenameList.empty()) 
        {
            for (int i = 0; i < DecodeList.size(); i++) 
            {
                Instruction p = DecodeList[i];
                p.rn_start = current_cycle + 1;
                p.de_duration = p.rn_start - p.de_start;
                RenameList.push_back(p);
            }
            DecodeList.clear();
        }
    }
}

void register_rename_stage() 
{
    if (!RenameList.empty()) 
    {
        if (RegReadList.empty()) 
        {
            
            int slots = 0;
            if (ROB_Tail_Pointer < ROB_Head_Pointer) 
            {
                slots = ROB_Head_Pointer - ROB_Tail_Pointer;
            } 
            else if (ROB_Head_Pointer < ROB_Tail_Pointer) 
            {
                slots = GLOBAL_ROB_SIZE - (ROB_Tail_Pointer - ROB_Head_Pointer);
            } 
            else 
            {
                int check_idx = 0;
                if (ROB_Tail_Pointer < GLOBAL_ROB_SIZE - 1) 
                {
                    check_idx = ROB_Tail_Pointer + 1;
                } 
                else 
                {
                    check_idx = ROB_Tail_Pointer - 1;
                }

                if (ROB_Table[check_idx].dest_reg == 0 && ROB_Table[check_idx].pc_val == 0 && !ROB_Table[check_idx].ready_bit) 
                {
                    slots = GLOBAL_ROB_SIZE;
                } 
                else 
                {
                    slots = 0;
                }
            }

            if ((unsigned)slots < RenameList.size()) return;

            for (int i = 0; i < RenameList.size(); i++) 
            {
                Instruction p = RenameList[i];

                if (p.src1 != -1) 
                {
                    if (RMT_Valid_Bit[p.src1]) 
                    {
                        p.src1 = RMT_Tag[p.src1];
                        p.s1_in_rob = true;
                    }
                }
                if (p.src2 != -1) 
                {
                    if (RMT_Valid_Bit[p.src2]) 
                    {
                        p.src2 = RMT_Tag[p.src2];
                        p.s2_in_rob = true;
                    }
                }

                ROB_Table[ROB_Tail_Pointer].dest_reg = p.dest;
                ROB_Table[ROB_Tail_Pointer].pc_val = p.id;
                ROB_Table[ROB_Tail_Pointer].ready_bit = false;

                if (p.dest != -1) 
                {
                    RMT_Valid_Bit[p.dest] = true;
                    RMT_Tag[p.dest] = ROB_Tail_Pointer;
                }

                p.dest = ROB_Tail_Pointer;

                if (ROB_Tail_Pointer != GLOBAL_ROB_SIZE - 1) ROB_Tail_Pointer++;
                else ROB_Tail_Pointer = 0;

                p.rr_start = current_cycle + 1;
                p.rn_duration = p.rr_start - p.rn_start;
                RegReadList.push_back(p);
            }
            RenameList.clear();
        }
    }
}

void register_read_stage() 
{
    if (!RegReadList.empty()) 
    {
        if (DispatchList.empty()) 
        {
            for (int i = 0; i < RegReadList.size(); i++) 
            {
                Instruction p = RegReadList[i];

                if (p.s1_in_rob) 
                {
                    if (ROB_Table[p.src1].ready_bit) p.s1_ready = true;
                } 
                else 
                {
                    p.s1_ready = true;
                }

                if (p.s2_in_rob) 
                {
                    if (ROB_Table[p.src2].ready_bit) p.s2_ready = true;
                } 
                else 
                {
                    p.s2_ready = true;
                }

                p.di_start = current_cycle + 1;
                p.rr_duration = p.di_start - p.rr_start;
                DispatchList.push_back(p);
            }
            RegReadList.clear();
        }
    }
}

void dispatch_stage() 
{
    if (!DispatchList.empty()) 
    {
        int free_iq = GLOBAL_IQ_SIZE - IssueQueue.size();
        if (free_iq >= DispatchList.size()) 
        {
            for (int i = 0; i < DispatchList.size(); i++) 
            {
                Instruction p = DispatchList[i];
                p.is_start = current_cycle + 1;
                p.di_duration = p.is_start - p.di_start;
                IssueQueue.push_back(p);
            }
            DispatchList.clear();
        }
    }
}

void issue_stage() 
{
    if (!IssueQueue.empty()) 
    {
        sort(IssueQueue.begin(), IssueQueue.end(), compare_instructions);
        
        int count = 0;
        bool check = true;
        while (check) 
        {
            check = false;
            for (int i = 0; i < IssueQueue.size(); i++) 
            {
                if (IssueQueue[i].s1_ready && IssueQueue[i].s2_ready) 
                {
                    Instruction p = IssueQueue[i];
                    p.ex_start = current_cycle + 1;
                    p.is_duration = p.ex_start - p.is_start;

                    ExecuteList.push_back(p);
                    IssueQueue.erase(IssueQueue.begin() + i);
                    
                    count++;
                    check = true;
                    break;
                }
            }
            if (count == GLOBAL_WIDTH) break;
        }
    }
}

void execute_stage() {
    if (!ExecuteList.empty()) 
    {
        for (int i = 0; i < ExecuteList.size(); i++) 
        {
            ExecuteList[i].latency--;
        }

        bool working = true;
        while (working) 
        {
            working = false;
            for (int i = 0; i < ExecuteList.size(); i++) 
            {
                if (ExecuteList[i].latency == 0) 
                {
                    Instruction p = ExecuteList[i];
                    p.wb_start = current_cycle + 1;
                    p.ex_duration = p.wb_start - p.ex_start;

                    WritebackList.push_back(p);

                    for (int k=0; k<IssueQueue.size(); k++) 
                    {
                        if (IssueQueue[k].src1 == p.dest) IssueQueue[k].s1_ready = true;
                        if (IssueQueue[k].src2 == p.dest) IssueQueue[k].s2_ready = true;
                    }
                    for (int k=0; k<DispatchList.size(); k++) 
                    {
                        if (DispatchList[k].src1 == p.dest) DispatchList[k].s1_ready = true;
                        if (DispatchList[k].src2 == p.dest) DispatchList[k].s2_ready = true;
                    }
                    for (int k=0; k<RegReadList.size(); k++) 
                    {
                        if (RegReadList[k].src1 == p.dest) RegReadList[k].s1_ready = true;
                        if (RegReadList[k].src2 == p.dest) RegReadList[k].s2_ready = true;
                    }

                    ExecuteList.erase(ExecuteList.begin() + i);
                    working = true;
                    break;
                }
            }
        }
    }
}

void writeback_stage() 
{
    if (!WritebackList.empty()) 
    {
        for (int i = 0; i < WritebackList.size(); i++) 
        {
            int rob_idx = WritebackList[i].dest;
            ROB_Table[rob_idx].ready_bit = true;

            WritebackList[i].rt_start = current_cycle + 1;
            WritebackList[i].wb_duration = WritebackList[i].rt_start - WritebackList[i].wb_start;
            RetireList.push_back(WritebackList[i]);
        }
        WritebackList.clear();
    }
}

void retire_stage() 
{
    int count = 0;
    while (count < GLOBAL_WIDTH) 
    {
        if (ROB_Head_Pointer == ROB_Tail_Pointer && ROB_Head_Pointer != GLOBAL_ROB_SIZE - 1) 
        {
            if (ROB_Table[ROB_Head_Pointer + 1].pc_val == 0) return;
        }

        if (ROB_Table[ROB_Head_Pointer].ready_bit) 
        {
            for (int i = 0; i < RegReadList.size(); i++) 
            {
                if (RegReadList[i].src1 == ROB_Head_Pointer) RegReadList[i].s1_ready = true;
                if (RegReadList[i].src2 == ROB_Head_Pointer) RegReadList[i].s2_ready = true;
            }

            for (int i = 0; i < RetireList.size(); i++) 
            {
                if (RetireList[i].id == ROB_Table[ROB_Head_Pointer].pc_val) 
                {
                    Instruction p = RetireList[i];
                    p.rt_duration = (current_cycle + 1) - p.rt_start;

                    // UPDATED: Using printf instead of cout
                    printf("%d fu{%d} src{%d,%d} dst{%d} FE{%d,%d} DE{%d,%d} RN{%d,%d} RR{%d,%d} DI{%d,%d} IS{%d,%d} EX{%d,%d} WB{%d,%d} RT{%d,%d}\n",
                        p.id, p.op_code,
                        p.reg1_orig, p.reg2_orig,
                        ROB_Table[ROB_Head_Pointer].dest_reg,
                        p.fe_start, p.fe_duration,
                        p.de_start, p.de_duration,
                        p.rn_start, p.rn_duration,
                        p.rr_start, p.rr_duration,
                        p.di_start, p.di_duration,
                        p.is_start, p.is_duration,
                        p.ex_start, p.ex_duration,
                        p.wb_start, p.wb_duration,
                        p.rt_start, p.rt_duration
                    );

                    RetireList.erase(RetireList.begin() + i);
                    break;
                }
            }

            for (int k = 0; k < 67; k++) 
            {
                if (RMT_Tag[k] == ROB_Head_Pointer) 
                {
                    RMT_Valid_Bit[k] = false;
                    RMT_Tag[k] = 0;
                }
            }

            ROB_Table[ROB_Head_Pointer].dest_reg = 0;
            ROB_Table[ROB_Head_Pointer].pc_val = 0;
            ROB_Table[ROB_Head_Pointer].ready_bit = false;

            if (ROB_Head_Pointer != GLOBAL_ROB_SIZE - 1) ROB_Head_Pointer++;
            else ROB_Head_Pointer = 0;

            count++;
        } 
        else 
        {
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 5) return -1;

    GLOBAL_ROB_SIZE = atoi(argv[1]);
    GLOBAL_IQ_SIZE = atoi(argv[2]);
    GLOBAL_WIDTH = atoi(argv[3]);
    string trace_path = argv[4];

    ROB_Head_Pointer = 3;
    ROB_Tail_Pointer = 3;
    
    for (int i = 0; i < GLOBAL_ROB_SIZE; i++) {
        RobItem temp;
        temp.dest_reg = 0; temp.pc_val = 0; temp.ready_bit = false;
        ROB_Table.push_back(temp);
    }
    for (int i = 0; i < 67; i++) {
        RMT_Valid_Bit[i] = false; RMT_Tag[i] = -1;
    }

    fstream file;
    file.open(trace_path.c_str(), ios::in);

    while (true) {
        if (simulation_done && file.eof()) break;

        retire_stage();
        writeback_stage();
        execute_stage();
        issue_stage();
        dispatch_stage();
        register_read_stage();
        register_rename_stage();
        instr_decode_stage();
        instr_fetch_stage(file);

        if (DecodeList.empty() && RenameList.empty() && 
            RegReadList.empty() && DispatchList.empty() && 
            IssueQueue.empty() && ExecuteList.empty() && 
            WritebackList.empty()) {
            
            if (ROB_Head_Pointer == ROB_Tail_Pointer) {
                if (ROB_Table[ROB_Tail_Pointer].pc_val == 0) simulation_done = true;
            }
        }
        current_cycle++;
    }

    file.close();

    printf("# === Simulator Command =========\n");
    printf("# ");
    for (int i = 0; i < argc; i++) printf("%s ", argv[i]);
    printf("\n");

    printf("# === Processor Configuration ===\n");
    printf("# ROB_SIZE = %d\n", GLOBAL_ROB_SIZE);
    printf("# IQ_SIZE  = %d\n", GLOBAL_IQ_SIZE);
    printf("# WIDTH    = %d\n", GLOBAL_WIDTH);

    printf("# === Simulation Results ========\n");
    printf("# Dynamic Instruction Count    = %d\n", total_instructions);
    printf("# Cycles                       = %d\n", current_cycle);
    printf("# Instructions Per Cycle (IPC) = %.2f\n", ((float)total_instructions / (float)current_cycle));

    return 0;
}