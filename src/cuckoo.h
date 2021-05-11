#pragma once
#include "vector"
#include "hash_table.h"

namespace Cuckoo {
    typedef unsigned int uint;
    const unsigned kStashSize = 101;           //!< How many slots the stash hash table contains.
    unsigned stash_hash_function(const std::uint32_t x, const std::uint32_t y,
        const unsigned key) {   //TODO:: might need to change type sig
        return (x ^ key + y) % kStashSize;
    }
    KeyValue* create_hashtable(std::uint32_t size);

    void insert_hashtable(KeyValue* hashtable, std::uint32_t size, const KeyValue* kvs, std::uint32_t num_kvs, std::uint32_t max_iterations);

    void lookup_hashtable(KeyValue* hashtable, std::uint32_t size, KeyValue* kvs, std::uint32_t num_kvs);

    void delete_hashtable(KeyValue* hashtable, std::uint32_t size, const KeyValue* kvs, std::uint32_t num_kvs);

    std::vector<KeyValue> iterate_hashtable(KeyValue* hashtable, std::uint32_t size);

    void destroy_hashtable(KeyValue* hashtable);

    class HashTableC : public HashTable {
    protected :
        std::uint32_t max_iterations = 10;
    public:
        HashTableC() {
            hashTableCapacity = 128 * 1024 * 1024;
            numKeyValues = hashTableCapacity / 2;
            max_iterations = 10;
            table = create_hashtable(hashTableCapacity);
        }
        HashTableC(std::uint32_t size, std::uint32_t max_iter) {
            hashTableCapacity = size;
            numKeyValues = size / 2;
            table = create_hashtable(hashTableCapacity);
            max_iterations = max_iter;
        }
        ~HashTableC() {
            destroy_hashtable(table);
        }
        virtual void insert_hashtable(const KeyValue* kvs, std::uint32_t num_kvs) {
            Cuckoo::insert_hashtable(table, hashTableCapacity, kvs, num_kvs, max_iterations);
        }
        virtual void lookup_hashtable(KeyValue* kvs, std::uint32_t num_kvs) {
            Cuckoo::lookup_hashtable(table, hashTableCapacity, kvs, num_kvs);
        }
        virtual void delete_hashtable(const KeyValue* kvs, std::uint32_t num_kvs) {
            Cuckoo::delete_hashtable(table, hashTableCapacity, kvs, num_kvs);
        }
        virtual std::vector<KeyValue> iterate_hashtable() {
            return Cuckoo::iterate_hashtable(table, hashTableCapacity);
        }
    };
}