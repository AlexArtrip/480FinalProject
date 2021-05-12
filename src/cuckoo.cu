#include "stdio.h"
#include "stdint.h"
#include "vector"
#include "cuckoo.h"
#include "cuda_util.h"
#include <chrono>
#include <thread>

namespace Cuckoo {
    //! Makes an 64-bit Entry out of a key-value pair for the hash table.
    inline __device__ __host__ KeyValue make_entry(unsigned key, unsigned value) {
        return (KeyValue(key) << 32) + value;
    }

    //! Returns the key of an Entry.
    inline __device__ __host__ unsigned get_key(KeyValue entry) {
        return (unsigned)(entry >> 32);
    }

    //! Returns the value of an Entry.
    inline __device__ __host__ unsigned get_value(KeyValue entry) {
        return (unsigned)(entry & 0xffffffff);
    }


    inline __device__ __host__ unsigned stash_hash_function(const unsigned key) {   //TODO:: might need to change type sig
        return (2720648079 ^ key + 13) % kStashSize;
    }
    // 32 bit Murmur3 hash
    __device__ uint hash(int hash_id, uint k, uint capacity) {
        k ^= k >> 16;
        if (hash_id == 0) {
            k *= 0x85ebca6b;
            k ^= k >> 13;
            k *= 0xc2b2ae35;
        } else {
            k *= 0xcc9e2d51;
            k ^= k >> 13;
            k *= 0x1b873593;
        }
        k ^= k >> 16;
        return k & (capacity - 1);

    }

    //! Determine where to insert the key next.  The hash functions are used in round-robin order.
    __device__ unsigned determine_next_location(const unsigned table_size,
                                                const unsigned key,
                                                const unsigned previous_location) {
        uint next_location = hash(0, key, table_size);
        if (next_location == previous_location) {
            return hash(1, key, table_size);
        }
        return next_location;
    }

    // Create a hash table. For linear probing, this is just an array of KeyValues
    KeyValue *create_hashtable(uint capacity, uint** stash_count) {
        // Allocate memory
        KeyValue *hashtable;
        cudaMalloc(&hashtable, sizeof(KeyValue) * (capacity + kStashSize));

        // Initialize hash table to empty
        static_assert(kEmpty == 0xffffffff, "memset expected kEmpty=0xffffffff");
        cudaMemset(hashtable, 0xff, sizeof(KeyValue) * (capacity + kStashSize));

        CUDA_SAFE_CALL(cudaMalloc((void**)stash_count, sizeof(uint)));
        CUDA_SAFE_CALL(cudaMemset(*stash_count, 0, sizeof(uint)));

        //printf("Hash table created successfully");
        //std::chrono::seconds dura(5);
        //std::this_thread::sleep_for(dura);
        return hashtable;
    }

    // Insert the key/values in kvs into the hashtable
    __global__ void gpu_hashtable_insert(KeyValue *hashtable, uint capacity, uint max_iteration_attempts,
                                         const KeyValue *kvs, unsigned int numkvs,
                                         uint *stash_count, uint *fail_count) {
        unsigned int threadid = blockIdx.x * blockDim.x + threadIdx.x;
        if (threadid < numkvs) {
            KeyValue entry = kvs[threadid];
            unsigned key = get_key(entry);
            unsigned prev_key = key;
            // The key is always inserted into its first slot at the start.
            uint location = hash(0, key, capacity);

            // Keep inserting until an empty slot is found or the eviction chain grows too large.
            for (unsigned its = 1; its <= max_iteration_attempts; its++) {
                // Insert the new entry.
                prev_key = key;
                entry = atomicExch(&hashtable[location], entry);
                key = get_key(entry);
                // If no key was evicted or this key is already present, we're done.
                if (key == kEmpty || prev_key == key) {
                    // *iterations_used = its;
                    return;
                }
                // Otherwise, determine where the evicted key will go.
                location = determine_next_location(capacity, key, location);
            }

            if (key != kEmpty) {
                //printf("failed insert will stash now after max_iter = %u \n", max_iteration_attempts);
                // Shove it into the stash.
                uint slot = stash_hash_function(key);
                KeyValue *stash = hashtable + capacity;
                KeyValue replaced_entry = atomicCAS((stash + slot), kvEmpty, entry);
                if (replaced_entry != kvEmpty) {
                    atomicAdd(fail_count, 1);
                } else {
                    atomicAdd(stash_count, 1);
                }
            }
        }
    }

    void insert_hashtable(KeyValue *pHashTable, uint capacity, uint max_iteration_attempts, const KeyValue *kvs,
                          uint num_kvs, uint *d_stash_count) {
        // Copy the keyvalues to the GPU
        KeyValue *device_kvs;
        cudaMalloc(&device_kvs, sizeof(KeyValue) * num_kvs);
        cudaMemcpy(device_kvs, kvs, sizeof(KeyValue) * num_kvs, cudaMemcpyHostToDevice);

        // Have CUDA calculate the thread block size
        int mingridsize;
        int threadblocksize;
        cudaOccupancyMaxPotentialBlockSize(&mingridsize, &threadblocksize, gpu_hashtable_insert, 0, 0);

        // Create events for GPU timing
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        cudaEventRecord(start);

//        unsigned* d_stash_count = NULL;
//        CUDA_SAFE_CALL(cudaMalloc((void**)&d_stash_count, sizeof(uint)));
//        CUDA_SAFE_CALL(cudaMemset(d_stash_count, 0, sizeof(uint)));
        unsigned* d_fail_count = NULL;
        CUDA_SAFE_CALL(cudaMalloc((void**)&d_fail_count, sizeof(uint)));
        CUDA_SAFE_CALL(cudaMemset(d_fail_count, 0, sizeof(uint)));

        // Insert all the keys into the hash table
        int gridsize = ((uint) num_kvs + threadblocksize - 1) / threadblocksize;
        gpu_hashtable_insert <<<gridsize, threadblocksize>>>(pHashTable, capacity, max_iteration_attempts,
                                                             device_kvs, (uint) num_kvs,
                                                             d_stash_count, d_fail_count);

        cudaEventRecord(stop);

        cudaEventSynchronize(stop);

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        float seconds = milliseconds / 1000.0f;

        printf("    GPU inserted %d items in %f ms (%f million keys/second) \n",
               num_kvs, milliseconds, num_kvs / (double) seconds / 1000000.0f);

        // Copy out the stash size.
        uint stash_count;
        CUDA_SAFE_CALL(cudaMemcpy(&stash_count, d_stash_count, sizeof(unsigned), cudaMemcpyDeviceToHost));
        if (stash_count != 0) {
            printf("        stash count is %u\n", stash_count);
        }
        // Copy out the stash size.
        uint fail_count;
        CUDA_SAFE_CALL(cudaMemcpy(&fail_count, d_fail_count, sizeof(unsigned), cudaMemcpyDeviceToHost));
        if (fail_count != 0) {
            printf("        fail count is %u\n", fail_count);
        }
        cudaFree(d_fail_count);

        cudaFree(device_kvs);
    }

    // Lookup keys in the hashtable, and return the values
    __global__ void gpu_hashtable_lookup(KeyValue *hashtable, uint capacity, KeyValue *kvs,
                                         unsigned int numkvs, uint* stash_count) {
        unsigned int threadid = blockIdx.x * blockDim.x + threadIdx.x;
        if (threadid < numkvs) {
            uint key = get_key(kvs[threadid]);
            KeyValue slot0 = hashtable[hash(0, key, capacity)];
            if (get_key(slot0) == key) {
                kvs[threadid] = slot0;
                return;
            }
            KeyValue slot1 = hashtable[hash(1, key, capacity)];
            if (get_key(slot1) == key) {
                kvs[threadid] = slot1;
                return;
            }
            if (*stash_count) {
                uint slot = stash_hash_function(key);
                KeyValue *stash = hashtable + capacity;
                KeyValue entry = stash[slot];
                if (get_key(entry) == key) {
                    kvs[threadid] = entry;
                    return;
                }
            }
            kvs[threadid] = make_entry(key, kEmpty);
        }
    }

    void lookup_hashtable(KeyValue *pHashTable, uint capacity, KeyValue *kvs, uint num_kvs, uint* stash_count) {
        // Copy the keyvalues to the GPU
        KeyValue *device_kvs;
        cudaMalloc(&device_kvs, sizeof(KeyValue) * num_kvs);
        cudaMemcpy(device_kvs, kvs, sizeof(KeyValue) * num_kvs, cudaMemcpyHostToDevice);

        // Have CUDA calculate the thread block size
        int mingridsize;
        int threadblocksize;
        cudaOccupancyMaxPotentialBlockSize(&mingridsize, &threadblocksize, gpu_hashtable_insert, 0, 0);

        // Create events for GPU timing
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        cudaEventRecord(start);

        // Insert all the keys into the hash table
        int gridsize = ((uint) num_kvs + threadblocksize - 1) / threadblocksize;
        gpu_hashtable_lookup <<< gridsize, threadblocksize >>> (pHashTable, capacity, device_kvs, (uint) num_kvs,
                                                                stash_count);

        cudaEventRecord(stop);

        cudaEventSynchronize(stop);

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        float seconds = milliseconds / 1000.0f;
        printf("    GPU lookup %d items in %f ms (%f million keys/second)\n",
               num_kvs, milliseconds, num_kvs / (double) seconds / 1000000.0f);

        cudaFree(device_kvs);
    }

    // Delete each key in kvs from the hash table, if the key exists
    // A deleted key is left in the hash table, but its value is set to kEmpty
    // Deleted keys are not reused; once a key is assigned a slot, it never moves
    __global__ void gpu_hashtable_delete(KeyValue *hashtable, uint capacity, const KeyValue *kvs,
                                         unsigned int numkvs, uint* stash_count) {
        unsigned int threadid = blockIdx.x * blockDim.x + threadIdx.x;
        if (threadid < numkvs) {
            uint key = get_key(kvs[threadid]);
            // TODO fix!!!
            KeyValue slot0 = hashtable[hash(0, key, capacity)];
            if (get_key(slot0) == key) {
                hashtable[threadid] = kvEmpty;
                return;
            }
            KeyValue slot1 = hashtable[hash(1, key, capacity)];
            if (get_key(slot1) == key) {
                hashtable[threadid] = kvEmpty;
                return;
            }
            if (*stash_count) {
                uint slot = stash_hash_function(key);
                KeyValue *stash = hashtable + capacity;
                KeyValue entry = stash[slot];
                if (get_key(entry) == key) {
                    stash[slot] = kvEmpty;
                    return;
                }
            }
        }
    }

    void delete_hashtable(KeyValue *pHashTable, uint capacity, const KeyValue *kvs, uint num_kvs, uint* stash_count) {
        // Copy the keyvalues to the GPU
        KeyValue *device_kvs;
        cudaMalloc(&device_kvs, sizeof(KeyValue) * num_kvs);
        cudaMemcpy(device_kvs, kvs, sizeof(KeyValue) * num_kvs, cudaMemcpyHostToDevice);

        // Have CUDA calculate the thread block size
        int mingridsize;
        int threadblocksize;
        cudaOccupancyMaxPotentialBlockSize(&mingridsize, &threadblocksize, gpu_hashtable_insert, 0, 0);

        // Create events for GPU timing
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        cudaEventRecord(start);

        // Insert all the keys into the hash table
        int gridsize = ((uint) num_kvs + threadblocksize - 1) / threadblocksize;
        gpu_hashtable_delete <<< gridsize, threadblocksize >>> (pHashTable, capacity, device_kvs, (uint) num_kvs,
                                                                stash_count);

        cudaEventRecord(stop);

        cudaEventSynchronize(stop);

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        float seconds = milliseconds / 1000.0f;
        printf("    GPU delete %d items in %f ms (%f million keys/second)\n",
               num_kvs, milliseconds, num_kvs / (double) seconds / 1000000.0f);

        cudaFree(device_kvs);
    }

    // Iterate over every item in the hashtable; return non-empty key/values
    __global__ void gpu_iterate_hashtable(KeyValue *pHashTable, uint capacity, KeyValue *kvs, uint *kvs_size) {
        unsigned int threadid = blockIdx.x * blockDim.x + threadIdx.x;
        if (threadid < capacity) {
            if (get_key(pHashTable[threadid]) != kEmpty) {
                uint value = get_value(pHashTable[threadid]);
                if (value != kEmpty) {
                    uint size = atomicAdd(kvs_size, 1);
                    kvs[size] = pHashTable[threadid];
                }
            }
        }
    }

    std::vector <KeyValue> iterate_hashtable(KeyValue *pHashTable, uint capacity) {
        uint *device_num_kvs;
        cudaMalloc(&device_num_kvs, sizeof(uint));
        cudaMemset(device_num_kvs, 0, sizeof(uint));

        KeyValue *device_kvs;
        cudaMalloc(&device_kvs, sizeof(KeyValue) * kNumKeyValues);

        int mingridsize;
        int threadblocksize;
        cudaOccupancyMaxPotentialBlockSize(&mingridsize, &threadblocksize, gpu_iterate_hashtable, 0, 0);

        int gridsize = (kHashTableCapacity + threadblocksize - 1) / threadblocksize;
        gpu_iterate_hashtable <<< gridsize, threadblocksize >>> (pHashTable, capacity, device_kvs, device_num_kvs);

        uint num_kvs;
        cudaMemcpy(&num_kvs, device_num_kvs, sizeof(uint), cudaMemcpyDeviceToHost);

        std::vector <KeyValue> kvs;
        kvs.resize(num_kvs);

        cudaMemcpy(kvs.data(), device_kvs, sizeof(KeyValue) * num_kvs, cudaMemcpyDeviceToHost);

        cudaFree(device_kvs);
        cudaFree(device_num_kvs);

        return kvs;
    }

    // Free the memory of the hashtable
    void destroy_hashtable(KeyValue *pHashTable) {
        cudaFree(pHashTable);
    }
}