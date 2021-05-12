//
// Created by Max Yu on 5/10/21.
//

#ifndef GPUHASHTABLES_HASH_TABLE_H
#define GPUHASHTABLES_HASH_TABLE_H

#include <stdint.h>
#include <string>

typedef uint32_t uint;

typedef uint64_t KeyValue;

//! Makes an 64-bit Entry out of a key-value pair for the hash table.
inline KeyValue hash_make_entry(unsigned key, unsigned value) {
    return (KeyValue(key) << 32) + value;
}

//! Returns the key of an Entry.
inline unsigned hash_get_key(KeyValue entry) {
    return (unsigned)(entry >> 32);
}

//! Returns the value of an Entry.
inline unsigned hash_get_value(KeyValue entry) {
    return (unsigned)(entry & 0xffffffff);
}

/*
struct KeyValue
{
    uint32_t key;
    uint32_t value;
};
*/


const uint32_t kHashTableCapacity = 128 * 1024 * 1024;
//const uint32_t kHashTableCapacity = 32 * 512 * 1024;

const uint32_t kNumKeyValues = (kHashTableCapacity / 10)* 9;

const uint32_t kEmpty = 0xffffffff;

//const KeyValue kvEmpty = make_entry(kEmpty, kEmpty);
const KeyValue kvEmpty = 0xffffffffffffffff;

class HashTable {
public:
    virtual void insert_hashtable(const KeyValue* kvs, uint32_t num_kvs) = 0;
    virtual void lookup_hashtable(KeyValue* kvs, uint32_t num_kvs) = 0;
    virtual void delete_hashtable(const KeyValue* kvs, uint32_t num_kvs) = 0;
    virtual std::vector<KeyValue> iterate_hashtable() = 0;
    virtual std::string name() = 0;

protected:
    uint32_t hashTableCapacity = 128 * 1024 * 1024;
    uint32_t numKeyValues = hashTableCapacity / 2;
    KeyValue* table;
};


#endif //GPUHASHTABLES_HASH_TABLE_H
