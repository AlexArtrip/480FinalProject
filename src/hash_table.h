//
// Created by Max Yu on 4/19/21.
//

#ifndef SIMPLECONCURRENTGPUHASHTABLE_HASH_TABLE_H
#define SIMPLECONCURRENTGPUHASHTABLE_HASH_TABLE_H

struct KeyValue
{
    uint32_t key;
    uint32_t value;
};

const uint32_t kHashTableCapacity = 128 * 1024 * 1024;

const uint32_t kNumKeyValues = kHashTableCapacity / 2;

const uint32_t kEmpty = 0xffffffff;

#endif //SIMPLECONCURRENTGPUHASHTABLE_HASH_TABLE_H
