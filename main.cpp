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
//#include "Memory/MemoryManager.h"

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

	JobSystem::startup(thread_count);
	//memory_manager.initialize();
	
	// int* a = NEW_ARRAY(int, 10, linear_allocator);
	// for (int i = 0; i != 10; i++) {
	// 	*(a + i) = i;
	// }
	// for (int i = 0; i != 10; i++) {
	// 	int q = a[i];
	// }

	int* a = NEW(int, memory_manager.get_stack_allocator())(1);
	int* b = NEW(int, memory_manager.get_stack_allocator())(2);
	int* c = NEW(int, memory_manager.get_stack_allocator())(3);
	int* d = NEW(int, memory_manager.get_stack_allocator())(4);

	struct temp {
		union {
			int* iptr;
			size_t size;
		};
		std::atomic<int> atomic_int;
	};
	

	size_t i = sizeof(std::atomic<int>);
	int i_array[40];
	int f = 9;
	temp* e = new temp;
	size_t size_0 = sizeof(*e);
	size_t size_1 = size_of(i_array);

	FREE(d, memory_manager.get_stack_allocator());
	FREE(c, memory_manager.get_stack_allocator());
	FREE(b, memory_manager.get_stack_allocator());
	FREE(a, memory_manager.get_stack_allocator());
	std::cout << std::endl;

	size_t size = 4;
	uint16_t count = 14;
	uint8_t* m_pool = static_cast<uint8_t*>(_aligned_malloc(count, size));
	assert(size >= 2); //
	for (int i = 0; i != count * size; i += size) {
		uint8_t upper = (size >> 8) & 0xFF;
		*(m_pool + i) = upper; // store upper byte of offset
		uint8_t lower = size & 0xFF;
		*(m_pool + i + 1) = lower; // store lower byte of offset
	}
	for (int i = 0; i != count * size; i += size) {
		uint16_t upper = (*(m_pool + i) << 8);// = static_cast<uint8_t>((size >> 8) & 0x00FF); // store upper byte of offset
		uint8_t lower = *(m_pool + i + 1);// = static_cast<uint8_t>((size & 0x00FF) >> 8); // store lowe byte of offset
		uint16_t c = upper + lower;
		uint16_t d = 0;
	}


	// PoolAllocator pool(4, (1u<<16) - 1);
	// int* object_0 = static_cast<int*>(pool.allocate());
	// int* object_1 = static_cast<int*>(pool.allocate());
	// *object_0 = 100;
	// pool.free(object_1);
	// object_1 = static_cast<int*>(pool.allocate());
	// *object_0 = 500;
	// int* object_2 = static_cast<int*>(pool.allocate());
	// *object_2 = 300;
	// int h = 9999;

	//while (true) {}
	// Mat4 m1;
	// Mat4 m2(1, 3, 5, 9, 1, 3, 1, 7, 4, 3, 9, 7, 5, 2, 0, 9);
	// 
	// Point3 p(0, 0, 1);
	// Point3 q(1, 0, 0);
	// Line l(p, q);
	// 
	// p.x = 1;
	
	//LinearAllocator alloc(1024); // create linear alocator 1ith 1kB of memory
	// int* a = static_cast<int*>(linear_allocator.allocate_aligned(sizeof(int), alignof(int)));
	// *a = 10;
	// linear_allocator.reset();
	// std::cout << a << std::endl;
	// std::cout << std::endl;

	// Mat4 ortho = orthogonalize(m2);
	// float f1 = det(m1);
	// float f2 = det(m2);
	// Mat4 m3 = transpose(m2);
	// Mat4 m4 = inverse(m2);
	// 
	// Trans4 m0 = make_rotation_z(1.5707f); //pi/4 radians

	// Quaternion q;
	// Vec3 v(0.0f, 0.0f, 3.0f);

	//q.set_rotation_matrix(make_rotation_z(radians(90)));
	// Quaternion qp(Vec3(0, 0, 1), radians(90));
	// Quaternion qc(Vec3(1, 0, 0), radians(90));
	// Vec3 res = transform(v, qp * qc); // performs parent then child rotation
	// Vec3 res1 = transform(v, q * q); // performs two rotations of zero degrees
	// float x = magnitude(qp * qc);

	// Trans4 t = make_scale(3.0f);
	// float f = det(t);
	//std::cout << time_span.count() * 1000000 << " microseconds.\n";
	// std::cout << time_span.count() * 1000 << " milliseconds.\n";
	// InputManager* input_manager = new InputManager();
	// 
	// input_manager->add_action(HASH("shoot"), InputManager::MouseButton::LEFT);
	// input_manager->add_action(HASH("jump"), InputManager::Key(' '));
	// 
	// while (!input_manager->is_held(HASH("shoot"))) {
	// 	input_manager->get_input();
	// 	std::this_thread::sleep_for(std::chrono::milliseconds(33));
	// 	if (input_manager->is_released(HASH("shoot"))) {
	// 		OutputDebugStringA("released\n");
	// 	}
	// 	if (input_manager->is_held(HASH("shoot"))) {
	// 		OutputDebugStringA("held\n");
	// 	}
	// 	if (input_manager->is_pressed(HASH("shoot"))) {
	// 		OutputDebugStringA("pressed\n");
	// 	}
	// 	if (input_manager->is_unheld(HASH("shoot"))) {
	// 		OutputDebugStringA("unheld\n");
	// 	}
	// }
	// 
	// input_manager->remap_action(HASH("shoot"), InputManager::MouseButton::RIGHT);
	// while (true) {
	// 	input_manager->get_input();
	// 	std::this_thread::sleep_for(std::chrono::milliseconds(33));
	// 	if (input_manager->is_released(HASH("shoot"))) {
	// 		OutputDebugStringA("released\n");
	// 	}
	// 	if (input_manager->is_held(HASH("shoot"))) {
	// 		OutputDebugStringA("held\n");
	// 	}
	// 	if (input_manager->is_pressed(HASH("shoot"))) {
	// 		OutputDebugStringA("pressed\n");
	// 	}
	// 	if (input_manager->is_unheld(HASH("shoot"))) {
	// 		OutputDebugStringA("unheld\n");
	// 	}
	// }

	// sum.store(0);
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
	FREE(job_1->m_data, memory_manager.get_pool_allocator(32));
	FREE(job_2->m_data, memory_manager.get_pool_allocator(32));
	FREE(job_3->m_data, memory_manager.get_pool_allocator(32));
	FREE(job_4->m_data, memory_manager.get_pool_allocator(32));
	// for parallel_for jobs, m_data points to dynamically allocated memory. need to make sure we give that back to the pool allocator when we are done with it
	
	// particle_job(parts_1, n_particles);
	// particle_job(parts_2, n_particles);
	// particle_job(parts_3, n_particles);
	// particle_job(parts_4, n_particles);

	// Queue<int>* a = NEW(Queue<int>, linear_allocator)(16);
	// a->push(0);
	// a->push(1);
	// a->push(2);
	// a->push(3);
	// a->push(4);
	// a->push(5);
	// a->push(6);
	// a->push(7);
	// a->push(8);
	// a->push(9);
	// a->push(10);
	// 
	// int _i;
	// a->pop(_i);
	// a->pop(_i);
	// a->pop(_i);
	// a->pop(_i);
	// a->pop(_i);
	// a->pop(_i);
	// a->pop(_i);


	// for (int i = 0; i != n_particles; i++) {
	// 	Job* job = JobSystem::create_job_as_child(empty_job, root);
	// 	JobSystem::queue_job(job);
	// }
	// JobSystem::queue_job(root);
	// JobSystem::wait(root);
	//  
	// int sum_ = sum.load();

	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
	//std::cout << time_span.count() * 1000000 << " microseconds.\n";
	//std::cout << time_span.count() * 1000 << " milliseconds.\n";
	std::string time(std::to_string(time_span.count() * 1000) + '\n');
	OutputDebugStringA(time.c_str());
	JobSystem::shutdown();
	//memory_manager.shutdown(); // this needs to be one of the last things we shutdown
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