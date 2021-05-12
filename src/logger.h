//
// Created by Max Yu on 5/12/21.
//

#ifndef GPUHASHTABLES_LOGGER_H
#define GPUHASHTABLES_LOGGER_H
#include "ht_utils.h"
#include <fstream>

//! THIS CLASS ASSUMES PROPER USAGE
class Logger {
protected:
    std::ofstream* file;
    HashTableType type;
    bool delFlag;
public:
    Logger() {
        exit(-1); // DO NOT CALL THIS CONSTRUCTOR!!!
    }
    Logger(HashTableType hashTableType, bool deleteFlag);
    ~Logger() {
        file->close();
        delete file;
    }
    void logInsert(uint capacity, float load, uint num_kvs, float milliseconds);
    void logInsert(uint capacity, float load, uint num_kvs, float milliseconds, uint iter, uint stash_count, uint fail_count);
    void logLookup(uint num_kvs, float milliseconds);
    void logDelete(uint num_kvs, float milliseconds);
};

#endif //GPUHASHTABLES_LOGGER_H
