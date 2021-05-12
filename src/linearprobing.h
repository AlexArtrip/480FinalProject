#pragma once
#include "vector"
#include "hash_table.h"
#include <string>

namespace LinearProbing {
    KeyValue* create_hashtable(uint size);

    void insert_hashtable(KeyValue* hashtable, uint size, const KeyValue* kvs, uint num_kvs);

    void lookup_hashtable(KeyValue* hashtable, uint size, KeyValue* kvs, uint num_kvs);

    void delete_hashtable(KeyValue* hashtable, uint size, const KeyValue* kvs, uint num_kvs);

    std::vector<KeyValue> iterate_hashtable(KeyValue* hashtable, uint size);

    void destroy_hashtable(KeyValue* hashtable);

    class HashTableLP : public HashTable {
    public:
        HashTableLP() {
            hashTableCapacity = 128 * 1024 * 1024;
            numKeyValues = hashTableCapacity / 2;
            table = create_hashtable(hashTableCapacity);
        }
        HashTableLP(uint size) {
            hashTableCapacity = size;
            numKeyValues = size / 2;
            table = create_hashtable(hashTableCapacity);
        }
        ~HashTableLP() {
            destroy_hashtable(table);
        }
        virtual void insert_hashtable(const KeyValue* kvs, uint num_kvs) {
            LinearProbing::insert_hashtable(table, hashTableCapacity, kvs, num_kvs);
        }
        virtual void lookup_hashtable(KeyValue* kvs, uint num_kvs) {
            LinearProbing::lookup_hashtable(table, hashTableCapacity, kvs, num_kvs);
        }
        virtual void delete_hashtable(const KeyValue* kvs, uint num_kvs) {
            LinearProbing::delete_hashtable(table, hashTableCapacity, kvs, num_kvs);
        }
        virtual std::vector<KeyValue> iterate_hashtable() {
            return LinearProbing::iterate_hashtable(table, hashTableCapacity);
        }
        virtual const std::string name() {
            return "Linear Probing ";
        }
    };
}