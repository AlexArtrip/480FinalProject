#pragma once
#include "vector"
#include "hash_table.h"

namespace Cuckoo {
    const unsigned kStashSize = 101;           //!< How many slots the stash hash table contains.
//    unsigned stash_hash_function(const uint x, const uint y,
//        const unsigned key) {   //TODO:: might need to change type sig
//        return (x ^ key + y) % kStashSize;
//    }

    KeyValue* create_hashtable(uint size, uint* stash_count);

    void insert_hashtable(KeyValue* hashtable, uint size, uint max_iteration_attempts, const KeyValue* kvs,
                          uint num_kvs, uint* stash_count);

    void lookup_hashtable(KeyValue* hashtable, uint size, KeyValue* kvs, uint num_kvs, uint* stash_count);

    void delete_hashtable(KeyValue* hashtable, uint size, const KeyValue* kvs, uint num_kvs, uint* stash_count);

    std::vector<KeyValue> iterate_hashtable(KeyValue* hashtable, uint size);

    void destroy_hashtable(KeyValue* hashtable);

    class HashTableC : public HashTable {
    protected :
        uint max_iterations = 10;
        uint stash_count = 0;
    public:
        HashTableC() {
            hashTableCapacity = 128 * 1024 * 1024;
            numKeyValues = hashTableCapacity / 2;
            max_iterations = 10;
            table = create_hashtable(hashTableCapacity, &stash_count);
        }
        HashTableC(uint size, uint max_iter) {
            hashTableCapacity = size;
            numKeyValues = size / 2;
            max_iterations = max_iter;
            table = create_hashtable(hashTableCapacity, &stash_count);
        }
        ~HashTableC() {
            destroy_hashtable(table);
        }
        virtual void insert_hashtable(const KeyValue* kvs, uint num_kvs) {
            Cuckoo::insert_hashtable(table, hashTableCapacity, max_iterations, kvs, num_kvs, &stash_count);
        }
        virtual void lookup_hashtable(KeyValue* kvs, uint num_kvs) {
            Cuckoo::lookup_hashtable(table, hashTableCapacity, kvs, num_kvs, &stash_count);
        }
        virtual void delete_hashtable(const KeyValue* kvs, uint num_kvs) {
            Cuckoo::delete_hashtable(table, hashTableCapacity, kvs, num_kvs, &stash_count);
        }
        virtual std::vector<KeyValue> iterate_hashtable() {
            return Cuckoo::iterate_hashtable(table, hashTableCapacity);
        }
        virtual const std::string name() {
            return "Standard Cuckoo ";
        }
    };
}