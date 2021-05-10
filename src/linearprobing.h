#pragma once
#include "vector"
#include "hash_table.h"

namespace LinearProbing {
    KeyValue* create_hashtable(uint32_t size);

    void insert_hashtable(KeyValue* hashtable, uint32_t size, const KeyValue* kvs, uint32_t num_kvs);

    void lookup_hashtable(KeyValue* hashtable, uint32_t size, KeyValue* kvs, uint32_t num_kvs);

    void delete_hashtable(KeyValue* hashtable, uint32_t size, const KeyValue* kvs, uint32_t num_kvs);

    std::vector<KeyValue> iterate_hashtable(KeyValue* hashtable, uint32_t size);

    void destroy_hashtable(KeyValue* hashtable);

    class HashTableLP : public HashTable {
    public:
        HashTableLP() {
            hashTableCapacity = 128 * 1024 * 1024;
            numKeyValues = hashTableCapacity / 2;
            table = create_hashtable(hashTableCapacity);
        }
        HashTableLP(uint32_t size) {
            hashTableCapacity = size;
            numKeyValues = size / 2;
            table = create_hashtable(hashTableCapacity);
        }
        ~HashTableLP() {
            destroy_hashtable(table);
        }
        virtual void insert_hashtable(const KeyValue* kvs, uint32_t num_kvs) {
            insert_hashtable(table, kvs, num_kvs);
        }
        virtual void lookup_hashtable(KeyValue* kvs, uint32_t num_kvs) {
            lookup_hashtable(table, kvs, num_kvs);
        }
        virtual void delete_hashtable(const KeyValue* kvs, uint32_t num_kvs) {
            delete_hashtable(table, kvs, num_kvs);
        }
        virtual std::vector<KeyValue> iterate_hashtable() {
            return iterate_hashtable(table);
        }
    };
}