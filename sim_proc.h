#ifndef SIM_H
#define SIM_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <algorithm>

using namespace std;

struct Instruction {
    int id;               
    int op_code;
    int dest, src1, src2; 
    int reg1_orig, reg2_orig; 
    int latency;

    bool s1_ready;
    bool s2_ready;
    bool s1_in_rob;
    bool s2_in_rob;

    int fe_start, fe_duration;
    int de_start, de_duration;
    int rn_start, rn_duration;
    int rr_start, rr_duration;
    int di_start, di_duration;
    int is_start, is_duration;
    int ex_start, ex_duration;
    int wb_start, wb_duration;
    int rt_start, rt_duration;
};

struct RobItem {
    int dest_reg;
    int pc_val;
    bool ready_bit;
};

int GLOBAL_ROB_SIZE;
int GLOBAL_IQ_SIZE;
int GLOBAL_WIDTH;
int current_cycle = 0;
int total_instructions = 0;
bool simulation_done = false;

vector<Instruction> FetchList;
vector<Instruction> DecodeList;
vector<Instruction> RenameList;
vector<Instruction> RegReadList;
vector<Instruction> DispatchList;
vector<Instruction> IssueQueue;
vector<Instruction> ExecuteList;
vector<Instruction> WritebackList;
vector<Instruction> RetireList;

vector<RobItem> ROB_Table;
int ROB_Head_Pointer = 0;
int ROB_Tail_Pointer = 0;

bool compare_instructions(const Instruction &a, const Instruction &b);
void parse_line(Instruction &inst, string text);

void retire_stage();
void writeback_stage();
void execute_stage();
void issue_stage();
void dispatch_stage();
void register_read_stage();
void register_rename_stage();
void instr_decode_stage();
void instr_fetch_stage(fstream &file);

#endif