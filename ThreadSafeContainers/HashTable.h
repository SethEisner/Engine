#pragma once
#include <assert.h>
#include "../Memory/MemoryManager.h"
#include <shared_mutex>

// use linear allocator, probably want to use general allocator to allow the hash table to be freed
// use quadratic hashing
// use closed hash table
// use shared mutex so multiple threads can read and onely one can write
template <typename Key, typename Value>
class HashTable {
	typedef size_t Hash;
	static const size_t BUCKET_SIZE = 4;
	struct Entry {
		Hash m_hash;
		Value m_value;
	};
	struct Bucket { // each bucket hold 4 entries, which should be more than enough
		Entry m_entry[BUCKET_SIZE];
	};
	
public:
	HashTable(size_t size = 127) : m_table_size(size) { // size is the max number of elements 
		void* temp_pointer = NEW_ARRAY(Bucket, size, memory_manager->get_general_allocator());
		m_table_handle = memory_manager->get_general_allocator()->register_allocated_mem(temp_pointer);
		Bucket* m_table = reinterpret_cast<Bucket*>(temp_pointer);
		for (int i = 0; i != m_table_size; i++) { // set all the m_keys to be 0 so we know they are not in use
			for (int j = 0; j != BUCKET_SIZE; j++) {
				m_table[i].m_entry[j].m_hash = SIZE_MAX;
			}
		}
	}
	~HashTable() {
		memory_manager->free(m_table_handle); // release our block of memory
	}
	void insert(const Key& _key, const Value& _value) { // inserts the element if it doesnt already exist 
		Hash hash = static_cast<Hash>(std::hash<Key>{}(_key));
		assert(hash != SIZE_MAX);
		size_t index = hash % m_table_size;
		Bucket* bucket = reinterpret_cast<Bucket*>(memory_manager->get_general_allocator()->get_pointer(m_table_handle)); // get the up to date pointer from the general allocator
		m_lock.lock(); // lock the mutex for exclusive ownership
		if (bucket_contains(bucket[index], hash) == BUCKET_SIZE) { // if the key value pair is not in the bucket add it to the bucket
			size_t entry_index = get_open_entry(bucket[index]); // returns BUCKET_SIZE if the bucket is full
			assert(entry_index != BUCKET_SIZE);
			bucket[index].m_entry[entry_index].m_hash = hash;
			bucket[index].m_entry[entry_index].m_value = _value;
		}
		m_lock.unlock();
	}
	void remove(const Key& _key) { // removes the element if it can, otherwise doesnt do anything
		Hash hash = static_cast<Hash>(std::hash<Key>{}(_key));
		assert(hash != SIZE_MAX);
		size_t index = hash % m_table_size;
		Bucket* bucket = reinterpret_cast<Bucket*>(memory_manager->get_general_allocator()->get_pointer(m_table_handle)); // get the up to date pointer from the general allocator
		m_lock.lock(); // lock the mutex for exclusive ownership
		size_t entry_index;
		if ((entry_index = bucket_contains(bucket[index], hash)) != BUCKET_SIZE) { // if the key value pair is in the bucket remove it
			assert(entry_index != BUCKET_SIZE);
			bucket[index].m_entry[entry_index].m_hash = SIZE_MAX;
		}
		m_lock.unlock();
	}
	const Value& at(const Key& _key) { // get the value associated with the key, return a constant reference so it cant be modified
		Hash hash = static_cast<Hash>(std::hash<Key>{}(_key));
		assert(hash != SIZE_MAX);
		size_t index = hash % m_table_size;
		Bucket* bucket = reinterpret_cast<Bucket*>(memory_manager->get_general_allocator()->get_pointer(m_table_handle)); // get the up to date pointer from the general allocator
		m_lock.lock_shared(); // lock the mutex for shared ownership
		size_t entry_index = bucket_contains(bucket[index], hash);
		assert(entry_index != BUCKET_SIZE); // assert that the key exists, could throw an exception
		m_lock.unlock_shared();
		return bucket[index].m_entry[entry_index].m_value;
	}
	bool contains(const Key& _key) { // see if something with this key is already in the hashtable
		Hash hash = static_cast<Hash>(std::hash<Key>{}(_key));
		assert(hash != SIZE_MAX);
		size_t index = hash % m_table_size;
		Bucket* bucket = reinterpret_cast<Bucket*>(memory_manager->get_general_allocator()->get_pointer(m_table_handle)); // get the up to date pointer from the general allocator
		m_lock.lock_shared(); // lock the mutex for shared ownership
		size_t entry_index = bucket_contains(bucket[index], hash);
		m_lock.unlock_shared();
		return entry_index != BUCKET_SIZE;
	}
	void set(const Key& _key, const Value& _value) { // sets the value associated with the key to the given value if it already exists, otherwise it will insert the key value pair
		Hash hash = static_cast<Hash>(std::hash<Key>{}(_key));
		assert(hash != SIZE_MAX);
		size_t index = hash % m_table_size;
		Bucket* bucket = reinterpret_cast<Bucket*>(memory_manager->get_general_allocator()->get_pointer(m_table_handle)); // get the up to date pointer from the general allocator
		m_lock.lock(); // lock the mutex for exclusive ownership
		size_t entry_index;
		if (entry_index = bucket_contains(bucket[index], hash) == BUCKET_SIZE) {
			entry_index = get_open_entry(bucket[index]);
			assert(entry_index != BUCKET_SIZE); // assert that the bucket has a spot for us to insert the value
		}
		bucket[index].m_entry[entry_index].m_value = _value;
		m_lock.unlock();
	}
private:
	size_t bucket_contains(const Bucket& bucket, Hash _hash) { // return the index if the hash is present in the array, otherwise returns bucket_size
		int i = 0;
		for (; i != BUCKET_SIZE && bucket.m_entry[i].m_hash != _hash; ++i) {} // loop until we find a bucket entry with the same hash
		return i; // if an empty bucket wasn't found then i is equal to BUCKET_SIZE
	}
	size_t get_open_entry(const Bucket& bucket) { // returns the index of the first open bucket slot, and BUCKET_SIZE otherwise
		int i = 0;
		for (; i != BUCKET_SIZE && bucket.m_entry[i].m_hash != SIZE_MAX; ++i) {} // loop until we find an open bucket slot
		return i; // if an empty bucket wasn't found then i is equal to BUCKET_SIZE
	}

	std::shared_mutex m_lock;
	size_t m_table_size;
	Handle m_table_handle; // handle to our memory, cannot store the pointer because of defragmentation
};