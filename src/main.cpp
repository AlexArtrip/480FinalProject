#include "algorithm"
#include "random"
#include "stdint.h"
#include "stdio.h"
#include "unordered_map"
#include "unordered_set"
#include "vector"
#include "chrono"
#include "hash_table.h"
#include "linearprobing.h"
#include "cuckoo.h"
//using namespace LinearProbing;
// Create random keys/values in the range [0, kEmpty)
// kEmpty is used to indicate an empty slot


std::vector<KeyValue> generate_random_keyvalues(std::mt19937& rnd, uint32_t numkvs)
{
    std::uniform_int_distribution<uint32_t> dis(0, kEmpty - 1);

    std::vector<KeyValue> kvs;
    kvs.reserve(numkvs);

    for (uint32_t i = 0; i < numkvs; i++)
    {
        uint32_t rand0 = dis(rnd);
        uint32_t rand1 = dis(rnd);
        kvs.push_back(hash_make_entry(rand0, rand1));
    }

    return kvs;
}

// return numshuffledkvs random items from kvs
std::vector<KeyValue> shuffle_keyvalues(std::mt19937& rnd, std::vector<KeyValue>& kvs, uint32_t numshuffledkvs)
{
    std::shuffle(kvs.begin(), kvs.end(), rnd);

    std::vector<KeyValue> shuffled_kvs;
    shuffled_kvs.resize(numshuffledkvs);

    std::copy(kvs.begin(), kvs.begin() + numshuffledkvs, shuffled_kvs.begin());

    return shuffled_kvs;
}

using Time = std::chrono::time_point<std::chrono::steady_clock>;

Time start_timer() 
{
    return std::chrono::high_resolution_clock::now();
}

double get_elapsed_time(Time start) 
{
    Time end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> d = end - start;
    std::chrono::microseconds us = std::chrono::duration_cast<std::chrono::microseconds>(d);
    return us.count() / 1000.0f;
}

void test_unordered_map(std::vector<KeyValue>& insert_kvs, std::vector<KeyValue>& delete_kvs)
{
    Time timer = start_timer();

    printf("Timing std::unordered_map...\n");

    {
        std::unordered_map<uint32_t, uint32_t> kvs_map;
        for (auto& kv : insert_kvs) 
        {
            kvs_map[hash_get_key(kv)] = hash_get_value(kv);
        }
        for (auto& kv : delete_kvs)
        {
            auto i = kvs_map.find(hash_get_key(kv));
            if (i != kvs_map.end())
                kvs_map.erase(i);
        }
    }

    double milliseconds = get_elapsed_time(timer);
    double seconds = milliseconds / 1000.0f;
    printf("Total time for std::unordered_map: %f ms (%f million keys/second)\n", 
        milliseconds, kNumKeyValues / seconds / 1000000.0f);
}

void test_correctness(std::vector<KeyValue>&, std::vector<KeyValue>&, std::vector<KeyValue>&);


void run_test(HashTable& hashTable, std::vector<KeyValue>& insert_kvs,
              std::vector<KeyValue>& delete_kvs, std::vector<KeyValue>& lookup_kvs) {
    // Begin test
    Time timer = start_timer();

    // Insert items into the hash table
    const uint32_t num_insert_batches = 16;
    //const uint32_t num_insert_batches = 1;
    uint32_t num_inserts_per_batch = (uint32_t)insert_kvs.size() / num_insert_batches;
    for (uint32_t i = 0; i < num_insert_batches; i++)
    {
        hashTable.insert_hashtable(insert_kvs.data() + i * num_inserts_per_batch, num_inserts_per_batch);
    }

    /*
    // Delete items from the hash table
    const uint32_t num_delete_batches = 8;
    uint32_t num_deletes_per_batch = (uint32_t)delete_kvs.size() / num_delete_batches;
    for (uint32_t i = 0; i < num_delete_batches; i++)
    {
        hashTable->delete_hashtable(delete_kvs.data() + i * num_deletes_per_batch, num_deletes_per_batch);
    }
    */
    const uint32_t num_lookup_batches = 8;
    //const uint32_t num_lookup_batches = 1;
    uint32_t num_lookups_per_batch = (uint32_t)lookup_kvs.size() / num_lookup_batches;
    for (uint32_t i = 0; i < num_lookup_batches; i++)
    {
        hashTable.lookup_hashtable(lookup_kvs.data() + i * num_lookups_per_batch, num_lookups_per_batch);
    }

    // Get all the key-values from the hash table
    std::vector<KeyValue> kvs = hashTable.iterate_hashtable();

    // Summarize results
    double milliseconds = get_elapsed_time(timer);
    double seconds = milliseconds / 1000.0f;
    printf("%s Total time (including memory copies, readback, etc): %f ms (%f million keys/second)\n",
           hashTable.name(),
           milliseconds,
           kNumKeyValues / seconds / 1000000.0f);

    //test_unordered_map(insert_kvs, delete_kvs);

    //test_correctness(insert_kvs, delete_kvs, kvs);

    printf("Success\n");
}

int main() 
{
    // To recreate the same random numbers across runs of the program, set seed to a specific
    // number instead of a number from random_device
    std::random_device rd;
    uint32_t seed = rd();
    std::mt19937 rnd(seed);  // mersenne_twister_engine

    printf("Random number generator seed = %u\n", seed);

    for (uint32_t i = 0; i < 5; i++) {
        printf("Initializing keyvalue pairs with random numbers...\n");

        std::vector<KeyValue> insert_kvs = generate_random_keyvalues(rnd, kNumKeyValues);
        std::vector<KeyValue> delete_kvs = shuffle_keyvalues(rnd, insert_kvs, kNumKeyValues / 2);
        std::vector<KeyValue> lookup_kvs = shuffle_keyvalues(rnd, insert_kvs, kNumKeyValues / 2);

        printf("Testing insertion/lookup of %d/%d elements into GPU hash table...\n",
               (uint32_t)insert_kvs.size(), (uint32_t)lookup_kvs.size());
        for (uint j = 1; j < 5; j++) {
            Cuckoo::HashTableC cuc(kHashTableCapacity, j * 10);
            run_test(cuc, insert_kvs, delete_kvs, lookup_kvs);
        }
        LinearProbing::HashTableLP lp;
        run_test(lp, insert_kvs, delete_kvs, lookup_kvs);
    }

    return 0;
}
