#include "instruction.h"
#include "cache.h"
#include "block.h"
#include "champsim.h"
#include "instruction.h"
#include "memory_class.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include<vector>

using namespace std;

#define MAX_INS 4







int main()
{

//    CACHE cache=new CACHE();
     CACHE LLC{"LLC", LLC_SET, LLC_WAY, LLC_SET *LLC_WAY, LLC_WQ_SIZE, LLC_RQ_SIZE, LLC_PQ_SIZE, LLC_MSHR_SIZE};
     LLC.operate();

    return 0;
}


