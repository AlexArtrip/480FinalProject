//
// Created by Max Yu on 5/10/21.
//

#ifndef GPUHASHTABLES_HASH_TABLE_H
#define GPUHASHTABLES_HASH_TABLE_H

#include <stdint.h>
struct KeyValue
{
    uint32_t key;
    uint32_t value;
};

const uint32_t kHashTableCapacity = 128 * 1024 * 1024;
//const uint32_t kHashTableCapacity = 32 * 512 * 1024;

const uint32_t kNumKeyValues = (kHashTableCapacity / 10)* 9;

const uint32_t kEmpty = 0xffffffff;

class HashTable {
public:
    virtual void insert_hashtable(const KeyValue* kvs, uint32_t num_kvs) = 0;
    virtual void lookup_hashtable(KeyValue* kvs, uint32_t num_kvs) = 0;
    virtual void delete_hashtable(const KeyValue* kvs, uint32_t num_kvs) = 0;
    virtual std::vector<KeyValue> iterate_hashtable() = 0;

protected:
    uint32_t hashTableCapacity = 128 * 1024 * 1024;
    uint32_t numKeyValues = hashTableCapacity / 2;
    KeyValue* table;
};

#endif //GPUHASHTABLES_HASH_TABLE_H
