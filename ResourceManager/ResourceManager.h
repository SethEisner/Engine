#pragma once
#include <functional>
#include "../Memory/GeneralAllocator.h"
#include <string>
#include <thread>
#include "../ThreadSafeContainers/HashTable.h"
/*
	add a resource
	check if a resource is in the registry
	free a resource
*/

// need to get my hands on some big boy data to see how reference counting should work
// do reference counting last
// 
// focus on getting the data into memory, then construct objects later once that's down
/* Implementation Notes:
	user can only load or free packaged resources (zip files)
	create a registry entry for each zip file
	registry entries have internal and external dependencies. 
		internal means the required data is within the same resource file (not zip but actual obj file for instance that the registry entry is for)
		external means the required data is within an entirely separate file, that could be within the same packaged resource or within another packaged resource
	manage the lifetimes through referenmce counting. 
	internal dependencies do not update the reference count at all because they are already within the loaded file that is being tracked (track the registry entry of the first load)
	external dependencies do update the reference count
	loading a packaged resouce creates a registry entry for that packaged resource if it does not exist, or increments the reference count if it does exist.
		additionally if the packaged resource does exist, we need up update the reference count of all of it's external references, which can be files or other packaged resources
		use a map/set to keep track of circular dependencies (add the pointer to the external dependencies list to the set if we haven't traversed it yet and only traverse lists that are not in the set)
		may be correct and more efficient to add the GUIDs to the set (i.e. keep a set of visited nodes to be more inline with an actual graph traversal)
	unloading a packaged resource decrements the resource count of the packaged resource count and again traverses the external dependency graph and decrements the reference count of all the resources it visits
	entries are removed from the registry if their reference count becomes zero
	use ziplib to read the given zip files (so I dont have to deal with it)
	GUIDs are hashed filepaths to allow for easy lookup
	pass filpaths as const std::string references instead of char*s so we can actually has the characters and not the pointer to get the GUID. additionally std::string will actually work in the queue
	can eventually replace the std::string with my own string class.
	FILEPATHS INCLUDE THE PATH TO THE FILE AND THE NAMEITSELF, ADDITIONALLY A FILEPATH CAN INCLUDE A SINGLE ".ZIP"

Support for external references in other packaged resources: (must handle loading them if they are unloaded)
	if a packaged resource is not present in the database, the whole filepath is added to the queue that the other thread reads from and loads stuff to memory
	does the thread that reads the data into memory create the registry entry. this is probably the simplest method
	need method for analyzing the loaded files for external dependencies. could look for keywords in the file (e.g. mtllib in an obj file)
		could find a parser library that finds this stuff for us super quickly (e.g. DirectXMesh)
	what happens if we read a packaged resource into memory that has a reference to another unloaded packaged resource
		add the dependency to the external dependency list of the registry entry (probably want to add the GUID to the whole filepath)
		add that packaged resource to the queue (what if the resource is already in the queue)
			when we get a package filepath from the queue, check if it is already in the registry first before reading the contents (to fix double adds) 
				then update the reference count and traverse the dependency graph
		when we load that packaged resource in from memory: (assume it is self contained for now so we dont need to load more stuff)
			traverse external dependencies graph and increment reference count
		how to know when a resource and all of it's dependencies are loaded?
			if we are not marked as ready, traverse the dependency graph
				if all of our dependencies are loaded (exists in the registry) and are ready then so are we

internal references point to something defined in the same file. could create the GUID from the whole filepath plus the name of the data in the file we are interally referencing
could add a map of user identifiable names to their GUID, which would help with internal references. if we asked for enemy skeleton animatin stage 4 then we could get it's GUID direclty...

Handle post load initialization later when I have a better idea about how the memory is allocated and when stuff is constructed
*/

const size_t REFERENCE_ARRAY_SIZE = 16;
class ResourceManager {
	typedef size_t GUID;
	struct RegistryEntry {
		Handle m_handle; // the handle to the general allocator's pointer
		int64_t m_ref_count; // the reference count of the resource
		GUID m_internal_references[REFERENCE_ARRAY_SIZE]; // the GUIDs of internal references - 
		GUID m_external_references[REFERENCE_ARRAY_SIZE]; // the GUIDs of all external references 
		bool m_ready;
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
	void add_resource(const std::string& filepath, void(*function)(char*) = nullptr) {
		// assert that the filepath is a zip file
		// get the GUID of the resource
		// check if the resource is in the registry
		// if it isn't, create a queue_entry item 
		// if the resource is already in the queue increment the reference count of self and all the references
	}
	bool resource_loaded(const std::string& filepath) {
		// get the GUID of the resource
		// return true if the GUID is in the registry and false otherwise
		return false;
	}
	void remove_resource(const std::string& filepath) {
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
	void load_resource(const std::string& filepath) { // occurs in the other thread
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