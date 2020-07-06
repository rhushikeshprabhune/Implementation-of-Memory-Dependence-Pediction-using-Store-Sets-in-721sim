#include "store_set.h"
#include <map>
#include<string> 
#include <iostream> 
#include <iterator>

using namespace std;

// Class Constructor
store_set::store_set()
{
    ssid_counter = 1;
}

// Class Deconstructor
store_set::~store_set()
{
}

// Lookup SSIT Table
int store_set::lookup_ssit(uint64_t instr_pc)
{ 
    if(ssit.find(instr_pc) != ssit.end())
    {
        
        return ssit[instr_pc];
    }   
    else
    {
        return -1;
    }
}


// Update SSIT Table
void store_set::update_ssit(uint64_t pc_load, uint64_t pc_store)
{
    int load_ssid = -1;
    int store_ssid = -1;
    int final_ssid;

    if(ssit.find(pc_load) != ssit.end())
    {
        load_ssid = ssit[pc_load];
    }
    
    if(ssit.find(pc_store) != ssit.end())
    {
        store_ssid = ssit[pc_store];
    }

    if((load_ssid != -1) && (store_ssid != -1))
    {
        if(load_ssid >= store_ssid)
        {
            final_ssid = store_ssid;
        }
        else
        {
            final_ssid = load_ssid;
        } 
    }

    if(load_ssid == -1 && store_ssid != -1)
    {
        final_ssid = store_ssid;
    }

    if(load_ssid != -1 && store_ssid == -1)
    {
        final_ssid = load_ssid;
    }

    if(load_ssid == -1 && store_ssid == -1)
    {
        final_ssid = ssid_counter;
        ssid_counter++;
    }

    ssit[pc_load] = final_ssid;
    ssit[pc_store] = final_ssid;
}

// Lookup LFST Table
int store_set::lookup_lfst(int ssid)
{
    //if(ssid != -1)
    //{
      //  if(lfst.find(ssid) != lfst.end())
        //{
            return lfst[ssid];
        //}
   // }
    //else
    //{
      //  return -1;
    //}
}


// Invalidate LFST Table
void store_set::invalidate_lfst(int ssid)
{
    lfst[ssid] = 0;
}


/// Update LFST Table by Store
void store_set::update_lfst(int pc, int storeInum)
{
    int ssid = lookup_ssit(pc);

    if(ssid != -1)
    {
        lfst[ssid] = storeInum;
    }
}