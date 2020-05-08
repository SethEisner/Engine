#pragma once
#include <atomic>
#include "../Memory/GeneralAllocator.h"
#include <string>
#include <thread>
#include "../ThreadSafeContainers/HashTable.h"
#include "../ThreadSafeContainers/Queue.h"
#include <list>
#include <shared_mutex>
#include <set>
#include <stack>
#include <mutex>
/* TODO:
	implement a get resource function that returns the handle, might need a map from a GUID to a handle, i dont think reading the m_registry without a lock is safe...
		the entry could be removed between when we check if it's still in the registry and when we go to get its handle
	wont need to grab a lock because the handle is never changed and the resource wont be freed out from under us
	replace std::set with our own hashtable
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
static constexpr char resource_path[52] = "C:/Users/Seth Eisner/source/repos/Engine/Resources/";
//void start_resource_thread(void);
class ResourceManager {
	typedef size_t GUID;
	typedef uint8_t byte;
	enum class FileType { ZIP, OBJ, MTL, INVALID };
	struct ReferenceCountEntry {
		GUID m_resource; // the resource to update the reference count of
		int m_delta; // the number to change the reference count by
		ReferenceCountEntry() : m_resource(SIZE_MAX), m_delta(0) {};
		ReferenceCountEntry(GUID _resource, int _delta) : m_resource(_resource), m_delta(_delta) {};
		ReferenceCountEntry& operator=(const ReferenceCountEntry& rhs) {
			if (this != &rhs) { // check for self assignment
				this->m_resource = rhs.m_resource;
				this->m_delta = rhs.m_delta;
			}
			return *this;
		}
		ReferenceCountEntry& operator=(ReferenceCountEntry&& rhs) noexcept {
			if (this != &rhs) { // check for self assignment
				this->m_resource = rhs.m_resource;
				this->m_delta = rhs.m_delta;
			}
			return *this;
		}
	};
	struct RegistryEntry {
		int64_t m_handle; // the handle to the general allocator's pointer
		int64_t m_ref_count; // the reference count of the resource
		size_t m_size; // size of the memory block 
		GUID m_internal_references[REFERENCE_ARRAY_SIZE]; // the GUIDs of internal references - 
		GUID m_external_references[REFERENCE_ARRAY_SIZE]; // the GUIDs of all external references 
		FileType m_file_type;
		bool m_ready;
		RegistryEntry(Handle _handle, size_t _size, int64_t _ref_count, bool _ready, FileType _file_type) : 
			m_size(_size), 
			m_handle(_handle),  
			m_ref_count(_ref_count), 
			m_internal_references(), 
			m_external_references(), 
			m_ready(_ready),
			m_file_type(_file_type) {
			for (int i = 0; i != REFERENCE_ARRAY_SIZE; ++i) {
				m_internal_references[i] = SIZE_MAX; // size_max is our indicator that the GUID does not exist
				m_external_references[i] = SIZE_MAX;
			}
		}
		RegistryEntry() = default; // default initialize
		RegistryEntry& operator=(const RegistryEntry& rhs) { // need move constructor because atomics
			if (this != &rhs) {
				this->m_size = rhs.m_size;
				this->m_handle = rhs.m_handle;
				this->m_ref_count = rhs.m_ref_count;
				std::copy(rhs.m_internal_references, rhs.m_internal_references + REFERENCE_ARRAY_SIZE, this->m_internal_references);
				std::copy(rhs.m_external_references, rhs.m_external_references + REFERENCE_ARRAY_SIZE, this->m_external_references);
				this->m_ready = rhs.m_ready;
				this->m_file_type = rhs.m_file_type;
			}
			return *this;
		}
	}; 
	template <typename T>
	struct QueueEntry {
		char* m_filepath; // use strcpy
		T type; // the type of the object we loaded in. used to call the constructor
		void (*function)(char*); // function to call after loading, is given the loaded data
	};
public:
	ResourceManager() : 
		m_registry(new HashTable<GUID, RegistryEntry>()),
		m_ready_map (new HashTable<GUID, bool>()),
		m_ready_map_lock (new std::shared_mutex()),
		m_dependency_count (new HashTable<GUID,size_t>()),
		m_dependency_count_lock (new std::mutex()),
		// m_dependencies_traversed_set(new HashTable<GUID,bool>()),
		m_dependencies_traversed_set (new std::set<GUID>()),
		m_allocator (new GeneralAllocator(1024*1024*1024)), 
		m_thread (new std::thread(&ResourceManager::start_resource_thread, this)), 
		m_resource_queue (new Queue<std::string>(16)),
		m_ref_count_queue (new Queue<ReferenceCountEntry>(16)),
		m_potentially_ready (new std::list<GUID>()),
		m_zip_files (new std::list<GUID>()),
		// m_traversed_sets_stack (new std::stack<HashTable<GUID, bool>*>())
		m_traversed_sets_stack(new std::stack<std::set<GUID>*>()) {}
	~ResourceManager() {
		run_flag = false; // set the run_flag to false to stop the thread from looping
		m_thread->join(); // 
		delete m_registry;
		delete m_ready_map;
		delete m_ready_map_lock;
		delete m_dependency_count;
		delete m_dependency_count_lock;
		delete m_dependencies_traversed_set;
		delete m_allocator;
		delete m_thread;
		delete m_resource_queue;
		delete m_ref_count_queue;
		delete m_potentially_ready;
		delete m_zip_files;
		delete m_traversed_sets_stack;
	}
	void load_resource(const std::string&, void(*function)(char*) = nullptr);
	bool resource_loaded(const std::string&);
	void remove_resource(const std::string&);
private:
	void remove_from_ready_map(GUID);
	void set_ready_map(GUID, bool);
	bool get_ready_map(GUID);
	void increment_dependency_count(GUID);
	void remove_dependency_count(GUID);
	size_t get_dependency_count(GUID); 
	void reset_ref_count_traversed_set();
	void change_reference_count_by(GUID, int);
	void get_external_dependencies(GUID, const std::string&);
	void traverse_dependency_graph(GUID, const std::string&, void (ResourceManager::*func_p)(GUID, const std::string&));
	Handle read_resource_into_memory(const std::string& zip_file);
	void unzip_resource(const std::string&, Handle);
	//std::string get_file_name(byte* compressed, size_t& offset);
	bool get_compressed_file_info(byte* compressed, size_t& compressed_size, size_t& uncompressed_size, size_t& offset, std::string& file_name);
	void ready_resources(void);
	int64_t get_dependency_sum(GUID);
	void free_resource_graph(GUID);
	void free_resources(void);
	void start_resource_thread(void);
	void add_external_dependency(GUID, GUID);
	bool all_dependencies_ready(GUID);
	bool all_dependencies_free(GUID);
	void set_resource_ready(GUID);
	bool empty_external_depencies(GUID);
	FileType get_file_type(const std::string&);
	// m_registry is shared across threads
	HashTable<GUID, RegistryEntry>* m_registry; // registry to keep track of what has been loaded
	HashTable<GUID, bool>* m_ready_map; // map from GUIDs to bools to we dont need to read the registry, only the resource thread should have access to the registry
	std::shared_mutex* m_ready_map_lock;
	HashTable<GUID, size_t>* m_dependency_count; // used to mark dependencies of unloaded stuff, 
	std::mutex* m_dependency_count_lock;
	// HashTable<GUID, bool>* m_dependencies_traversed_set;
	std::set<GUID>* m_dependencies_traversed_set; // used to read in dependencies
	 // used to update reference counts
	GeneralAllocator* m_allocator; // use own general allocator
	std::thread* m_thread;
	Queue<std::string>* m_resource_queue; // queue of names of zip files to load
	Queue<ReferenceCountEntry>* m_ref_count_queue;
	std::list<GUID>* m_potentially_ready;
	std::list<GUID>* m_zip_files;
	std::stack<std::set<GUID>*>* m_traversed_sets_stack; // 
	bool run_flag = true; // dont need synchronization on this because only 1 thread writes to it and the other reads
	// std::shared_mutex* m_lock; // need to implement locking in a better way because we have two threads internal to this class
	/* remove m_ref_count_traversed_set_lock */
	// std::recursive_mutex* m_ref_count_traversed_set_lock;
	/* remove m_registry_lock */
	// std::shared_mutex* m_registry_lock; // shouldnt need the lock because only one thread modifies the registry, and we put an atomic around the member that is shared
	// HashTable<GUID, std::shared_mutex*>* m_registry_entry_locks;
	// std::stack<HashTable<GUID, bool>*>* m_traversed_sets_stack;
	
	/* SHARED objects:
		m_ready_map, needs an actual lock
		m_dependency_map, this needs an actual lock
		m_resource_queue (threadsafe queue so we dont need to worry about it)
	*/
	/* RESOURCE THREAD objects: (objects used exclusively by the resource thread so no synchronization is required)
		m_registry,
		m_dependencies_traversed_set,
		m_ref_count_traversed_set,
		m_potentially_ready,
		m_traversed_sets_stack,
		m_allocator
	*/
	/* OTHER THREAD objects:
		m_thread
	*/
};