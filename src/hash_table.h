//
// Created by Max Yu on 5/12/21.
//

#ifndef GPUHASHTABLES_HASH_TABLE_H
#define GPUHASHTABLES_HASH_TABLE_H

#include "logger.h"

class HashTable {
public:
    virtual void insert_hashtable(const KeyValue* kvs, uint32_t num_kvs) = 0;
    virtual void lookup_hashtable(KeyValue* kvs, uint32_t num_kvs) = 0;
    virtual void delete_hashtable(const KeyValue* kvs, uint32_t num_kvs) = 0;
    virtual std::vector<KeyValue> iterate_hashtable() = 0;
    virtual void destroy_hashtable() = 0;
    virtual const char * name() {
        return "default ";
    }
    void setLogger(Logger* l) {
        logger = l;
    }

protected:
    uint32_t hashTableCapacity = 128 * 1024 * 1024;
    uint32_t numKeyValues = hashTableCapacity / 2;
    KeyValue* table;
    Logger* logger;
};

#endif //GPUHASHTABLES_HASH_TABLE_H
