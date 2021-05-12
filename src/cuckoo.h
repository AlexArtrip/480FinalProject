#pragma once
#include "vector"
#include "hash_table.h"

namespace Cuckoo {
    const unsigned kStashSize = 101;           //!< How many slots the stash hash table contains.
//    unsigned stash_hash_function(const uint x, const uint y,
//        const unsigned key) {   //TODO:: might need to change type sig
//        return (x ^ key + y) % kStashSize;
//    }

    unsigned ComputeMaxIterations(const unsigned n,
                                  const unsigned table_size,
                                  const unsigned num_functions) {
        float lg_input_size = (float)(log((double)n) / log(2.0));

// #define CONSTANT_ITERATIONS
//#ifdef CONSTANT_ITERATIONS
//        // Set the maximum number of iterations to 7lg(N).
//    const unsigned MAX_ITERATION_CONSTANT = 7;
//    unsigned max_iterations = MAX_ITERATION_CONSTANT * lg_input_size;
//#else
        // Use an empirical formula for determining what the maximum number of
        // iterations should be.  Works OK in most situations.
        float load_factor = float(n) / table_size;
        float ln_load_factor = (float)(log(load_factor) / log(2.71828183));

        unsigned max_iterations = (unsigned)(4.0 * ceil(-1.0 / (0.028255 + 1.1594772 *
                                                                           ln_load_factor)* lg_input_size));
//#endif
        return max_iterations;
    }

    KeyValue* create_hashtable(uint size, uint** stash_count);

    void insert_hashtable(KeyValue* hashtable, uint size, uint max_iteration_attempts, const KeyValue* kvs,
                          uint num_kvs, uint* stash_count);

    void lookup_hashtable(KeyValue* hashtable, uint size, KeyValue* kvs, uint num_kvs, uint* stash_count);

    void delete_hashtable(KeyValue* hashtable, uint size, const KeyValue* kvs, uint num_kvs, uint* stash_count);

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
            max_iterations = ComputeMaxIterations(kNumKeyValues, kHashTableCapacity, 2);
            table = create_hashtable(hashTableCapacity, &d_stash_count);
        }
        HashTableC(uint size, uint expected_kvs) {
            hashTableCapacity = size;
            numKeyValues = size / 2;
            max_iterations = ComputeMaxIterations(expected_kvs, size, 2);
            table = create_hashtable(hashTableCapacity, &d_stash_count);
        }
        ~HashTableC() {
            if (table) {
                Cuckoo::destroy_hashtable(table);
            }
            // cudaFree(d_stash_count);
        }
        virtual void insert_hashtable(const KeyValue* kvs, uint num_kvs) {
            Cuckoo::insert_hashtable(table, hashTableCapacity, max_iterations, kvs, num_kvs, d_stash_count);
        }
        virtual void lookup_hashtable(KeyValue* kvs, uint num_kvs) {
            Cuckoo::lookup_hashtable(table, hashTableCapacity, kvs, num_kvs, d_stash_count);
        }
        virtual void delete_hashtable(const KeyValue* kvs, uint num_kvs) {
            Cuckoo::delete_hashtable(table, hashTableCapacity, kvs, num_kvs, d_stash_count);
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