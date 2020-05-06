#include <algorithm>
#include <assert.h>
#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <string>
#include "JobSystem/JobSystem.h"
#include "InputManager/InputManager.h"
#include "Utilities/Utilities.h"
#include <windows.h>
#include "Math/Matrix.h"
#include "Math/Line.h"
#include "Math/Plane.h"
#include <malloc.h>
#include <stdint.h>
#include "Memory/MemoryManager.h"
#include "Memory/GeneralAllocator.h"
#include "ThreadSafeContainers/HashTable.h"


#include "ResourceManager/ResourceManager.h"



#include <stdio.h>
#include "zlib.h"


const int thread_count = 3; //std::min((unsigned int) 3, std::thread::hardware_concurrency() - 1);
//LinearAllocator linear_allocator(1024);

struct Particles {
	float x;
	float y;
	float z;
	////uint8_t padding[52];
	Particles() : x(0.0f), y(0.0f), z(0.0f) {}
};

std::atomic<uint64_t> sum;

void empty_job(Job* job, const void* data) { // we pass job incase we create a job we can set it's parent to the job argument
	// std::cout << std::this_thread::get_id() << std::endl;
	// std::this_thread::sleep_for(std::chrono::milliseconds(100));
	sum.fetch_add(1);
}

void particle_job(Particles* p_p, const size_t count) {
	for (int i = 0; i != count; ++i) {
		p_p[i].x = static_cast<float>(rand());
		p_p[i].y = static_cast<float>(rand());// * ::JobSystem::id;
		p_p[i].z = static_cast<float>(rand());// * ::JobSystem::id;
	}
	// Job* root = JobSystem::create_job(empty_job);
	// for (int i = 0; i != 10; i++) {
	// 	Job* job = JobSystem::create_job_as_child(empty_job, root);
	// 	JobSystem::queue_job(job);
	// }
	// JobSystem::queue_job(root);
	// JobSystem::wait(root);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


template<typename T>
size_t size_of(T ptr) {
	return sizeof(*ptr);
}


bool get_compressed_file_info(byte* compressed, size_t& compressed_size, size_t& uncompressed_size, size_t& offset, std::string& file_name) {
	uncompressed_size = 0;
	compressed_size = 0;
	static constexpr uint32_t central_director_file_header_signature = 0x02014b50;
	static constexpr size_t local_file_header_size = 30;
	if (*reinterpret_cast<uint32_t*>(compressed + offset) != central_director_file_header_signature) { // still have compressed data to uncompress
		compressed_size = *reinterpret_cast<uint32_t*>(compressed + offset + 18);
		uncompressed_size = *reinterpret_cast<uint32_t*>(compressed + offset + 22);
		uint16_t n = *reinterpret_cast<uint16_t*>(compressed + offset + 26);
		uint16_t m = *reinterpret_cast<uint16_t*>(compressed + offset + 28);

		std::string name(reinterpret_cast<char*>(compressed + offset + 30), n);
		file_name = name; // copy the name into the reference;
		offset += local_file_header_size + static_cast<size_t>(n) + static_cast<size_t>(m); // offset returns the start of the compressed data
		return true;
	}
	return false;
}


// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hprevinstance, LPSTR lpcmdline, int nCmdShow) {
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR pCmdLine, _In_ int nCmdShow) {
	//HINSTANCE hInstance = (HINSTANCE) GetModuleHandle(NULL);
	const wchar_t CLASS_NAME[] = L"Sample Window Class";
	WNDCLASS wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);
	HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Engine", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);
	if (hwnd == NULL) {
		return 99999;
	}
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	byte* test = NEW_ARRAY(byte, 380, memory_manager->get_general_allocator());// reinterpret_cast<byte*>(memory_manager->get_general_allocator()->allocate(uncompressed_size, 1));
	int handle = memory_manager->get_general_allocator()->register_allocated_mem(test);
	memory_manager->free(handle);


	//JobSystem::startup(thread_count);

	ResourceManager rm;

	//HashTable<int, int> t;
	//t.insert(9, -9);
	//t.insert(9, -9);
	//t.insert(9, -9);
	//t.insert(9, -9);
	//t.insert(9, -9);
	//assert(t.at(9) == -9);
	//t.set(9, 8);
	//assert(t.at(9) == 8);
	//assert(t.contains(9));
	//t.remove(9);
	//assert(!t.contains(9));
	//t.reset();

	const size_t CHUNK = 1 << 16;

	int ret = 0;

	// byte* i = NEW(byte, memory_manager->get_general_allocator())(0x78);
	// int handle = memory_manager->get_general_allocator()->register_allocated_mem(i);
	// memory_manager->free(handle);
	// std::string path = "C:\\Users\\Seth Eisner\\source\\repos\\Engine\\Resources\\Miami_Sample.zip";
	
	
	rm.load_resource("CyberCity3D_Sample_OBJ.zip");
	
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	};
	// std::string path = "C:/Users/Seth Eisner/source/repos/Engine/Resources/s";
	// FILE* fp = fopen(path.c_str(), "rb");
	// assert(fp != nullptr);
	// fseek(fp, 0, SEEK_END);
	// size_t file_length = ftell(fp);
	// rewind(fp);
	// byte* compressed = (byte*)malloc(file_length);// NEW_ARRAY(byte, file_length, m_allocator); // allocate space for the compressed memory from our general allocator
	// //int h_compressed = m_allocator->register_allocated_mem(compressed);
	// fread(compressed, file_length, 1, fp); // read the file into memory
	// fclose(fp);
	// Bytef* uncompressed = (Bytef*)malloc(22); // i think 22 bytes is the minimum size if it's just a central directory header
	// AllocConsole();
	// freopen("CONOUT$", "w", stdout);
	// size_t compressed_size;
	// size_t uncompressed_size;
	// 
	// while (get_compressed_file_info(compressed, compressed_size, uncompressed_size, offset)) {
	// 	byte* uncompressed = NEW_ARRAY(byte, uncompressed_size, memory_manager->get_general_allocator());// reinterpret_cast<byte*>(memory_manager->get_general_allocator()->allocate(uncompressed_size, 1));
	// 	assert(uncompressed != nullptr);
	// 	//byte* uncompressed = (byte*)malloc(uncompressed_size + 1);
	// 	int handle = memory_manager->get_general_allocator()->register_allocated_mem(uncompressed);
	// 	z_stream infstream;
	// 	infstream.zalloc = Z_NULL;
	// 	infstream.zfree = Z_NULL;
	// 	infstream.opaque = Z_NULL;
	// 	infstream.avail_in = compressed_size;//(uInt)((unsigned char*)defstream.next_out - b); // size of input
	// 	infstream.next_in = compressed + offset; // input char array
	// 	infstream.avail_out = uncompressed_size; // size of output
	// 	infstream.next_out = uncompressed; // output char array
	// 	inflateInit2(&infstream, -15); // inflateInit2 skips the headers and looks for a raw stream 
	// 	ret = inflate(&infstream, Z_NO_FLUSH);
	// 	inflateEnd(&infstream);
	// 	//OutputDebugStringA(reinterpret_cast<LPCSTR>(uncompressed));
	// 	//uncompressed[uncompressed_size] = '\0';
	// 	for (int i = 0; i < uncompressed_size; i += (1 << 12)) {
	// 		OutputDebugStringA(reinterpret_cast<LPCSTR>(uncompressed + i));
	// 	}
	// 	OutputDebugStringA("\n");
	// 	memory_manager->get_general_allocator()->free(handle); // the free function throws the exception, i dont think the sizes are causing the headers to be set properly in allocate function
	// 	offset += compressed_size;
	// }


	

	// the actual DE-compression work.
	
	//ret = uncompress(uncompressed, &size, compressed, compressed_size);
	//ret = inflate(&strm, Z_SYNC_FLUSH);
	//ret = uncompress(uncompressed, &length, compressed, file_length);
	assert(ret >= 0);
	// for (int i = 0; i != uncompressed_size; i++) {
	// 	putchar(uncompressed[i]);
	// }
	// fp = nullptr;

	// for (int i = 0; i != length; i++) {
	// 	OutputDebugStringA(reinterpret_cast<LPCSTR>(uncompressed + i));
	// }

	//GeneralAllocator allocator(1024);
	// int* my_int = NEW(int, memory_manager->get_general_allocator()) (8);
	// Handle handle = memory_manager->get_general_allocator()->register_allocated_mem(my_int);
	// int* temp = reinterpret_cast<int*>(memory_manager->get_general_allocator()->get_pointer(handle));
	// *temp = 10;
	// memory_manager->free(handle);
	// memory_manager->defragment();
	//int  h = allocator.allocate(32, 8);
	//int  i = allocator.allocate(32, 8);
	//int  j = allocator.allocate(32, 8);
	//int  k = allocator.allocate(32, 8);
	//int  l = allocator.allocate(32, 8);
	//allocator.free(b);
	//allocator.free(k);
	//allocator.free(d);
	//allocator.free(i);
	//allocator.free(h);
	
	
	/* InputManager* input_manager = new InputManager();
	
	input_manager->add_action(HASH("shoot"), InputManager::MouseButton::LEFT);
	input_manager->add_action(HASH("jump"), InputManager::Key(' '));
	
	while (!input_manager->is_held(HASH("shoot"))) {
		input_manager->get_input();
		std::this_thread::sleep_for(std::chrono::milliseconds(33));
		if (input_manager->is_released(HASH("shoot"))) {
			OutputDebugStringA("released\n");
		}
		if (input_manager->is_held(HASH("shoot"))) {
			OutputDebugStringA("held\n");
		}
		if (input_manager->is_pressed(HASH("shoot"))) {
			OutputDebugStringA("pressed\n");
		}
		if (input_manager->is_unheld(HASH("shoot"))) {
			OutputDebugStringA("unheld\n");
		}
	}
	
	input_manager->remap_action(HASH("shoot"), InputManager::MouseButton::RIGHT);
	while (true) {
		input_manager->get_input();
		std::this_thread::sleep_for(std::chrono::milliseconds(33));
		if (input_manager->is_released(HASH("shoot"))) {
			OutputDebugStringA("released\n");
		}
		if (input_manager->is_held(HASH("shoot"))) {
			OutputDebugStringA("held\n");
		}
		if (input_manager->is_pressed(HASH("shoot"))) {
			OutputDebugStringA("pressed\n");
		}
		if (input_manager->is_unheld(HASH("shoot"))) {
			OutputDebugStringA("unheld\n");
		}
	}*/
	

	/*
	Job* root = JobSystem::create_job(empty_job);
	const uint32_t n_particles = 100000;
	Particles* parts_1 = new Particles[n_particles]; // allocate 100000 particles
	Particles* parts_2 = new Particles[n_particles];
	Particles* parts_3 = new Particles[n_particles];
	Particles* parts_4 = new Particles[n_particles];
	// 
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	// 
	Job* job_1 = parallel_for(parts_1, n_particles, particle_job, CountSplitter(1024));
	Job* job_2 = parallel_for(parts_2, n_particles, particle_job, CountSplitter(1024));
	Job* job_3 = parallel_for(parts_3, n_particles, particle_job, CountSplitter(1024));
	Job* job_4 = parallel_for(parts_4, n_particles, particle_job, CountSplitter(1024));
	JobSystem::queue_job(job_1);
	JobSystem::queue_job(job_2);
	JobSystem::queue_job(job_3);
	JobSystem::queue_job(job_4);
	
	// parallel_for is much faster if we do work after queuing the jobs -> the entire point of the job system is to exploit this data parallelism 
	uint64_t sum = 1;
	for (size_t i = 0; i != n_particles * 100; ++i) {
		sum += rand();
	}
	 
	JobSystem::wait(job_1);
	JobSystem::wait(job_2);
	JobSystem::wait(job_3);
	JobSystem::wait(job_4);
	// make happen at end of frame for every parallel_for job...
	// FREE(job_1->m_data, memory_manager->get_pool_allocator(32));
	// FREE(job_2->m_data, memory_manager->get_pool_allocator(32));
	// FREE(job_3->m_data, memory_manager->get_pool_allocator(32));
	// FREE(job_4->m_data, memory_manager->get_pool_allocator(32));


	// for (int i = 0; i != n_particles; i++) {
	// 	Job* job = JobSystem::create_job_as_child(empty_job, root);
	// 	JobSystem::queue_job(job);
	// }
	// JobSystem::queue_job(root);
	// JobSystem::wait(root);
	// 
	*/

	// std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	// std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
	// std::cout << time_span.count() * 1000000 << " microseconds.\n";
	// std::cout << time_span.count() * 1000 << " milliseconds.\n";
	// std::string time(std::to_string(time_span.count() * 1000) + '\n');
	// OutputDebugStringA(time.c_str());
	JobSystem::shutdown();

	return 0;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		exit(0);
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);
	}
	return 0;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}