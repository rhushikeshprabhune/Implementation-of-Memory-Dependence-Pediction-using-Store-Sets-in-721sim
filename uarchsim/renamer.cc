#include "renamer.h"
#include <assert.h>
#include <cstring>

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs, uint64_t n_branches)
{
    assert(n_phys_regs > n_log_regs);
    assert((n_branches >= 1)&&(n_branches <= 64));
    
    logic_regs = n_log_regs;
    physical_regs = n_phys_regs;
    unres_branch = n_branches;
    freelist_size = n_phys_regs - n_log_regs;
    actvlist_size = n_phys_regs - n_log_regs;
    
    init_rmt();
    init_amt();
    init_freelist();
    init_actvlist();
    init_phy_regfile();
    init_GBM();
    init_GBM_bak();
}

void renamer::init_rmt()
{
    rmap_table = new uint64_t[logic_regs];
    
    for(uint64_t i = 0; i < logic_regs; i++)
    {
        rmap_table[i] = i;
    }
}

void renamer::init_amt()
{
    amap_table = new uint64_t[logic_regs];
    
    for(uint64_t i = 0; i < logic_regs; i++)
    {
        amap_table[i] = i;
    }
}

void renamer::init_freelist()
{
    free_list = new uint64_t[freelist_size];
    
    freelist_head = 0;
    freelist_tail = 0;
    freelist_entries = freelist_size;

    for(uint64_t i = 0; i < freelist_size; i++)
    {
        free_list[i] = logic_regs + i;
    }

}

void renamer::init_actvlist()
{
    active_list = new actv_list[actvlist_size];
    
    actvlist_head = 0;
    actvlist_tail = 0;
    actvlist_entries = 0;
}

void renamer::init_phy_regfile()
{
    physical_regf = new uint64_t[physical_regs];
    phy_regf_ready = new bool[physical_regs];
    
    for(uint64_t i = 0; i < physical_regs; i++)
    {
        physical_regf[i] = i;
        phy_regf_ready[i] = true;
    }
}

void renamer::init_GBM()
{
    GBM = 0;
}

void renamer::init_GBM_bak()
{
    branch_checkpoint = new br_checkpoint[unres_branch];
    
    for(uint64_t i = 0; i < unres_branch; i++)
    {
        branch_checkpoint[i].rmap_table = new uint64_t[logic_regs];
    }
}

renamer::~renamer()
{
    free_rmt();
    free_amt();
    free_freelist();
    free_actvlist();
    free_phy_regfile();
    free_GBM_bak();
}

void renamer::free_rmt()
{
    assert(rmap_table != NULL);
    delete[] rmap_table;
}

void renamer::free_amt()
{
    assert(amap_table != NULL);
    delete[] amap_table;
}

void renamer::free_freelist()
{
    assert(free_list != NULL);
    delete[] free_list;
}

void renamer::free_actvlist()
{
    assert(active_list != NULL);
    delete[] active_list;
}

void renamer::free_phy_regfile()
{
    delete[] physical_regf;
    delete[] phy_regf_ready;
}

void renamer::free_GBM_bak()
{
    for(uint64_t i = 0; i < unres_branch; i++)
    {
        assert(branch_checkpoint[i].rmap_table != NULL);
        delete[] branch_checkpoint[i].rmap_table;
    }
    
    assert(branch_checkpoint != NULL);
    delete[] branch_checkpoint;
}

void renamer::bak_rmt(uint64_t branch_id)
{
    assert(branch_id < unres_branch);
    memcpy((void*)branch_checkpoint[branch_id].rmap_table, (const void*)rmap_table, logic_regs*sizeof(uint64_t));
}

void renamer::bak_freelist_head(uint64_t branch_id)
{
    assert(branch_id < unres_branch);
    branch_checkpoint[branch_id].freelist_head = freelist_head;
}

void renamer::bak_gbm(uint64_t branch_id)
{
    assert(branch_id < unres_branch);
    branch_checkpoint[branch_id].GBM_bak = GBM;
}

void renamer::restore_amt_rmt()
{
    for(uint64_t i = 0; i < logic_regs; i++)
    {
        rmap_table[i] = amap_table[i];
    }
}

void renamer::restore_rmt(uint64_t branch_id)
{
    assert(branch_id < unres_branch);
    memcpy((void *)rmap_table, (const void *)branch_checkpoint[branch_id].rmap_table, logic_regs * sizeof(uint64_t));
}

void renamer::restore_freelist(uint64_t branch_id)
{
    assert(branch_id < unres_branch);
    uint64_t restore_hptr = branch_checkpoint[branch_id].freelist_head;
    
    while(freelist_head != restore_hptr)
    {
        if(freelist_head == 0)
        {
            freelist_head = freelist_size - 1;
        }
        else
        {
            freelist_head--;
        }
        
        assert(freelist_entries < freelist_size);
        freelist_entries++;
    }
    
    uint64_t i = freelist_entries;
    uint64_t j = freelist_head;
    while(i > 0)
    {
        uint64_t preg_num = free_list[j];
        phy_regf_ready[preg_num] = true;
        j++;

        if(j == freelist_size)
        {
            j = 0;
        }
        i--;
    }
}

void renamer::restore_actvlist(uint64_t al_id)
{
    assert(al_id < actvlist_size);
    
    al_id++;
    if(al_id == actvlist_size)
    {
        al_id = 0;
    }
    
    while(actvlist_tail != al_id)
    {
        if(actvlist_tail == 0)
        {
            actvlist_tail = actvlist_size - 1;
        }
        else
        {
            actvlist_tail--;
        }
        assert(actvlist_entries > 0);
        actvlist_entries--;
    }
}

void renamer::restore_GBM(uint64_t branch_id)
{
    assert(branch_id < unres_branch);
    GBM = branch_checkpoint[branch_id].GBM_bak;
}

void renamer::set_gbmbit(uint64_t branch_id)
{
    assert(branch_id < unres_branch);
    uint64_t set_mask = (1 << branch_id);
    uint64_t res_bit = GBM & set_mask;
    assert(res_bit == 0);
    GBM = GBM | set_mask;
}

uint64_t renamer::find_gbmfreebit()
{
    uint64_t i;
    for(i = 0; i < unres_branch; i++)
    {
        uint64_t ext_mask = (1<<i);
        uint64_t res_bit = GBM & ext_mask;
        if(res_bit == 0)
        {
            return i;
        }
    }
    
    return i;
}

void renamer::clear_gbmbit(uint64_t branch_id)
{
    assert(branch_id < unres_branch);
    uint64_t ext_mask = (1 << branch_id);
    uint64_t clr_mask = ~(1 << branch_id);
    uint64_t res_bit = GBM & ext_mask;
    assert(res_bit > 0);
    GBM = GBM & clr_mask;
}

uint64_t renamer::num_gbmfreebits()
{
    uint64_t ext_mask;
    uint64_t i;
    uint64_t free_cnt = 0;
    for(i = 0; i < unres_branch; i++)
    {
        ext_mask = (1 << i);
        uint64_t res_bit = GBM & ext_mask;
        if(res_bit == 0)
        {
            free_cnt++;
        }
    }
    
    return free_cnt;
}

void renamer::return_freereg(uint64_t preg_id)
{
    assert(preg_id < physical_regs);
    assert(freelist_entries < freelist_size);
    
    free_list[freelist_tail] = preg_id;
    freelist_tail++;
    
    if(freelist_tail == freelist_size)
    {
        freelist_tail = 0;
    }
    freelist_entries++;
}

uint64_t renamer::get_freereg()
{
    assert(freelist_entries > 0);
    uint64_t reg_to_return = free_list[freelist_head];
    
    freelist_head++;
    if(freelist_head == freelist_size)
    {
        freelist_head = 0;
    }
    freelist_entries--;
    
    phy_regf_ready[reg_to_return] = false;
    
    return reg_to_return;
}

void renamer::squash_actvlist()
{
    actvlist_head = 0;
    actvlist_tail = 0;
    actvlist_entries = 0;
}

void renamer::squash_freelist()
{
    freelist_head = freelist_tail;
    freelist_entries = freelist_size;
    
    uint64_t i = freelist_entries;
    uint64_t j = freelist_head;
    while(i != 0)
    {
        uint64_t preg_num = free_list[j];
        phy_regf_ready[preg_num] = true;
        j++;

        if(j == freelist_size)
        {
            j = 0;
        }
        i--;
    }
}

bool renamer::stall_reg(uint64_t bundle_dst)
{
    if(freelist_entries < bundle_dst)
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint64_t renamer::get_branch_mask()
{
    return GBM;
}

bool renamer::stall_branch(uint64_t bundle_branch)
{
    uint64_t free_bnos = num_gbmfreebits();
    if(free_bnos < bundle_branch)
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint64_t renamer::rename_rsrc(uint64_t log_reg)
{
    assert(log_reg < logic_regs);
    return rmap_table[log_reg];
}

uint64_t renamer::rename_rdst(uint64_t log_reg)
{
    assert(log_reg < logic_regs);
    uint64_t reg_to_ret = get_freereg();
    rmap_table[log_reg] = reg_to_ret;
    return reg_to_ret;
}

uint64_t renamer::checkpoint()
{
    uint64_t bran_id = find_gbmfreebit();
    set_gbmbit(bran_id);
    bak_rmt(bran_id);
    bak_freelist_head(bran_id);
    bak_gbm(bran_id);
    return bran_id;
}

uint64_t renamer::dispatch_inst(bool dest_valid,
                       uint64_t log_reg,
                       uint64_t phys_reg,
                       bool load,
                       bool store,
                       bool branch,
                       bool amo,
                       bool csr,
                       uint64_t PC)
{
    assert(actvlist_entries < actvlist_size);
    
    uint64_t actv_lst_indx = actvlist_tail;
    
    active_list[actv_lst_indx].dest_used = dest_valid;
    
    if(dest_valid == true)
    {
        assert(log_reg < logic_regs);
        assert(phys_reg < physical_regs);
        
        active_list[actv_lst_indx].logic_reg_num = log_reg;
        active_list[actv_lst_indx].phy_reg_num = phys_reg;
    }
    
    active_list[actv_lst_indx].instr_complete = false;
    active_list[actv_lst_indx].excep_flag = false;
    active_list[actv_lst_indx].ldv_flag = false;
    active_list[actv_lst_indx].brmispred_flag = false;
    active_list[actv_lst_indx].valmispred_flag = false;
    active_list[actv_lst_indx].load_ins = load;
    active_list[actv_lst_indx].store_ins = store;
    active_list[actv_lst_indx].branch_ins = branch;
    active_list[actv_lst_indx].atomic_ins = amo;
    active_list[actv_lst_indx].sys_ins = csr;
    active_list[actv_lst_indx].PC_addr = PC;
    actvlist_tail++;
    
    if(actvlist_tail == actvlist_size)
    {
        actvlist_tail = 0;
    }
    actvlist_entries++;
    
    return actv_lst_indx;
}

bool renamer::stall_dispatch(uint64_t bundle_inst)
{
    uint64_t free_regs = actvlist_size - actvlist_entries;
    if(free_regs < bundle_inst)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool renamer::is_ready(uint64_t phys_reg)
{
    assert(phys_reg < physical_regs);
    return phy_regf_ready[phys_reg];
}

void renamer::clear_ready(uint64_t phys_reg)
{
    assert(phys_reg < physical_regs);
    phy_regf_ready[phys_reg] = false;
}

void renamer::set_ready(uint64_t phys_reg)
{
    assert(phys_reg < physical_regs);
    phy_regf_ready[phys_reg] = true;
}

uint64_t renamer::read(uint64_t phys_reg)
{
    assert(phys_reg < physical_regs);
    return physical_regf[phys_reg];
}

void renamer::write(uint64_t phys_reg, uint64_t value)
{
    assert(phys_reg < physical_regs);
    physical_regf[phys_reg] = value;
}

void renamer::set_complete(uint64_t AL_index)
{
    assert(AL_index < actvlist_size);
    active_list[AL_index].instr_complete = true;
}

void renamer::resolve(uint64_t AL_index, uint64_t branch_ID, bool correct)
{
    assert(AL_index < actvlist_size);
    assert(branch_ID < unres_branch);
    
    if(correct == true)
    {
        uint64_t clr_mask = ~(1 << branch_ID);
        GBM = GBM & clr_mask;
        for(uint64_t i = 0; i < unres_branch; i++)
        {
            branch_checkpoint[i].GBM_bak = branch_checkpoint[i].GBM_bak & clr_mask;
        }
    }
    else
    {
        restore_GBM(branch_ID);
        clear_gbmbit(branch_ID);
        restore_actvlist(AL_index);
        restore_freelist(branch_ID);
        restore_rmt(branch_ID);
    }
}

bool renamer::precommit(bool &completed,
               bool &exception, bool &load_viol, bool &br_misp, bool &val_misp,
               bool &load, bool &store, bool &branch, bool &amo, bool &csr,
               uint64_t &PC, uint64_t &offendStorePC, uint32_t &offendStoreALIndex)
{
    if(actvlist_entries == 0)
    {
        return false;
    }
    else
    {
        completed = active_list[actvlist_head].instr_complete;
        exception = active_list[actvlist_head].excep_flag;
        load_viol = active_list[actvlist_head].ldv_flag;
        br_misp = active_list[actvlist_head].brmispred_flag;
        val_misp = active_list[actvlist_head].valmispred_flag;
        load = active_list[actvlist_head].load_ins;
        store = active_list[actvlist_head].store_ins;
        branch = active_list[actvlist_head].branch_ins;
        amo = active_list[actvlist_head].atomic_ins;
        csr = active_list[actvlist_head].sys_ins;
        PC = active_list[actvlist_head].PC_addr;
        offendStorePC = active_list[actvlist_head].store_pc;
        offendStoreALIndex = active_list[actvlist_head].store_inum;

        return true;
    }
}

void renamer::commit()
{
    assert(actvlist_entries > 0);
    assert(active_list[actvlist_head].instr_complete == true);
    assert(active_list[actvlist_head].excep_flag == false);
    assert(active_list[actvlist_head].ldv_flag == false);
    
    if(active_list[actvlist_head].dest_used == true)
    {
        uint64_t log_regid = active_list[actvlist_head].logic_reg_num;
        uint64_t reg_to_free = amap_table[log_regid];
        return_freereg(reg_to_free);
        amap_table[log_regid] = active_list[actvlist_head].phy_reg_num;
    }
    
    actvlist_head++;
    if(actvlist_head == actvlist_size)
    {
        actvlist_head = 0;
    }
    actvlist_entries--;
}

void renamer::set_exception(uint64_t AL_index)
{
    assert(AL_index < actvlist_size);
    active_list[AL_index].excep_flag = true;
}
    
void renamer::set_load_violation(uint64_t AL_index)
{
    assert(AL_index < actvlist_size);
    active_list[AL_index].ldv_flag = true;
}
    
void renamer::set_branch_misprediction(uint64_t AL_index)
{
    assert(AL_index < actvlist_size);
    active_list[AL_index].brmispred_flag = true;
}
    
void renamer::set_value_misprediction(uint64_t AL_index)
{
    assert(AL_index < actvlist_size);
    active_list[AL_index].valmispred_flag = true;
}
    
bool renamer::get_exception(uint64_t AL_index)
{
    assert(AL_index < actvlist_size);
    return active_list[AL_index].excep_flag;
}

void renamer::squash()
{
    restore_amt_rmt();
    squash_freelist();
    squash_actvlist();
    GBM = 0;
}

void renamer::set_store_pc(uint64_t store_pc, unsigned int AL_index)
{
    assert(AL_index < actvlist_size);
    active_list[AL_index].store_pc = store_pc;
}

void renamer::set_store_inum(unsigned int AL_index, unsigned int store_inum)
{
    assert(AL_index < actvlist_size);
    active_list[AL_index].store_inum = store_inum;
}
