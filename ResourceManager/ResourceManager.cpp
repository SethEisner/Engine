#include "ResourceManager.h"
#include <stdio.h>
#include "zlib.h"
#include <windows.h>
//#include <stack>

/* TODO:

*/

static constexpr bool RESOURCE_QUEUE_FULL = false;

void ResourceManager::load_resource(const std::string& resource_name, void(*function)(char*)) {
	// resource name is the name of the zip file in the resources folder (set by resource_path) that we need to check if it's loaded
	
	// assert that the filepath is a zip file
	// get the GUID of the resource
	size_t resource_guid = std::hash<std::string>{}(resource_name);
	// check if the resource is in the registry
	if (!m_registry->contains(resource_guid)) {
	// assert here because the push must return true, if it returns false then the queue is full and we need to make it larger
		if (m_dependency_map->contains(resource_guid)) { // increment the dependency count of the zip file in the dependency map
			size_t* count = m_dependency_map->at(resource_guid);
			++count;
		}
		else {
			m_dependency_map->insert(resource_guid, 1);
		}
		if (!m_resource_queue->push(resource_name)) assert(RESOURCE_QUEUE_FULL);
	}
	else {
		m_ref_count_traversed_set->reset(); // reset function is thread safe
		m_lock->lock();
		increment_reference_count(resource_guid); // another thing loaded it so we need to increment it's rerference count and the reference count of what it references
		m_lock->unlock();
	}
	// if it isn't, create a queue_entry item 
	// if the resource is already in the queue increment the reference count of self and all the references
}
bool ResourceManager::resource_loaded(const std::string& filepath) {
	// get the GUID of the resource
	// return true if the GUID is in the registry and it's ready
	size_t resource_guid = std::hash<std::string>{}(filepath);
	return (m_registry->contains(resource_guid) && m_registry->at(resource_guid)->m_ready); // m_registry (HashTable) is threadsafe
}
void ResourceManager::remove_resource(const std::string& filepath) {
	// get the GUID of the resource
	// decrement the reference count in the registry
	// how to handle circular references // keep a set of already traversed lists so we dont traverse the same list twice
	// decrement the reference counts of the internal references, internal references should have an empty internal reference array though as they contain the actual data(?)
		// if we neewd to decrement internal references do so, but dont recurse further?
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
// private functions
void ResourceManager::increment_reference_count(GUID registry_entry) {
	//RegistryEntry* entry = &m_registry->at(registry_entry);
	RegistryEntry* entry = m_registry->at(registry_entry);
	entry->m_ref_count += 1;
	for (int i = 0; i != REFERENCE_ARRAY_SIZE && entry->m_external_references[i] != SIZE_MAX && !(m_ref_count_traversed_set->contains(entry->m_external_references[i])) ; i++) {
		m_ref_count_traversed_set->insert(entry->m_external_references[i], true); // mark the node as visited before we visit it
		//traverse_dependency_graph(entry->m_external_references[i], node_function); // visit the node to traverse it
		increment_reference_count(entry->m_external_references[i]);
	}
	// foreach GUID in the external references, increment their reference count too (recursive)
}
void ResourceManager::unzip_resource(const std::string& zip_file) { // occurs in the other thread
	z_stream infstream;
	infstream.zalloc = Z_NULL;
	infstream.zfree = Z_NULL;
	infstream.opaque = Z_NULL;
	std::string path;
	std::string file_name;
	size_t compressed_size = 0;
	size_t uncompressed_size = 0;
	size_t offset = 0;
	size_t zip_hash;
	RegistryEntry* zip_entry;
	GUID resource_guid;
	zip_hash = std::hash<std::string>{}(zip_file);
	if (m_registry->contains(zip_hash)) {
		m_ref_count_traversed_set->reset();
		increment_reference_count(zip_hash); // increment it's count because we need to keep track of all load calls
		return;
	}

	// load the compressed data into memory
	path = resource_path + zip_file;
	FILE* fp = fopen(path.c_str(), "rb");
	assert(fp != nullptr);
	fseek(fp, 0, SEEK_END);
	size_t file_length = ftell(fp);
	rewind(fp);
	byte* compressed = NEW_ARRAY(byte, file_length, m_allocator); // allocate space for the compressed memory from our general allocator
	int h_compressed = m_allocator->register_allocated_mem(compressed);
	fread(compressed, file_length, 1, fp); // read the file into memory
	fclose(fp);
	// make the registry entry for the zip file
	// read the dependency count of the zip from the dependency map
	size_t zip_dependency_count = 0;
	if (m_dependency_map->contains(zip_hash)) {
		zip_dependency_count = *m_dependency_map->at(zip_hash);
	}
	m_registry->insert((zip_hash), RegistryEntry(0, 0, zip_dependency_count, false, FileType::ZIP));
	zip_entry = m_registry->at(zip_hash);
	// decompress each file in the zip into its own registry entry
	size_t external_resource_index = 0;
	while (get_compressed_file_info(compressed, compressed_size, uncompressed_size, offset, file_name)) { // there is still a file that needs decompressing
		uncompressed_size++; // add space for the null terminator so we can use strstr later
		byte* uncompressed = NEW_ARRAY(byte, uncompressed_size, m_allocator); // create space for the uncompressed file
		assert(uncompressed != nullptr);
		int h_uncompressed = m_allocator->register_allocated_mem(uncompressed);
		infstream.avail_in = compressed_size;//(uInt)((unsigned char*)defstream.next_out - b); // size of input
		infstream.next_in = compressed + offset; // input char array
		infstream.avail_out = uncompressed_size; // size of output
		infstream.next_out = uncompressed; // output char array
		inflateInit2(&infstream, -15); // inflateInit2 skips the headers and looks for a raw stream 
		int ret = inflate(&infstream, Z_NO_FLUSH);
		assert(ret >= 0); // inflate must have succeeded
		inflateEnd(&infstream);
		uncompressed[uncompressed_size] = '\0';
		offset += compressed_size;
		// make the registry entry for this newly uncompressed file inside the zip file
		std::string full_name = zip_file + '/' + file_name;
		FileType file_type = get_file_type(file_name);
		resource_guid = std::hash<std::string>{}(zip_file + '/' + file_name);
		// look at the dependency map to see if anything depends on this file
		size_t count = 0;
if (m_dependency_map->contains(resource_guid)) {
	count = *m_dependency_map->at(resource_guid);
	m_dependency_map->remove(resource_guid); // remove it because we have read the count and don't need it anymore
}
// add 1 to the count because a resource file always is depended on by the zip it is contained within
// add the dependency counnt of the zip file to the count
m_registry->insert(resource_guid, RegistryEntry(h_uncompressed, uncompressed_size, count + zip_dependency_count, false, file_type)); // moves the registry entry into the hash table
//entry = m_registry->at(resource_guid);
// entry = m_registry->at(resource_guid);
// add every file we decompress to the external dependencies list of the zip file
assert(external_resource_index < 16);
m_lock->lock(); // need to lock access to the external references array
zip_entry->m_external_references[external_resource_index++] = resource_guid;
m_lock->unlock();
m_potentially_ready->push_back(resource_guid);
	}
	zip_entry = m_registry->at(zip_hash);
	m_allocator->free(h_compressed); // free the compressed buffer

	// get the external dependencies in the zip files
	// m_dependencies_traversed_set->reset();
	m_dependencies_traversed_set->clear();
	// reads the data
	traverse_dependency_graph(zip_hash, zip_file, &ResourceManager::get_external_dependencies);
	// add this zip to an unready list
	m_potentially_ready->push_back(zip_hash);
	auto temp = m_potentially_ready->size();
}
void ResourceManager::get_external_dependencies(GUID resource, const std::string& zip_file) {
	// zip_file can be treated as the base directory
	if (!m_registry->contains(resource)) return; // if the resource is not in the registry then cant do anything here
	RegistryEntry* entry = m_registry->at(resource);
	char* p_mem = reinterpret_cast<char*>(m_allocator->get_pointer(entry->m_handle));
	switch (entry->m_file_type) {
	case FileType::ZIP:
		assert(false); // we should never get here
	case FileType::MTL: // mtls are self contained
		break;
	case FileType::OBJ:
		// OutputDebugStringA(p_mem);
		// OutputDebugStringA("\n");
		//char* p_instance = strstr(p_mem, "mtllib");
		char* p_reference = strstr(p_mem, "mtllib ") + strlen("mtllib ");
		while (p_reference - p_mem < entry->m_size) { // search entire file for external references
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
			if (m_registry->contains(dependency_hash)) {
				m_ref_count_traversed_set->reset();
				increment_reference_count(dependency_hash);
			}
			else { // if our zip file + the reference file name is not in the dictionary, then we are referencing a file that's in another zip file
				dependency_hash = std::hash<std::string>{}(reference);
				if (!m_dependency_map->contains(dependency_hash)) { // add the dependency to the dependency map if it's not in the map
					m_dependency_map->insert(dependency_hash, 1);
				}
				else { // if it's in the map, increment the dependency
					size_t* dependency_count = m_dependency_map->at(dependency_hash);
					dependency_count += 1;
				}
				// get the zip part of the reference (up until the first /)
				size_t index = reference.find('/', 0); // get first occurance of /
				// assert that the zip is not in the registry
				std::string zip(reference, 0, index);
				// add the zip to the queue
				//m_traversed_sets_stack->push(m_dependencies_traversed_set);
				size_t old_size = m_dependencies_traversed_set->size();
				//m_traversed_sets_stack->push(m_dependencies_traversed_set); // store our traversed set to the stack because unzip resources clears the set and searches for more dependencies
				m_traversed_sets_stack->emplace(m_dependencies_traversed_set);
				unzip_resource(zip.c_str()); // dont call load_resource because that is the user's function as adds to the dependency map
				m_dependencies_traversed_set = m_traversed_sets_stack->top();
				size_t new_size = m_dependencies_traversed_set->size();
				m_traversed_sets_stack->pop();
				assert(old_size == new_size); // using a stack of HashTables causes this to assert
				assert(m_dependencies_traversed_set);
			}
			// add the file to our dependency list
			add_external_dependency(resource, dependency_hash);
			// get the next dependency
			p_reference = strstr(p_reference, "mtllib ") + strlen("mtllib ");
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
	RegistryEntry* entry;
	if (m_registry->contains(resource)) {
		entry = m_registry->at(resource); // ge tthe entry if it's in the registry
		//if (m_dependencies_traversed_set->find(entry->m_external_references[0]) == m_dependencies_traversed_set->end()) {
		//	entry = m_registry->at(resource);
		//}	
		for (int i = 0; i != REFERENCE_ARRAY_SIZE && entry->m_external_references[i] != SIZE_MAX && (m_dependencies_traversed_set->find(entry->m_external_references[i]) == m_dependencies_traversed_set->end()); i++) {
		// for (int i = 0; i != REFERENCE_ARRAY_SIZE && entry->m_external_references[i] != SIZE_MAX && !(m_dependencies_traversed_set->contains(entry->m_external_references[i])); i++) {
			//m_dependencies_traversed_set->insert(entry->m_external_references[i], true);// , true); // mark the node as visited before we visit it
			m_dependencies_traversed_set->insert(entry->m_external_references[i]);// , true); // mark the node as visited before we visit it
			(this->*node_function)(entry->m_external_references[i], zip_file); // call the function on the node
			traverse_dependency_graph(entry->m_external_references[i], zip_file, node_function); // visit the node to traverse it
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
	// for each resource
	std::list<GUID> ready;
	std::list<GUID>::iterator begin;
	for (begin = m_potentially_ready->begin(); begin != m_potentially_ready->end(); begin++) { // iterate over the potentially ready list (contains zips)
		// if there are no external dependencies or 
		// if all of our external depdencies are ready then we are ready
		if (empty_external_depencies(*begin) || all_dependencies_ready(*begin)) {
			set_resource_ready(*begin);
			ready.push_back(*begin);//  m_potentially_ready->remove(*begin);
		}
	}
	for (begin = ready.begin(); begin != ready.end(); begin++) { // iterate over the ready list
		m_potentially_ready->remove(*begin);
	}
	ready.clear();
	// add it to the stack
	// if its external list is empty, set it to be ready and pop the stack
	// if there's stuff in the external list, add to stack in order

}
void ResourceManager::start_resource_thread(void) { // run a loop that tries to pop a filepath from the Queue, and then read the data into memory
	std::string zip_file;
	while (run_flag) {
		// if pop returned true then resource name has the name of the resource we need to unzip
		if (m_resource_queue->pop(zip_file)) {
			unzip_resource(zip_file);
		}
		auto temp = m_potentially_ready->size();
		if (temp > 0) {
			ready_resources();
		}
		// ready the resources
		// a resource is ready if all of it's external dependencies are in the registry, and all of its dependencies' resources are in the registry
		
	}
}
void ResourceManager::set_resource_ready(GUID resource) {
	RegistryEntry* entry = m_registry->at(resource);
	entry->m_ready = true;
}
bool ResourceManager::all_dependencies_ready(GUID resource) {
	int i = 0;
	RegistryEntry* entry = m_registry->at(resource);
	for (; i != REFERENCE_ARRAY_SIZE && entry->m_external_references[i] != SIZE_MAX; ++i) {
		if (!m_registry->contains(entry->m_external_references[i])) return false; // the dependency isnt in the registry so we are not ready
		if (!m_registry->at(entry->m_external_references[i])->m_ready) return false; // the dependency is not ready so we are not ready
	}
	return true;
}
bool ResourceManager::empty_external_depencies(GUID resource) {
	int i = 0;
	RegistryEntry* entry = m_registry->at(resource);
	for (; i != REFERENCE_ARRAY_SIZE && entry->m_external_references[i] == SIZE_MAX; ++i) {}
	return (i == REFERENCE_ARRAY_SIZE); // if we reached the end of the array then every reference is size_max so we are entirely empty
}
void ResourceManager::add_external_dependency(GUID resource, GUID dependency) {
	RegistryEntry* entry = m_registry->at(resource);
	int i = 0;
	for (; i != REFERENCE_ARRAY_SIZE && entry->m_external_references[i] != SIZE_MAX; ++i) {}
	assert(i != REFERENCE_ARRAY_SIZE);
	entry->m_external_references[i] = dependency;
}
ResourceManager::FileType ResourceManager::get_file_type(const std::string& file_name) {
	size_t index = 0;
	index = file_name.rfind("."); // find the last occurance of the . to get the file extension
	assert(index != std::string::npos); // assert that the file has an extension
	std::string extension(file_name.c_str() + index, file_name.length() - index);
	if (extension == ".zip") return FileType::ZIP;
	if (extension == ".obj") return FileType::OBJ;
	if (extension == ".mtl") return FileType::MTL;
	assert(false); // the file extension is one we dont know how to handle
}