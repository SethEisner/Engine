#include <algorithm>
#include "ResourceManager.h"
#include <stdio.h>
#include "zlib.h"
#include <windows.h>
//#include <windows.h>
//#include <stack>

/* TODO:
*/

static constexpr bool RESOURCE_QUEUE_FULL = false;
static constexpr bool REFERENCE_QUEUE_FULL = false;
static constexpr uint32_t flags = aiProcessPreset_TargetRealtime_Quality | aiProcess_ConvertToLeftHanded | aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices;
//typedef size_t GUID;
// static HashTable<size_t, bool> ref_count_traversed_set;
/* IMPORTANT:
	the user thread(s) should never access the registry directly
	if a user thread wants to modify the registry, it needs to give the changes it wants to make to the resource thread
*/

void ResourceManager::load_resource(const std::string& resource_name, void(*function)(char*)) {
	// resource name is the name of the zip file in the resources folder (set by resource_path) that we need to check if it's loaded
	// assert that the filepath is a zip file
	//assert(resource_name.substr(resource_name.length() - 3, 3) == "zip");
	size_t resource_guid = std::hash<std::string>{}(resource_name);
	// increment it's dependency count because we need to keep track of how the zip was requested
	increment_dependency_count(resource_guid); // function uses a spin lock so we never block, spinlock should be okay because the resource thread will never be never be waiting on a user thread
	if (!m_resource_queue->push(resource_name)) assert(RESOURCE_QUEUE_FULL);
	// can push the item onto the queue regardless of if it's already in the registry because the resource thread will take the given resource and check if it's in the registry itself
	// the resource thread will then update the reference count accordingly
}
bool ResourceManager::resource_loaded(const std::string& filepath) {
	// possible race condition if we remove the resource, it's still loaded because it hasnt actually been freed yet, and then we try to use the resource 
	GUID resource_guid = std::hash<std::string>{}(filepath);
	return get_ready_map(resource_guid);
}
Byte* ResourceManager::get_data_pointer(const std::string& filepath) { // given the filename of the resource, get the pointer to the data contained in the registry
	GUID resource_guid = std::hash<std::string>{}(filepath);
	return reinterpret_cast<Byte*>(m_allocator->get_pointer(m_handle_map->at(resource_guid)));
}
size_t ResourceManager::get_data_size(const std::string& filepath) const {
	GUID resource_guid = std::hash<std::string>{}(filepath);
	return m_size_map->at(resource_guid);
}


void ResourceManager::remove_resource(const std::string& filepath) {
	GUID resource_guid = std::hash<std::string>{}(filepath);
	assert(resource_loaded(filepath)); // make sure remove_resource is only ever called on loaded resources
	// push the reference count changes we would like happen onto the queue so the other thread deals with it
	// because it needs to update the registry and I dont want the user thread to grab the registry lock
	if (!m_ref_count_queue->push(ReferenceCountEntry(resource_guid, -1))) assert(REFERENCE_QUEUE_FULL);
}
const aiScene* ResourceManager::get_scene_pointer(const std::string& file_name) const {
	size_t resource_guid = std::hash<std::string>{}(file_name);
	return m_scene_map->at(resource_guid);
}
// private functions
void ResourceManager::remove_from_ready_map(GUID resource) {
	while (!m_ready_map_lock->try_lock()) {} // spin until we get the lock
	m_ready_map->remove(resource);
	m_ready_map_lock->unlock();
}
void ResourceManager::set_ready_map(GUID resource, bool ready_flag) {
	while (!m_ready_map_lock->try_lock()) {} // spin until we get the lock
	m_ready_map->set(resource, ready_flag);
	m_ready_map_lock->unlock();
}
bool ResourceManager::get_ready_map(GUID resource) {
	while (!m_ready_map_lock->try_lock_shared()) {} // spin until we get the lock
	bool ret = m_ready_map->contains(resource) && m_ready_map->at(resource);
	m_ready_map_lock->unlock_shared();
	return ret;
}
void ResourceManager::increment_dependency_count(GUID resource_guid) {
	while (!m_dependency_count_lock->try_lock()) {} // spin until we get the lock, mainly so the user thread(s) don't block
	size_t count = 0;
	if (m_dependency_count->contains(resource_guid)) { // increment the dependency count of the zip file in the dependency map
		count = m_dependency_count->at(resource_guid); // okay because we are not using it to change count
		m_dependency_count->set(resource_guid, count + 1);
	}
	else {
		m_dependency_count->insert(resource_guid, 1);
	}
	// m_dependency_count->set(resource_guid, count + 1);
	m_dependency_count_lock->unlock();
}
void ResourceManager::remove_dependency_count(GUID resource_guid) {
	m_dependency_count_lock->lock();
	m_dependency_count->set(resource_guid, 0);
	m_dependency_count_lock->unlock();
}
size_t ResourceManager::get_dependency_count(GUID resource_guid) {
	size_t count = 0;
	m_dependency_count_lock->lock();
	if (m_dependency_count->contains(resource_guid)) count = m_dependency_count->at(resource_guid);
	m_dependency_count_lock->unlock();
	return count;
}
void ResourceManager::change_reference_count_by(GUID registry_entry, int delta) {
	RegistryEntry entry = m_registry->at(registry_entry);
	// entry.m_ref_count += delta;
	m_registry->at(registry_entry).m_ref_count += delta;
	for (int i = 0; i != REFERENCE_ARRAY_SIZE; i++) {
		if (entry.m_external_references[i] != SIZE_MAX) {
			change_reference_count_by(entry.m_external_references[i], delta);
		}
	}
}
byte* ResourceManager::read_resource_into_memory(const std::string& zip_file) {
	GUID zip_hash = std::hash<std::string>{}(zip_file);
	if (m_registry->contains(zip_hash)) { // if the zip is already in the registry, update it's reference count
		size_t zip_dependency_count = get_dependency_count(zip_hash);
		//reset_ref_count_traversed_set();
		change_reference_count_by(zip_hash, static_cast<int>(zip_dependency_count)); // increment it's count because we need to keep track of all load calls
		//return -1;
		return nullptr;
	}
	std::string path = resource_path + zip_file;
	FILE* fp = fopen(path.c_str(), "rb");
	assert(fp != nullptr);
	fseek(fp, 0, SEEK_END);
	size_t file_length = ftell(fp);
	rewind(fp);
	//byte* compressed = NEW_ARRAY(byte, file_length, m_allocator); // allocate space for the compressed memory from our general allocator
	//Handle h_compressed = m_allocator->register_allocated_mem(compressed);
	byte* compressed = new byte[file_length];
	fread(compressed, file_length, 1, fp); // read the file into memory
	fclose(fp);
	//return h_compressed;
	return compressed;
}
// void ResourceManager::unzip_resource(const std::string& zip_file, Handle h_compressed) { // occurs in the other thread
void ResourceManager::unzip_resource(const std::string& zip_file, byte* compressed) { // occurs in the other thread
	z_stream infstream;
	infstream.zalloc = Z_NULL;
	infstream.zfree = Z_NULL;
	infstream.opaque = Z_NULL;
	std::string path;
	std::string file_name;
	size_t compressed_size = 0;
	size_t uncompressed_size = 0;
	size_t offset = 0;
	GUID zip_hash = std::hash<std::string>{}(zip_file);;
	GUID resource_guid;
	//byte* compressed = reinterpret_cast<byte*>(m_allocator->get_pointer(h_compressed));
	size_t zip_dependency_count = get_dependency_count(zip_hash);
	// make handle be negative becuase zips dont have ny memory associated with them
	m_registry->insert((zip_hash), RegistryEntry(-1, 0, zip_dependency_count, false, FileType::ZIP));
	set_ready_map(zip_hash, false);
	m_zip_files->push_back(zip_hash);
	// decompress each file in the zip into its own registry entry
	size_t external_resource_index = 0;
	while (get_compressed_file_info(compressed, compressed_size, uncompressed_size, offset, file_name)) { // there is still a file that needs decompressing
		//uncompressed_size++; // add space for the null terminator so we can use strstr later
		byte* uncompressed = NEW_ARRAY(byte, uncompressed_size, m_allocator); // create space for the uncompressed file
		assert(uncompressed != nullptr);
		Handle h_uncompressed = m_allocator->register_allocated_mem(uncompressed);// +1);
		infstream.avail_in = static_cast<uInt>(compressed_size);//(uInt)((unsigned char*)defstream.next_out - b); // size of input
		infstream.next_in = compressed + offset; // input char array
		infstream.avail_out = static_cast<uInt>(uncompressed_size); // size of output
		infstream.next_out = uncompressed; // output char array
		inflateInit2(&infstream, -15); // inflateInit2 skips the headers and looks for a raw stream 
		int ret = inflate(&infstream, Z_NO_FLUSH);
		assert(ret >= 0); // inflate must have succeeded
		inflateEnd(&infstream);
		//uncompressed[uncompressed_size] = '\0';
		offset += compressed_size;
		// make the registry entry for this newly uncompressed file inside the zip file
		std::string full_name = zip_file + '/' + file_name;
		FileType file_type = get_file_type(file_name);
		resource_guid = std::hash<std::string>{}(full_name);
		// look at the dependency map to see if anything depends on this file
		size_t count = get_dependency_count(resource_guid);
		remove_dependency_count(resource_guid);
		// add 1 to the count because a resource file always is depended on by the zip it is contained within
		// add the dependency counnt of the zip file to the count
		m_registry->insert(resource_guid, RegistryEntry(h_uncompressed, uncompressed_size, count + zip_dependency_count, false, file_type)); // moves the registry entry into the hash table
		m_handle_map->insert(resource_guid, h_uncompressed);
		m_size_map->insert(resource_guid, uncompressed_size);
		set_ready_map(resource_guid, false);
		// add every file we decompress to the external dependencies list of the zip file
		add_external_dependency(zip_hash, resource_guid);
		m_potentially_ready->push_back(resource_guid);
	}
	//m_allocator->free(h_compressed); // free the compressed buffer
	delete[] compressed;
	// get the external dependencies in the zip files
	//m_dependencies_traversed_set->clear();
	traverse_dependency_graph(zip_hash, zip_file, &ResourceManager::get_external_dependencies); // actually reads the uncompressed data if it
	// add this zip to an unready list
	m_potentially_ready->push_back(zip_hash);
}
void ResourceManager::get_external_dependencies(GUID resource, const std::string& zip_file) {
	// zip_file can be treated as the base directory
	if (!m_registry->contains(resource)) return; // if the resource is not in the registry then cant do anything here
	/* NOTE:
		should not need to put a lock on entry here because we only modify the entry in a function that locks it itself
	*/
	RegistryEntry entry = m_registry->at(resource);
	char* p_mem = reinterpret_cast<char*>(m_allocator->get_pointer(entry.m_handle));
	//uint32_t flags = 0; // flags for Assimp, 0 does nothing special
	//aiScene* scene_ptr = nullptr;
	switch (entry.m_file_type) {
	case FileType::ZIP:
		assert(false); // we should never get here
	case FileType::MTL: // mtls are self contained
		break;
	//case FileType::FBX:
	case FileType::DAE: { // zip file contains a dae file
		const aiScene* scene_ptr = m_importer->ReadFileFromMemory(p_mem, entry.m_size, flags);
		if (!scene_ptr) { // will be a nullptr if there was an error
			OutputDebugStringA(m_importer->GetErrorString());
			break;
		}
		m_scene_map->insert(resource, scene_ptr);
		break;
	}
	case FileType::DDS: { // texture files, shouldn't be anything special for dds files as they have no external dependencies

	}
	case FileType::OBJ:
		char needle[] = { 'm', 't', 'l', 'l', 'i', 'b', ' ' };
		auto p_reference = std::search(p_mem, p_mem + entry.m_size, std::begin(needle), std::end(needle)) + sizeof(needle);
		while (p_reference < p_mem + entry.m_size) { // search entire file for external references
			std::string reference;
			size_t index = 0;
			char next_char = p_reference[index];
			while (!(next_char == '\r' || next_char == '\n')) {
				reference += next_char;
				next_char = p_reference[++index];
			}
			// reference contains the name of the file we are looking at
			// if we are referencing a file in a different zip file, then load that zip file
			// add the file to our dependency list
			std::string full_path = zip_file + '/' + reference;
			GUID dependency_hash = std::hash<std::string>{}(full_path);
			if (m_registry->contains(dependency_hash)) { // reference is already in the registry so increment its reference count
				//reset_ref_count_traversed_set();
				change_reference_count_by(dependency_hash, 1);
			}
			else { // if our zip file + the reference file name is not in the dictionary, then we are referencing a file that's in another zip file
				dependency_hash = std::hash<std::string>{}(reference);
				increment_dependency_count(dependency_hash);
				// get the zip part of the reference (up until the first /)
				size_t index = reference.find('/', 0); // get first occurance of /
				std::string dependent_zip(reference, 0, index);
				//Handle handle = read_resource_into_memory(dependent_zip);
				Byte* handle = read_resource_into_memory(dependent_zip);
				// dont call load_resource because that is the user's function as adds to the dependency map
				//assert(handle != -1); //if handle is -1 then we couldnt get a free block large enough
				assert(handle != nullptr);
				unzip_resource(dependent_zip, handle);
			}
			// add the file to our dependency list
			add_external_dependency(resource, dependency_hash);
			// get the next dependency
			p_reference = std::search(p_reference, p_mem + entry.m_size, std::begin(needle), std::end(needle)) + sizeof(needle);
		}
		break;
	}
}
void ResourceManager::traverse_dependency_graph(GUID resource, const std::string& zip_file, void (ResourceManager::* node_function)(GUID, const std::string&)) {//void(*node_function)(GUID)) { // give it a function to call on every node
	// traverse the external dependency graph for the zip file
	// for each GUID in the graph
	// look inside of it for keywords of external references...
	// foreach external reference keyword (for obj file look for mtllib) make sure that the referenced file is loaded
	// add the file as an external dependency of the loaded (non-zip) file
	// depth first search
	RegistryEntry entry;
	entry = m_registry->at(resource);
	for (int i = 0; i != REFERENCE_ARRAY_SIZE; i++) {
		if (entry.m_external_references[i] != SIZE_MAX) {
			(this->*node_function)(entry.m_external_references[i], zip_file); // call the function on the node
			traverse_dependency_graph(entry.m_external_references[i], zip_file, node_function); // visit the node to traverse it
		}
	}
}
bool ResourceManager::get_compressed_file_info(byte* compressed, size_t& compressed_size, size_t& uncompressed_size, size_t& offset, std::string& file_name) {
	uncompressed_size = 0;
	compressed_size = 0;
	static constexpr uint32_t central_director_file_header_signature = 0x02014b50;
	static constexpr uint32_t file_header_signature = 0x04034b50;
	static constexpr size_t local_file_header_size = 30;
	uint32_t header;
	if (*reinterpret_cast<uint32_t*>(compressed + offset) != central_director_file_header_signature) { // still have compressed data to uncompress
		do { // skip over any directory entries
			header = *reinterpret_cast<uint32_t*>(compressed + offset);
			compressed_size = *reinterpret_cast<uint32_t*>(compressed + offset + 18);
			uncompressed_size = *reinterpret_cast<uint32_t*>(compressed + offset + 22);
			uint16_t n = *reinterpret_cast<uint16_t*>(compressed + offset + 26);
			uint16_t m = *reinterpret_cast<uint16_t*>(compressed + offset + 28);

			std::string name(reinterpret_cast<char*>(compressed + offset + 30), n);
			file_name = name; // copy the name into the reference;
			offset += local_file_header_size + static_cast<size_t>(n) + static_cast<size_t>(m); // offset returns the start of the compressed data
		} while (header == file_header_signature && uncompressed_size == 0);
		return true;
	}
	return false;
}
void ResourceManager::ready_resources() {
	// a resource is ready if all of it's external dependencies are in the registry, and all of its dependencies' resources are in the registry
	// for each resource
	std::list<GUID> ready;
	std::list<GUID>::iterator begin;
	for (begin = m_potentially_ready->begin(); begin != m_potentially_ready->end(); begin++) { // iterate over the potentially ready list (contains zips)
		// if there are no external dependencies or 
		// if all of our external depdencies are ready then we are ready
		if (empty_external_depencies(*begin) || all_dependencies_ready(*begin)) {
			set_resource_ready(*begin);
			//set_ready_map(*begin, true); // mark it as ready in the ready map
			ready.push_back(*begin);//  m_potentially_ready->remove(*begin);
		}
	}
	for (begin = ready.begin(); begin != ready.end(); begin++) { // iterate over the ready list
		m_potentially_ready->remove(*begin);
	}
	ready.clear();
}
int64_t ResourceManager::get_dependency_sum(GUID resource) {
	int64_t sum = 0;
	if (m_registry->contains(resource)) {
		RegistryEntry entry = m_registry->at(resource);
		assert(entry.m_ref_count >= 0); // the reference count should always be positive and if it's not then something bad happened with freeing too ma
		sum = entry.m_ref_count;
		for (int i = 0; i != REFERENCE_ARRAY_SIZE; i++) {
			if (entry.m_external_references[i] != SIZE_MAX) {
				sum += get_dependency_sum(entry.m_external_references[i]);
			}
		}
	}
	else { // the entry was not in the registry, however we only got here because something depended on us so we must consider the reference count to be something positive
		sum = 1lu << 31;
	}
	return sum;

}
void ResourceManager::free_resource_graph(GUID resource) {
	if (m_registry->contains(resource)) {
		RegistryEntry entry = m_registry->at(resource);
		for (int i = 0; i != REFERENCE_ARRAY_SIZE; i++) {
			free_resource_graph(entry.m_external_references[i]);
			if (all_dependencies_free(resource)) { // the array is empty so we can free ourself, remove ourself from the registry, and return
				if (entry.m_file_type != FileType::ZIP) m_allocator->free(entry.m_handle); // free the memory associated with us if we have any
				m_registry->remove(resource); // remove ourself from the registry
				remove_from_ready_map(resource);
				return;
			}
		}
	}
}
void ResourceManager::free_resources() {
	// assert that no reference counts are negative
	// can only free ready resources so we dont mess up the registry while building 
	// for each zip file entry that has a reference count of zero, (start at zip because that is always the root of a tree structure
	//		traverse its dependency graph and sum up all of the reference counts within.
	//		also, if a resource is not ready then we cannot trust i
	//		if that sum is zero, then we can free the resource and all of it's dependencies
	// maintain a list of zip file entries to iterate over
	std::list<GUID> free_list;
	std::list<GUID>::iterator begin;
	int64_t sum = 0;
	for (begin = m_zip_files->begin(); begin != m_zip_files->end(); begin++) {
		RegistryEntry entry = m_registry->at(*begin);
		if (entry.m_ready) {
			//m_dependencies_traversed_set->clear();
			sum = get_dependency_sum(*begin);
			if (sum == 0) free_list.push_back(*begin); // race condition?
		}
	}
	for (begin = free_list.begin(); begin != free_list.end(); begin++) {
		free_resource_graph(*begin);
		m_zip_files->remove(*begin); // remove the zip file from our list
	}
}
void ResourceManager::start_resource_thread(void) { // run a loop that tries to pop a filepath from the Queue, and then read the data into memory
	std::string zip_file;
	ReferenceCountEntry entry;
	byte* handle;
	while (run_flag) {
		// if pop returned true then resource name has the name of the resource we need to unzip
		if (m_resource_queue->pop(zip_file)) {
			// if ((handle = read_resource_into_memory(zip_file)) != -1) {
			if ((handle = read_resource_into_memory(zip_file)) != nullptr) {
				unzip_resource(zip_file, handle);
			}
		}
		if (m_potentially_ready->size() > 0) ready_resources();
		if (m_zip_files->size() > 0) free_resources();
		while (m_ref_count_queue->pop(entry)) { // should be done after unzipping
			//reset_ref_count_traversed_set();
			change_reference_count_by(entry.m_resource, entry.m_delta);
		}
	}
}
void ResourceManager::set_resource_ready(GUID resource) { // changes the registry entry so needs to lock
	m_registry->at(resource).m_ready = true;
	set_ready_map(resource, true);
}
bool ResourceManager::all_dependencies_ready(GUID resource) { // a readonly function
	size_t i = 0;
	bool ret = true;
	RegistryEntry entry = m_registry->at(resource);
	for (; i != REFERENCE_ARRAY_SIZE; ++i) { // need to search the entire array because freeing will put holes in the array
		if (entry.m_external_references[i] != SIZE_MAX) { // we have something at this index
			if ((!m_registry->contains(entry.m_external_references[i])) || // the dependency is not in the registry
				(!m_registry->at(entry.m_external_references[i]).m_ready)) {
				ret = false; // the dependency isnt in the registry so we are not ready
				break;
			}
		}
	}
	return ret;
}
bool ResourceManager::all_dependencies_free(GUID resource) {
	int i = 0;
	RegistryEntry entry = m_registry->at(resource);
	for (; i != REFERENCE_ARRAY_SIZE && (entry.m_external_references[i] == SIZE_MAX || !(m_registry->contains(entry.m_external_references[i]))); ++i) {}
	return (i == REFERENCE_ARRAY_SIZE); // if we reached the end of the array then every reference is size_max so we are entirely empty
}
bool ResourceManager::empty_external_depencies(GUID resource) { // a readonly function
	int i = 0;
	RegistryEntry entry = m_registry->at(resource);
	for (; i != REFERENCE_ARRAY_SIZE && entry.m_external_references[i] == SIZE_MAX; ++i) {}
	return (i == REFERENCE_ARRAY_SIZE); // if we reached the end of the array then every reference is size_max so we are entirely empty
}
void ResourceManager::add_external_dependency(GUID resource, GUID dependency) { // modifies the dependency array so we need it to lock it
	int i = 0;
	RegistryEntry entry = m_registry->at(resource); // get the resoure
	for (; i != REFERENCE_ARRAY_SIZE && entry.m_external_references[i] != SIZE_MAX; ++i) {

	} // find an empty slot
	assert(i != REFERENCE_ARRAY_SIZE); // assert that an empty slot was found
	//entry.m_external_references[i] = dependency; // modify the dependency array
	m_registry->at(resource).m_external_references[i] = dependency;
}
ResourceManager::FileType ResourceManager::get_file_type(const std::string& file_name) {
	size_t index = 0;
	index = file_name.rfind("."); // find the last occurance of the . to get the file extension
	assert(index != std::string::npos); // assert that the file has an extension
	std::string extension(file_name.c_str() + index, file_name.length() - index);
	if (extension == ".zip") return FileType::ZIP;
	if (extension == ".obj") return FileType::OBJ;
	if (extension == ".mtl") return FileType::MTL;
	if (extension == ".dae") return FileType::DAE;
	if (extension == ".dds") return FileType::DDS;
	//if (extension == ".fbx") return FileType::FBX;
	assert(false); // the file extension is one we dont know how to handle
	return FileType::INVALID;
}

// Ogex* parse_ogex(const char* begin, const char* end) {
// 	Ogex* ogex = new Ogex();
// }