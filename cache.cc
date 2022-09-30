#include "cache.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include<vector>
#include <string>

//uint64_t l2pf_access = 0;
using namespace std;

int lg2(int n)
{
    int i, m = n, c = -1;
    for (i=0; m; i++) {
        m /= 2;
        c++;
    }
    return c;
}

void CACHE::handle_read(fstream &fin)
{
    // handle read
    uint32_t read_cpu = 0;
//    if (read_cpu == NUM_CPUS)
//        return;
    string line;
    llc_prefetcher_initialize(read_cpu);
    long double cnt_acc=0,cnt_miss=0;

    while(getline(fin, line)){

       stringstream s(line);
       string word;
       uint64_t ip = 0;
       uint64_t index = 0;
       s >> word;
       ip = stol(word);
       s >> word;
       index = stol(word);
       //cout<<"ip: " <<ip<< "  address: "<<index<<endl;

        // access cache
        uint32_t set = get_set(index);
        int way = check_hit(index);
        //cout<<" got way and set,      ******    set:"<<set<<"   way:" <<way<<endl;

        if (way >= 0) { // read hit

            // update prefetcher on load instruction
            uint64_t pf_addr=llc_prefetcher_operate(read_cpu,index, ip, 1, LOAD);//Bingo


//            uint32_t pf_set = get_set(pf_addr);
//            int pf_way = check_hit(pf_addr);
            handle_prefech(cnt_acc,cnt_miss,pf_addr,ip);

            // update replacement policyx

            llc_update_replacement_state(read_cpu, set, way, index, ip, 0, LOAD, 1);

            // COLLECT STATS
//                sim_hit[read_cpu][RQ.entry[index].type]++;
//                sim_access[read_cpu][RQ.entry[index].type]++;

            // update prefetch stats and reset prefetch bit
            if (block[set][way].prefetch) {
                pf_useful++;
                block[set][way].prefetch = 0;
            }
            block[set][way].used = 1;
            HIT[LOAD]++;
            ACCESS[LOAD]++;
            cnt_acc++;
            // remove this entry from RQ

        }
        else { // read miss

            // check mshr
            uint8_t miss_handled = 1;


            // find victim


            way = llc_find_victim(read_cpu, set, block[set], ip, index, LOAD);

            //cout<<"the newly found way is  "<<  way <<endl;

            if(way==LLC_WAY) {
                llc_update_replacement_state(read_cpu, set, way, index, ip, 0, LOAD, 0);
            }
            uint8_t do_fill = 1;


            if(do_fill){

                //cout<<"the block info : set "<<set<<"   way: "<<way<<"   block[set][way]: "<<block[set][way].full_addr<<endl;

                llc_prefetcher_cache_fill(read_cpu,index, set, way,  1 , block[set][way].full_addr);//Bingo


                llc_update_replacement_state(cpu, set, way, index, ip, block[set][way].full_addr, LOAD, 0);
                fill_cache(set, way, index);
                block[set][way].dirty = 1;
                cnt_miss++;
            }



            if (miss_handled){
                // update prefetcher on load instruction

                uint64_t pf_addr=llc_prefetcher_operate(read_cpu, index, ip, 0, LOAD);//Bingo
                handle_prefech(cnt_acc,cnt_miss,pf_addr,ip);
                MISS[LOAD]++;
                ACCESS[LOAD]++;

            }


        }

    }
    cout<<"hit rate: "<< cnt_acc/(cnt_acc+cnt_miss)<<endl;
    cout<<cnt_acc<<endl;
}

void CACHE::handle_prefech(double long &cnt_acc, double long &cnt_miss,uint64_t pf_addr,uint64_t ip){

    uint32_t set = get_set(pf_addr);
    int way = check_hit(ip);

    if (way >= 0) {
        //prefetch hit
        llc_update_replacement_state(cpu, set, way, block[set][way].full_addr, ip, 0,LOAD , 1);
        cnt_acc++;
    }else{
        // prefetch miss
        way = llc_find_victim(cpu, set, block[set], ip, pf_addr, LOAD);
        llc_prefetcher_cache_fill(cpu,pf_addr, set, way,  1 , block[set][way].full_addr);//Bingo
        fill_cache(set, way, pf_addr);
        block[set][way].dirty = 1;

        cnt_miss++;
        }
    }

void CACHE::operate()
{
//    handle_fill();
//    handle_writeback();
    fstream fin;
    string filename="../matrix0001.txt";
    fin.open(filename, ios::in);
    handle_read(fin);

//    if (PQ.occupancy && (RQ.occupancy == 0))
//        handle_prefetch();
}

uint32_t CACHE::get_set(uint64_t address)
{
    return (uint32_t) (address & ((1 << lg2(NUM_SET)) - 1)); 
}

uint32_t CACHE::get_way(uint64_t address, uint32_t set)
{
    for (uint32_t way=0; way<NUM_WAY; way++) {
        if (block[set][way].valid && (block[set][way].tag == address)) 
            return way;
    }

    return NUM_WAY;
}

void CACHE::fill_cache(uint32_t set, uint32_t way, uint64_t addr)
{

    if (block[set][way].prefetch && (block[set][way].used == 0))
        pf_useless++;

    if (block[set][way].valid == 0)
        block[set][way].valid = 1;
    block[set][way].dirty = 0;
   // block[set][way].prefetch = (packet->type == PREFETCH) ? 1 : 0;
    block[set][way].used = 0;

    if (block[set][way].prefetch)
        pf_fill++;

    //block[set][way].delta = packet->delta;
    //block[set][way].depth = packet->depth;
    //block[set][way].signature = packet->signature;
    //block[set][way].confidence = packet->confidence;

    block[set][way].tag = addr;
    block[set][way].address = addr;
    block[set][way].full_addr = addr;
    //block[set][way].data = packet->data;
    block[set][way].cpu = cpu;
    //block[set][way].instr_id = packet->instr_id;

    DP ( if (warmup_complete[packet->cpu]) {
    cout << "[" << NAME << "] " << __func__ << " set: " << set << " way: " << way;
    cout << " lru: " << block[set][way].lru << " tag: " << hex << block[set][way].tag << " full_addr: " << block[set][way].full_addr;
    cout << " data: " << block[set][way].data << dec << "\tpacket->cpu = " << packet->cpu  << endl; });
}

int CACHE::check_hit(uint64_t index)
{
    uint32_t set = get_set(index);
    int match_way = -1;

    if (NUM_SET < set) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << NUM_SET;
        cerr << " address: " << hex << index << " full_addr: " << index << dec;
//        cerr << " event: " << packet->event_cycle << endl;
        assert(0);
    }

    // hit
    for (uint32_t way=0; way<NUM_WAY; way++) {
        if (block[set][way].valid && (block[set][way].tag == index)) {

            match_way = way;

            DP ( if (warmup_complete[packet->cpu]) {
            cout << "[" << NAME << "] " << __func__ << " instr_id: " << packet->instr_id << " type: " << +packet->type << hex << " addr: " << packet->address;
            cout << " full_addr: " << packet->full_addr << " tag: " << block[set][way].tag << " data: " << block[set][way].data << dec;
            cout << " set: " << set << " way: " << way << " lru: " << block[set][way].lru;
            cout << " event: " << packet->event_cycle << " cycle: " << current_core_cycle[cpu] << "\tpacket->cpu = " << packet->cpu << endl; });

            break;
        }
    }

    return match_way;
}

int CACHE::invalidate_entry(uint64_t inval_addr)
{
    uint32_t set = get_set(inval_addr);
    int match_way = -1;

    if (NUM_SET < set) {
        cerr << "[" << NAME << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << NUM_SET;
        cerr << " inval_addr: " << hex << inval_addr << dec << endl;
        assert(0);
    }

    // invalidate
    for (uint32_t way=0; way<NUM_WAY; way++) {
        if (block[set][way].valid && (block[set][way].tag == inval_addr)) {

            block[set][way].valid = 0;

            match_way = way;

            DP ( if (warmup_complete[cpu]) {
            cout << "[" << NAME << "] " << __func__ << " inval_addr: " << hex << inval_addr;  
            cout << " tag: " << block[set][way].tag << " data: " << block[set][way].data << dec;
            cout << " set: " << set << " way: " << way << " lru: " << block[set][way].lru << " cycle: " << current_core_cycle[cpu] << "\tcpu = " << cpu << endl; });

            break;
        }
    }

    return match_way;
}


int CACHE::prefetch_line(uint32_t cpu, uint64_t ip, uint64_t base_addr, uint64_t pf_addr, int fill_level)
{
    pf_requested++;
//    cout<<"oooo"<<endl;
    if ((base_addr>>LOG2_PAGE_SIZE) == (pf_addr>>LOG2_PAGE_SIZE)) {
//        cout<<"iiii"<<endl;
        PACKET pf_packet;
        pf_packet.fill_level = fill_level;
        pf_packet.cpu = cpu;
        pf_packet.address = pf_addr >> LOG2_BLOCK_SIZE;
        pf_packet.full_addr = pf_addr;
        //pf_packet.instr_id = LQ.entry[lq_index].instr_id;
        //pf_packet.rob_index = LQ.entry[lq_index].rob_index;
        pf_packet.ip = ip;
        pf_packet.type = PREFETCH;
        //pf_packet.event_cycle = current_core_cycle[cpu];	//Bingo: Handle late prefetches, here.

        // give a dummy 0 as the IP of a prefetch
        //add_pq(&pf_packet);
        pf_issued++;

        return 1;
    }


    return 0;
}



//void CACHE::return_data(PACKET *packet)	//Bingo: This function should be heavily modified for LLC, supporting multiple backward messages!
//{
//	if (cache_type == IS_LLC)
//        {
//		std::list<int> requesters;
//
//		//Search MSHR
//		for (uint32_t index = 0; index < MSHR_SIZE; index++)
//		{
//			if (MSHR.entry[index].address == packet->address)
//			{
//				requesters.push_back(index);
//			}
//		}
//
//		if (requesters.size() == 0)
//		{
//			cerr << "Bingo: Cannot find a matching entry in the MSHR of LLC" << endl;
//			assert(0);
//		}
//
//		while (!requesters.empty())
//		{
//			int mshr_index = requesters.front();
//			requesters.pop_front();
//
//			MSHR.num_returned++;
//			MSHR.entry[mshr_index].returned = COMPLETED;
//			MSHR.entry[mshr_index].data = packet->data;
//
//			if (MSHR.entry[mshr_index].event_cycle < current_core_cycle[packet->cpu])
//				MSHR.entry[mshr_index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
//			else
//				MSHR.entry[mshr_index].event_cycle += LATENCY;
//
//			update_fill_cycle();
//		}
//
//		return;
//	}
//
//	if (cache_type == IS_LLC)
//	{
//		cerr << "Bingo: Should not reach here. If the cache is LLC, the function should terminated before here." << endl;
//		assert(0);
//	}
//
//
//    // check MSHR information
//    int mshr_index = check_mshr(packet);
//
//    // sanity check
//    if (mshr_index == -1) {
//        cerr << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << packet->instr_id << " cannot find a matching entry!";
//        cerr << " full_addr: " << hex << packet->full_addr;
//        cerr << " address: " << packet->address << dec;
//        cerr << " event: " << packet->event_cycle << " current: " << current_core_cycle[packet->cpu] << endl;
//        assert(0);
//    }
//
//    // MSHR holds the most updated information about this request
//    // no need to do memcpy
//    MSHR.num_returned++;
//    MSHR.entry[mshr_index].returned = COMPLETED;
//    MSHR.entry[mshr_index].data = packet->data;
//
//    // ADD LATENCY
//    if (MSHR.entry[mshr_index].event_cycle < current_core_cycle[packet->cpu])
//        MSHR.entry[mshr_index].event_cycle = current_core_cycle[packet->cpu] + LATENCY;
//    else
//        MSHR.entry[mshr_index].event_cycle += LATENCY;
//
//    update_fill_cycle();
//
//    DP (if (warmup_complete[packet->cpu]) {
//    cout << "[" << NAME << "_MSHR] " <<  __func__ << " instr_id: " << MSHR.entry[mshr_index].instr_id;
//    cout << " address: " << hex << MSHR.entry[mshr_index].address << " full_addr: " << MSHR.entry[mshr_index].full_addr;
//    cout << " data: " << MSHR.entry[mshr_index].data << dec << " num_returned: " << MSHR.num_returned;
//    cout << " index: " << mshr_index << " occupancy: " << MSHR.occupancy;
//    cout << " event: " << MSHR.entry[mshr_index].event_cycle << " current: " << current_core_cycle[packet->cpu] << " next: " << MSHR.next_fill_cycle << "\tpacket->cpu = " << packet->cpu  << endl; });
//}

void CACHE::update_fill_cycle()
{
    // update next_fill_cycle
    uint64_t min_cycle = UINT64_MAX;
    uint32_t min_index = MSHR.SIZE;
    for (uint32_t i=0; i<MSHR.SIZE; i++) {
        if ((MSHR.entry[i].returned == COMPLETED) && (MSHR.entry[i].event_cycle < min_cycle)) {
            min_cycle = MSHR.entry[i].event_cycle;
            min_index = i;
        }

        DP (if (warmup_complete[MSHR.entry[i].cpu]) {
        cout << "[" << NAME << "_MSHR] " <<  __func__ << " checking instr_id: " << MSHR.entry[i].instr_id;
        cout << " address: " << hex << MSHR.entry[i].address << " full_addr: " << MSHR.entry[i].full_addr;
        cout << " data: " << MSHR.entry[i].data << dec << " returned: " << +MSHR.entry[i].returned << " fill_level: " << MSHR.entry[i].fill_level;
        cout << " index: " << i << " occupancy: " << MSHR.occupancy;
        cout << " event: " << MSHR.entry[i].event_cycle << " current: " << current_core_cycle[MSHR.entry[i].cpu] << " next: " << MSHR.next_fill_cycle << "\tMSHR.entry[i].cpu = " << MSHR.entry[i].cpu  << endl; });
    }
    
    MSHR.next_fill_cycle = min_cycle;
    MSHR.next_fill_index = min_index;
    if (min_index < MSHR.SIZE) {

        DP (if (warmup_complete[MSHR.entry[min_index].cpu]) {
        cout << "[" << NAME << "_MSHR] " <<  __func__ << " instr_id: " << MSHR.entry[min_index].instr_id;
        cout << " address: " << hex << MSHR.entry[min_index].address << " full_addr: " << MSHR.entry[min_index].full_addr;
        cout << " data: " << MSHR.entry[min_index].data << dec << " num_returned: " << MSHR.num_returned;
        cout << " event: " << MSHR.entry[min_index].event_cycle << " current: " << current_core_cycle[MSHR.entry[min_index].cpu] << " next: " << MSHR.next_fill_cycle << "\tMSHR.entry[min_index].cpu = " << MSHR.entry[min_index].cpu  << endl; });
    }
}
//
//int CACHE::check_mshr(PACKET *packet)
//{
//    // search mshr
//    for (uint32_t index=0; index<MSHR_SIZE; index++) {
//        if (MSHR.entry[index].address == packet->address && MSHR.entry[index].cpu == packet->cpu /*Bingo*/) {
//
//            DP ( if (warmup_complete[packet->cpu]) {
//            cout << "[" << NAME << "_MSHR] " << __func__ << " same entry instr_id: " << packet->instr_id << " prior_id: " << MSHR.entry[index].instr_id;
//            cout << " address: " << hex << packet->address;
//            cout << " full_addr: " << packet->full_addr << dec << "\tpacket->cpu = " << packet->cpu  << endl; });
//
//            return index;
//        }
//    }
//
//    DP ( if (warmup_complete[packet->cpu]) {
//    cout << "[" << NAME << "_MSHR] " << __func__ << " new address: " << hex << packet->address;
//    cout << " full_addr: " << packet->full_addr << dec << "\tpacket->cpu = " << packet->cpu << endl; });
//
//    DP ( if (warmup_complete[packet->cpu] && (MSHR.occupancy == MSHR_SIZE)) {
//    cout << "[" << NAME << "_MSHR] " << __func__ << " mshr is full";
//    cout << " instr_id: " << packet->instr_id << " mshr occupancy: " << MSHR.occupancy;
//    cout << " address: " << hex << packet->address;
//    cout << " full_addr: " << packet->full_addr << dec;
//    cout << " cycle: " << current_core_cycle[packet->cpu] << "\tpacket->cpu = " << packet->cpu  << endl; });
//
//    return -1;
//}
//
//void CACHE::add_mshr(PACKET *packet)
//{
//    uint32_t index = 0;
//
//    // search mshr
//    for (index=0; index<MSHR_SIZE; index++) {
//        if (MSHR.entry[index].address == 0) {
//
//            memcpy(&MSHR.entry[index], packet, sizeof(PACKET));
//            MSHR.entry[index].returned = INFLIGHT;
//            MSHR.occupancy++;
//
//            DP ( if (warmup_complete[packet->cpu]) {
//            cout << "[" << NAME << "_MSHR] " << __func__ << " instr_id: " << packet->instr_id;
//            cout << " address: " << hex << packet->address << " full_addr: " << packet->full_addr << dec;
//            cout << " index: " << index << " occupancy: " << MSHR.occupancy << "\tpacket->cpu = " << packet->cpu  << endl; });
//
//            break;
//        }
//    }
//}

uint32_t CACHE::get_occupancy(uint8_t queue_type, uint64_t address)
{
    if (queue_type == 0)
        return MSHR.occupancy;
    else if (queue_type == 1)
        return RQ.occupancy;
    else if (queue_type == 2)
        return WQ.occupancy;
    else if (queue_type == 3)
        return PQ.occupancy;

    return 0;
}

uint32_t CACHE::get_size(uint8_t queue_type, uint64_t address)
{
    if (queue_type == 0)
        return MSHR.SIZE;
    else if (queue_type == 1)
        return RQ.SIZE;
    else if (queue_type == 2)
        return WQ.SIZE;
    else if (queue_type == 3)
        return PQ.SIZE;

    return 0;
}

void CACHE::increment_WQ_FULL(uint64_t address)
{
    WQ.FULL++;
}

void CACHE::llc_prefetcher_initialize(uint32_t cpu) {
    llc_prefetcher_initialize_(cpu); /* delegate */
}

uint64_t CACHE::llc_prefetcher_operate(uint32_t cpu, uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type) {
    if (type != LOAD) assert(0);

    stats.llc_prefetcher_operate(cpu, addr, ip, cache_hit, type);
    //cout<<"stats compelete"<<endl;
    return llc_prefetcher_operate_(cpu, addr, ip, cache_hit, type); /* delegate */
}

void CACHE::llc_prefetcher_cache_fill(uint32_t cpu, uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr) {
    stats.llc_prefetcher_cache_fill(cpu, addr, set, way, prefetch, evicted_addr);
    llc_prefetcher_cache_fill_(cpu, addr, set, way, prefetch, evicted_addr); /* delegate */
}

void CACHE::llc_prefetcher_final_stats(uint32_t cpu) {
    stats.llc_prefetcher_final_stats(cpu);
    llc_prefetcher_final_stats_(cpu); /* delegate */
    cout << endl;
}

void CACHE::llc_prefetcher_inform_warmup_complete() {
    stats.llc_prefetcher_inform_warmup_complete();
    llc_prefetcher_inform_warmup_complete_(); /* delegate */
}

void CACHE::llc_prefetcher_inform_roi_complete(uint32_t cpu) {
    stats.llc_prefetcher_inform_roi_complete(cpu);
    llc_prefetcher_inform_roi_complete_(cpu); /* delegate */
}

void CACHE::llc_prefetcher_roi_stats(uint32_t cpu) {
    stats.llc_prefetcher_roi_stats(cpu);
    llc_prefetcher_roi_stats_(cpu); /* delegate */
    cout << endl;
}
