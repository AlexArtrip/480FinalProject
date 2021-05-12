//
// Created by Max Yu on 5/10/21.
//

#ifndef GPUHASHTABLES_HT_UTILS_H
#define GPUHASHTABLES_HT_UTILS_H

#include <stdint.h>
#include <string>

typedef uint32_t uint;

typedef unsigned long long int KeyValue;

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


const uint32_t kHashTableCapacity = 128 * 1024 * 1024;
//const uint32_t kHashTableCapacity = 32 * 512 * 1024;

const uint32_t kNumKeyValues = (kHashTableCapacity / 10)* 5;

const uint32_t kEmpty = 0xffffffff;

//const KeyValue kvEmpty = make_entry(kEmpty, kEmpty);
const KeyValue kvEmpty = 0xffffffffffffffff;


enum HashTableType {
    LINEAR_PROBING = 0,
    CUCKOO = 1,
    CUCKOO_1H1P = 2,
    CUCKOO_1H3P = 3
};

const std::string ht_filenames[] = { "LP", "C", "C1h1p", "C1h3p" };


#endif //GPUHASHTABLES_HASH_TABLE_H
