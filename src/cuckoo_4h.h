#pragma once
#include "vector"
#include "ht_utils.h"
#include "logger.h"
#include "hash_table.h"


namespace Cuckoo4h {
    const unsigned kStashSize = 101;           //!< How many slots the stash hash table contains.
//    unsigned stash_hash_function(const uint x, const uint y,
//        const unsigned key) {   //TODO:: might need to change type sig
//        return (x ^ key + y) % kStashSize;
//    }

    unsigned ComputeMaxIterations(const unsigned n,
        const unsigned table_size,
        const unsigned num_functions);

    KeyValue* create_hashtable(uint size, uint** stash_count);

    void insert_hashtable(KeyValue* hashtable, Logger* logger,
                          uint size, uint max_iteration_attempts, const KeyValue* kvs,
                          uint num_kvs, uint* stash_count);

    void lookup_hashtable(KeyValue* hashtable, Logger* logger,
                          uint size, KeyValue* kvs, uint num_kvs, uint* stash_count);

    void delete_hashtable(KeyValue* hashtable, Logger* logger,
                          uint size, const KeyValue* kvs, uint num_kvs, uint* stash_count);

    std::vector<KeyValue> iterate_hashtable(KeyValue* hashtable, uint size);

    void destroy_hashtable(KeyValue* hashtable);

    class HashTableC : public HashTable {
    protected:
        uint max_iterations = 10;
        uint* d_stash_count = NULL;
    public:
        HashTableC() {
            hashTableCapacity = kHashTableCapacity;
            numKeyValues = hashTableCapacity / 2;
            max_iterations = Cuckoo::ComputeMaxIterations(kNumKeyValues, kHashTableCapacity, 4);
            table = create_hashtable(hashTableCapacity, &d_stash_count);
        }
        HashTableC(uint size, uint expected_kvs) {
            hashTableCapacity = size;
            numKeyValues = size / 2;
            max_iterations = Cuckoo::ComputeMaxIterations(expected_kvs, size, 4);
            table = create_hashtable(hashTableCapacity, &d_stash_count);
        }
        ~HashTableC() {
            if (table) {
                Cuckoo::destroy_hashtable(table);
            }
            // cudaFree(d_stash_count);
        }
        virtual void insert_hashtable(const KeyValue* kvs, uint num_kvs) {
            Cuckoo::insert_hashtable(table, logger, hashTableCapacity, max_iterations, kvs, num_kvs, d_stash_count);
        }
        virtual void lookup_hashtable(KeyValue* kvs, uint num_kvs) {
            Cuckoo::lookup_hashtable(table, logger, hashTableCapacity, kvs, num_kvs, d_stash_count);
        }
        virtual void delete_hashtable(const KeyValue* kvs, uint num_kvs) {
            Cuckoo::delete_hashtable(table, logger, hashTableCapacity, kvs, num_kvs, d_stash_count);
        }
        virtual std::vector<KeyValue> iterate_hashtable() {
            return Cuckoo::iterate_hashtable(table, hashTableCapacity);
        }
        virtual void destroy_hashtable() {
            Cuckoo::destroy_hashtable(table);
        }     
        const char* name() {
			return "Standard Cuckoo ";
        }
    };
}