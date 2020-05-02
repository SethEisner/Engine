#pragma once
#include <functional>
#include "../Memory/GeneralAllocator.h"
#include <thread>
#include "../ThreadSafeContainers/HashTable.h"
/*
	add a resource
	check if a resource is in the registry
	free a resource
*/

// need to get my hands on some big boy data to see how reference counting should work
// do reference counting last
// or we could only reference cound the zip files... (NO)

const size_t REFERENCE_ARRAY_SIZE = 16;
class ResourceManager {
	typedef size_t GUID;
	struct RegistryEntry {
		Handle m_handle; // the handle to the general allocator's pointer
		int64_t m_ref_count; // the reference count of the resource
		GUID m_internal_references[REFERENCE_ARRAY_SIZE]; // the GUIDs of internal references - 
		GUID m_external_references[REFERENCE_ARRAY_SIZE]; // the GUIDs of all external references 
	}; 
	template <typename T>
	struct QueueEntry {
		char* m_filepath; // use strcpy
		T type; // the type of the object we loaded in. used to call the constructor
		void (*function)(char*); // function to call after loading, is given the loaded data
	};

public:
	ResourceManager() : m_allocator(new GeneralAllocator(1024)), m_thread() {
		// spawns the thread
	}
	~ResourceManager() {
		delete m_allocator;
		delete m_thread;
	}
	void add_resource(char * filepath, void(*function)(char*) = nullptr) {
		// assert that the filepath is a zip file
		// get the GUID of the resource
		// check if the resource is in the registry
		// if it isn't, create a queue_entry item 
		// if the resource is already in the queue increment the reference count of self and all the references
	}
	bool resource_loaded(char* filepath) {
		// get the GUID of the resource
		// return true if the GUID is in the registry and false otherwise
		return false;
	}
	void remove_resource(char* filepath) {
		// get the GUID of the resource
		// decrement the reference count in the registry
		// how to handle circular references // keep a set of already traversed lists so we dont traverse the same list twice
		// decrement the reference counts of the internal references, internal references should have an empty internal reference array though as they contain the actual data(?)
			// if we neewd to de4crement internal references do so, but dont recurse further?
		// reset m_traversed_set
			// decrement our reference count
			// go to the list of external dependencies
			// if the external dependencies list is not in the set, add a pointer to this list to a set
			// traverse the external dependencies list and go to each entry in the registry
			// once in the registry entry, subtract 1 from its reference count
				// if the pointer to the external dependencies array is not in the set, add it and traverse it 
				// otherwise we backstrack
		// if the reference count is zero, free it's memory from the general allocator and remove its registry entry
	}
private:
	void load_resource(char* filepath) { // occurs in the other thread
		// use miniz to read the zip file into memory
		// run the function provided
		// construct the object if we can using the general allocator
		// add a new entry to the registry 
	}
	void start_thread() { // run a loop that tries to pop a filepath from the Queue, and then read the data into memory

	}
	HashTable<void*, bool> m_traversed_set; // use the hashtable as a set 
	GeneralAllocator* m_allocator;
	std::thread* m_thread;
	// queue of filenames
};