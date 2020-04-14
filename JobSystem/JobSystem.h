#pragma once
#include <thread>
//#include "../ThreadSafeContainers/concurrentqueue.h"
#include "../ThreadSafeContainers/Queue.h"

struct alignas (64) Job {
	void (*m_function) (Job*, const void*);
	Job* m_parent;
	std::atomic<int32_t> m_unfinished_jobs;
	uint8_t m_data[44];
	Job() = default;
};
namespace JobSystem { // cant be namespace because the variables have to be static
	typedef void (*JobFunction) (Job*, const void*);
	void startup(uint8_t);
	void shutdown();
	void start_thread(uint8_t);
	Job* create_job(JobFunction);
	Job* create_job(JobFunction, const void*);
	Job* create_job_as_child(JobFunction, Job*);
	Job* create_job_as_child(JobFunction, Job*, const void*);
	void finish(Job*);
	Job* allocate_job();
	//moodycamel::ConcurrentQueue<Job*>* get_worker_thread_queue();
	//moodycamel::ConcurrentQueue<Job*>* get_worker_thread_queue(int);
	Queue<Job*>* get_worker_thread_queue();
	Queue<Job*>* get_worker_thread_queue(int);
	void queue_job(Job*);
	bool job_completed(const Job*);
	void execute_job(Job*);
	bool is_empty_job(Job*);
	Job* get_job();
	void wait(const Job*);
	struct Worker {
		Queue<Job*>* m_queue;
		std::thread* m_thread;
		explicit Worker() : m_queue(new Queue<Job*>(1024)), m_thread() {}
		~Worker() {
			delete m_thread;
			delete m_queue;
		}
	};
	namespace { // anonymous namespace to make the members private. intended to stop other translation units from trying to access the static members
		const uint32_t G_MAX_JOB_COUNT = 1024 * 1024;
		const uint32_t G_MAX_JOB_MASK = G_MAX_JOB_COUNT - 1u;
		static uint8_t g_worker_count;
		static bool shutdown_flag;
		static thread_local Job g_job_buffer[G_MAX_JOB_COUNT];
		static thread_local uint32_t g_allocated_jobs = 0;
		static thread_local int8_t id;
		static Worker* m_workers;
	}

	
}
class CountSplitter {
public:
	explicit CountSplitter(uint32_t count) : m_count(count) {}
	template <typename T>
	inline bool split(unsigned int count) const {
		return (count > m_count);
	}
private:
	uint32_t m_count;
};
template <typename T, typename S>
struct parallel_for_job_data {
	typedef S SplitterType;
	typedef T DataType;
	T* m_data; // container we are running the parallel for on
	uint32_t m_count;
	void (*m_function)(T*, size_t);
	SplitterType m_splitter;
	parallel_for_job_data(T* data, uint32_t count, void (*function)(T*, size_t), SplitterType splitter) : m_data(data), m_count(count), m_function(function), m_splitter(splitter) {}
};
// TODO: convert sizes from uint32_t to size_t
template <typename JobData>
void parallel_for_job(Job* job, const void* job_data) {
	const JobData* data = static_cast<const JobData*>(job_data);
	const JobData::SplitterType& splitter = data->m_splitter;
	//if (data->m_count > data->m_split_size) {
	if(splitter.split<JobData::DataType>(data->m_count)) {
		const uint32_t left_count = data->m_count / 2u;
		const uint32_t right_count = data->m_count - left_count;
		const JobData left_data(data->m_data, left_count, data->m_function, splitter);
		const JobData right_data(data->m_data + left_count, right_count, data->m_function, splitter);
		Job* left = JobSystem::create_job_as_child(parallel_for_job<JobData>, job, &left_data);
		Job* right = JobSystem::create_job_as_child(parallel_for_job<JobData>, job, &right_data);
		JobSystem::queue_job(left);
		JobSystem::queue_job(right);
	}
	else {
		(data->m_function)(data->m_data, data->m_count);
	}

}
template <typename T, typename S>
Job* parallel_for(T* data, uint32_t count, void (*function)(T*, size_t), const S& splitter) {
	typedef parallel_for_job_data<T, S> JobData;
	const JobData* job_data = new JobData(data, count, function, splitter);
	Job* job = JobSystem::create_job(parallel_for_job<JobData>, job_data); // create a job that runs the splitting function
	return job;
}

