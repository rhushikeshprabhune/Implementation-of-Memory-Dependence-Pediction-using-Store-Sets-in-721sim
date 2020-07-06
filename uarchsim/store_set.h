#ifndef __STORE_SET_H_
#define __STORE_SET_H_

#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string> 
#include <iterator>

class store_set
{
    public:
        store_set();
        ~store_set();

        std::map<int, int> ssit; // Store Set Identifier Table
        std::map<int, int> lfst; // Last Fetched Store Table
        int ssid_counter; // Store Set Assignment Counter

        // Funcs related to SSIT
        int lookup_ssit(uint64_t instr_pc);
        void update_ssit(uint64_t pc_load, uint64_t pc_store);
        void cyclic_clear_ssit();
        void invalidate_ssit_all(int num_insn);

        // Funcs related to LFST
        int lookup_lfst(int ssid);
        int update_lfst(int pc_store);
        void invalidate_lfst(int pc_store);
        void clear_lfst();
        void update_lfst_load(int pc_load, int pc_store);
        void invalidate_lfst_all(int num_insn);
        void update_lfst(int pc, int store_AL_index);      
};

#endif