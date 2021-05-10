#pragma once
#include "vector"
#include "hash_table.h"

namespace LinearProbing {
    KeyValue* create_hashtable();

    void insert_hashtable(KeyValue* hashtable, const KeyValue* kvs, uint32_t num_kvs);

    void lookup_hashtable(KeyValue* hashtable, KeyValue* kvs, uint32_t num_kvs);

    void delete_hashtable(KeyValue* hashtable, const KeyValue* kvs, uint32_t num_kvs);

    std::vector<KeyValue> iterate_hashtable(KeyValue* hashtable);

    void destroy_hashtable(KeyValue* hashtable);
}