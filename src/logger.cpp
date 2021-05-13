//
// Created by Max Yu on 5/12/21.
//

#include "logger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <assert.h>
#include <thread>

Logger::Logger(HashTableType hashTableType, bool deleteFlag) {
    type = hashTableType;
    delFlag = deleteFlag;

    std::stringstream filename;
    std::time_t current = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::cout << "yeet\n";

    std::tm * local_time = std::localtime(&current);
    
    filename << ht_filenames[hashTableType] << "_";
    if (deleteFlag) {
        filename << "d_";
    }
    filename << 1 + local_time->tm_mon << "-" << local_time->tm_mday << "-";
    filename << 1 + local_time->tm_hour << "-" << 1 + local_time->tm_min << "-";
    filename << 1 + local_time->tm_sec << ".txt";
    std::cout << filename.str() << std::endl;

    file = new std::ofstream(filename.str());
    if (!file->is_open())
    {
        printf("CANNOT OPEN LOGGER FILE");

        std::chrono::seconds dura(5);
        std::this_thread::sleep_for(dura);
        exit(-1);
    }
    *file << "Capacity\tLoadFactor\tInsertNumKVs\tInsertTime\t";
    switch (hashTableType) {
        case LINEAR_PROBING:
            break;
        default:
            *file << "InsertIter\tStashCount\tFailCount\t";
            break;
    }
    *file << "LookupNumKVs\tLookupTime";
    if (deleteFlag) {
        *file << "\tDeleteNumKVs\tDeleteTime";
    }
    *file << std::endl;
}

void Logger::logInsert(uint capacity, float load, uint num_kvs, float milliseconds) {
    *file << capacity << "\t" << load << "\t" << num_kvs << "\t" << milliseconds << "\t";
}

void Logger::logInsert(uint capacity, float load, uint num_kvs, float milliseconds, uint iter, uint stash_count, uint fail_count) {
    assert(type != LINEAR_PROBING);
    logInsert(capacity, load, num_kvs, milliseconds);
    *file << iter << "\t" << stash_count << "\t" << fail_count << "\t";
}

void Logger::logLookup(uint num_kvs, float milliseconds) {
    *file << num_kvs << "\t" << milliseconds;
    if (delFlag) {
        *file << "\t";
    } else {
        *file << "\n";
    }
}

void Logger::logDelete(uint num_kvs, float milliseconds) {
    *file << num_kvs << "\t" << milliseconds << "\n";
}

