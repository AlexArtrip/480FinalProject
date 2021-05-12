#include "stdio.h"
#include "stdint.h"
#include "vector"
#include "linearprobing.h"
#include "cuda_util.h"
namespace LinearProbing {
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
    // 32 bit Murmur3 hash
    __device__ uint hash(uint k, uint capacity)
    {
        k ^= k >> 16;
        k *= 0x85ebca6b;
        k ^= k >> 13;
        k *= 0xc2b2ae35;
        k ^= k >> 16;
        return k & (capacity - 1);
    }

    // Create a hash table. For linear probing, this is just an array of KeyValues
    KeyValue* create_hashtable(uint capacity)
    {
        // Allocate memory
        KeyValue* hashtable;
        cudaMalloc(&hashtable, sizeof(KeyValue) * capacity);

        // Initialize hash table to empty
        static_assert(kEmpty == 0xffffffff, "memset expected kEmpty=0xffffffff");
        cudaMemset(hashtable, 0xff, sizeof(KeyValue) * capacity);

        return hashtable;
    }

    // Insert the key/values in kvs into the hashtable
    __global__ void gpu_hashtable_insert(KeyValue* hashtable, uint capacity, const KeyValue* kvs, unsigned int numkvs)
    {
        unsigned int threadid = blockIdx.x * blockDim.x + threadIdx.x;
        if (threadid < numkvs)
        {
            uint key = get_key(kvs[threadid]);
            uint value = get_value(kvs[threadid]);
            uint slot = hash(key, capacity);

            while (true)
            {
                uint prev = atomicCAS((uint *)&hashtable[slot], kEmpty, key);
                if (prev == kEmpty || prev == key)
                {
                    hashtable[slot] = kvs[threadid];
                    return;
                }

                slot = (slot + 1) & (capacity - 1);
            }
        }
    }

    void insert_hashtable(KeyValue* pHashTable, Logger* logger, uint capacity, const KeyValue* kvs, uint num_kvs)
    {
        // Copy the keyvalues to the GPU
        KeyValue* device_kvs;
        CUDA_SAFE_CALL(cudaMalloc(&device_kvs, sizeof(KeyValue) * num_kvs));
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
        int gridsize = ((uint)num_kvs + threadblocksize - 1) / threadblocksize;
        gpu_hashtable_insert << <gridsize, threadblocksize >> > (pHashTable, capacity, device_kvs, (uint)num_kvs);

        cudaEventRecord(stop);

        cudaEventSynchronize(stop);

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        float seconds = milliseconds / 1000.0f;
        printf("    GPU inserted %d items in %f ms (%f million keys/second)\n",
            num_kvs, milliseconds, num_kvs / (double)seconds / 1000000.0f);
        logger->logInsert(capacity, num_kvs * 1.0 / capacity, num_kvs, milliseconds);

        cudaFree(device_kvs);
    }

    // Lookup keys in the hashtable, and return the values
    __global__ void gpu_hashtable_lookup(KeyValue* hashtable, uint capacity, KeyValue* kvs, unsigned int numkvs)
    {
        unsigned int threadid = blockIdx.x * blockDim.x + threadIdx.x;
        if (threadid < numkvs)
        {
            uint key = get_key(kvs[threadid]);
            uint slot = hash(key, capacity);

            while (true)
            {
                if (get_key(hashtable[slot]) == key)
                {
                    kvs[threadid] = hashtable[slot];
                    return;
                }
                if (get_key(hashtable[slot]) == kEmpty)
                {
                    kvs[threadid] = make_entry(key, kEmpty);
                    return;
                }
                slot = (slot + 1) & (capacity - 1);
            }
        }
    }

    void lookup_hashtable(KeyValue* pHashTable, Logger* logger, uint capacity, KeyValue* kvs, uint num_kvs)
    {
        // Copy the keyvalues to the GPU
        KeyValue* device_kvs;
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
        int gridsize = ((uint)num_kvs + threadblocksize - 1) / threadblocksize;
        gpu_hashtable_lookup << <gridsize, threadblocksize >> > (pHashTable, capacity, device_kvs, (uint)num_kvs);

        cudaEventRecord(stop);

        cudaEventSynchronize(stop);

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        float seconds = milliseconds / 1000.0f;
        printf("    GPU lookup %d items in %f ms (%f million keys/second)\n",
            num_kvs, milliseconds, num_kvs / (double)seconds / 1000000.0f);
        logger->logLookup(num_kvs, milliseconds);

        cudaFree(device_kvs);
    }

    // Delete each key in kvs from the hash table, if the key exists
    // A deleted key is left in the hash table, but its value is set to kEmpty
    // Deleted keys are not reused; once a key is assigned a slot, it never moves
    __global__ void gpu_hashtable_delete(KeyValue* hashtable, uint capacity, const KeyValue* kvs, unsigned int numkvs)
    {
        unsigned int threadid = blockIdx.x * blockDim.x + threadIdx.x;
        if (threadid < numkvs)
        {
            uint key = get_key(kvs[threadid]);
            uint slot = hash(key, capacity);
            while (true)
            {
                if (get_key(hashtable[slot]) == key)
                {
                    hashtable[slot] = make_entry(key, kEmpty);
                    return;
                }
                if (get_key(hashtable[slot]) == kEmpty)
                {
                    return;
                }
                slot = (slot + 1) & (capacity - 1);
            }
        }
    }

    void delete_hashtable(KeyValue* pHashTable, Logger* logger, uint capacity, const KeyValue* kvs, uint num_kvs)
    {
        // Copy the keyvalues to the GPU
        KeyValue* device_kvs;
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
        int gridsize = ((uint)num_kvs + threadblocksize - 1) / threadblocksize;
        gpu_hashtable_delete << <gridsize, threadblocksize >> > (pHashTable, capacity, device_kvs, (uint)num_kvs);

        cudaEventRecord(stop);

        cudaEventSynchronize(stop);

        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        float seconds = milliseconds / 1000.0f;
        printf("    GPU delete %d items in %f ms (%f million keys/second)\n",
            num_kvs, milliseconds, num_kvs / (double)seconds / 1000000.0f);
        logger->logDelete(num_kvs, milliseconds);

        cudaFree(device_kvs);
    }

    // Iterate over every item in the hashtable; return non-empty key/values
    __global__ void gpu_iterate_hashtable(KeyValue* pHashTable, uint capacity, KeyValue* kvs, uint* kvs_size)
    {
        unsigned int threadid = blockIdx.x * blockDim.x + threadIdx.x;
        if (threadid < capacity)
        {
            if (get_key(pHashTable[threadid]) != kEmpty)
            {
                uint value = get_value(pHashTable[threadid]);
                if (value != kEmpty)
                {
                    uint size = atomicAdd(kvs_size, 1);
                    kvs[size] = pHashTable[threadid];
                }
            }
        }
    }

    std::vector<KeyValue> iterate_hashtable(KeyValue* pHashTable, uint capacity)
    {
        uint* device_num_kvs;
        cudaMalloc(&device_num_kvs, sizeof(uint));
        cudaMemset(device_num_kvs, 0, sizeof(uint));

        KeyValue* device_kvs;
        cudaMalloc(&device_kvs, sizeof(KeyValue) * kNumKeyValues);

        int mingridsize;
        int threadblocksize;
        cudaOccupancyMaxPotentialBlockSize(&mingridsize, &threadblocksize, gpu_iterate_hashtable, 0, 0);

        int gridsize = (kHashTableCapacity + threadblocksize - 1) / threadblocksize;
        gpu_iterate_hashtable << <gridsize, threadblocksize >> > (pHashTable, capacity, device_kvs, device_num_kvs);

        uint num_kvs;
        cudaMemcpy(&num_kvs, device_num_kvs, sizeof(uint), cudaMemcpyDeviceToHost);

        std::vector<KeyValue> kvs;
        kvs.resize(num_kvs);

        cudaMemcpy(kvs.data(), device_kvs, sizeof(KeyValue) * num_kvs, cudaMemcpyDeviceToHost);

        cudaFree(device_kvs);
        cudaFree(device_num_kvs);

        return kvs;
    }

    // Free the memory of the hashtable
    void destroy_hashtable(KeyValue* pHashTable)
    {
        cudaFree(pHashTable);
    }
}