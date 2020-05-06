#include "ResourceManager.h"
#include <stdio.h>
#include "zlib.h"
#include <windows.h>

static constexpr bool RESOURCE_QUEUE_FULL = false;

void ResourceManager::load_resource(const std::string& resource_name, void(*function)(char*)) {
	// resource name is the name of the zip file in the resources folder (set by resource_path) that we need to check if it's loaded
	
	// assert that the filepath is a zip file
	// get the GUID of the resource
	size_t resource_guid = std::hash<std::string>{}(resource_name);
	// check if the resource is in the registry
if (!m_registry->contains(resource_guid)) {
	// assert here because the push must return true, if it returns false then the queue is full and we need to make it larger
	if (!m_resource_queue->push(resource_name)) assert(RESOURCE_QUEUE_FULL);
}
else {
	m_ref_count_traversed_set->reset();
	increment_reference_count(resource_guid); // another thing loaded it so we need to increment it's rerference count and the reference count of what it references
}
// if it isn't, create a queue_entry item 
// if the resource is already in the queue increment the reference count of self and all the references

}
bool ResourceManager::resource_loaded(const std::string& filepath) {
	// get the GUID of the resource
	// return true if the GUID is in the registry and it's ready
	return false;
}
void ResourceManager::remove_resource(const std::string& filepath) {
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
	RegistryEntry* entry;
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
	m_registry->insert((zip_hash), RegistryEntry(0, 0, 1, false, FileType::ZIP));

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
		m_registry->insert(resource_guid, RegistryEntry(h_uncompressed, uncompressed_size, 1, false, file_type)); // moves the registry entry into the hash table
		//entry = m_registry->at(resource_guid);
		entry = m_registry->at(zip_hash);
		// add every file we decompress to the external dependencies list of the zip file
		assert(external_resource_index < 16);
		entry->m_external_references[external_resource_index++] = resource_guid;
	}
	entry = m_registry->at(zip_hash);
	m_allocator->free(h_compressed); // free the compressed buffer

	// get the external dependencies in the zip files
	m_dependencies_traversed_set->reset();
	traverse_dependency_graph(zip_hash, &ResourceManager::get_external_dependencies);

	// traverse the graph and set it to ready if all of it's external dependencies are ready
}
void ResourceManager::get_external_dependencies(GUID resource) {
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
			GUID dependency_hash = std::hash<std::string>{}(reference);
			if (m_registry->contains(dependency_hash)) {
				m_ref_count_traversed_set->reset();
				increment_reference_count(dependency_hash);
			}
			else {
				if (!m_resource_queue->push(reference)) assert(RESOURCE_QUEUE_FULL); // add the file to the resource queue if there's space
			}
			// add the file to our dependency list
			add_external_dependency(resource, dependency_hash);
			// get the next dependency
			p_reference = strstr(p_reference, "mtllib ") + strlen("mtllib ");
		}
		break;
	}
}
void ResourceManager::traverse_dependency_graph(GUID resource, void (ResourceManager::*node_function)(GUID)){//void(*node_function)(GUID)) { // give it a function to call on every node
	// traverse the external dependency graph for the zip file
	// for each GUID in the graph
	// look inside of it for keywords of external references...
	// foreach external reference keyword (for obj file look for mtllib) make sure that the referenced file is loaded
	// add the file as an external dependency of the loaded (non-zip) file
	RegistryEntry* entry = m_registry->at(resource);
	for (int i = 0; i != REFERENCE_ARRAY_SIZE && entry->m_external_references[i] != SIZE_MAX && !(m_dependencies_traversed_set->contains(entry->m_external_references[i])); i++) {
		m_dependencies_traversed_set->insert(entry->m_external_references[i], true); // mark the node as visited before we visit it
		(this->*node_function)(entry->m_external_references[i]); // call the function on the node
		traverse_dependency_graph(entry->m_external_references[i], node_function); // visit the node to traverse it
	}
}
bool ResourceManager::get_compressed_file_info(byte* compressed, size_t& compressed_size, size_t& uncompressed_size, size_t& offset, std::string& file_name) {
	uncompressed_size = 0;
	compressed_size = 0;
	static constexpr uint32_t central_director_file_header_signature = 0x02014b50;
	static constexpr uint32_t file_header_signature = 0x04034b50;
	static constexpr size_t local_file_header_size = 30;
	uint32_t header;
	// uint16_t n = *reinterpret_cast<uint16_t*>(compressed + offset + 26);
	// uint16_t m = *reinterpret_cast<uint16_t*>(compressed + offset + 28);
	// offset += local_file_header_size + static_cast<size_t>(n) + static_cast<size_t>(m);
	// n = *reinterpret_cast<uint16_t*>(compressed + offset + 26);
	// m = *reinterpret_cast<uint16_t*>(compressed + offset + 28);
	// offset += local_file_header_size + static_cast<size_t>(n) + static_cast<size_t>(m);
	// char header[300];// = compressed;
	// for (int i = 0; i != 300; i++){
	// 	header[i] = *(compressed + offset + i);
	// }
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
void ResourceManager::start_resource_thread(void) { // run a loop that tries to pop a filepath from the Queue, and then read the data into memory
	std::string zip_file;
	while (true) {
		while (m_resource_queue->pop(zip_file)) { // if pop returned true then resource name has the name of the resource we need to unzip
			unzip_resource(zip_file);
			// the zip file in the queue was already loaded, could happen if the load calls were called before the thread had time to read the resource into memory
			
		}
	}
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