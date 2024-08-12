module;

#include <mutex>
#include <thread>
#include <functional>
#include <condition_variable>

export module runner;

// Callback function for finished job
export using RunnerFinish = std::function<void()>;

// Runner is wrapper around thread, for adding jobs to same thread, waiting for work until desctruction
export class Runner {
	using RunnerFunc = std::function<void()>;

	std::thread m_thread;
	std::mutex m_mutex;
	std::condition_variable m_cv;
	bool m_stop = false;
	std::vector<RunnerFunc> m_jobs;

public:
	Runner() : m_thread([&] {
		while (true) {
			RunnerFunc job;
			{
				std::unique_lock lock(m_mutex);
				m_cv.wait(lock, [&] { return m_stop || !m_jobs.empty(); });
				if (m_stop) break;
				job = std::move(m_jobs.front());
				m_jobs.erase(m_jobs.begin());
			}
			job();
		}
	}) {}

	~Runner() {
		{
			std::lock_guard lock(m_mutex);
			m_stop = true;
		}
		m_cv.notify_one();
		m_thread.join();
	}

	// Adds job to the queue and notifies the thread to run it, calls RunnerFinish when job is done
	auto add_job(const RunnerFunc &job, const RunnerFinish &finish = {}) -> void {
		{
			std::lock_guard lock(m_mutex);
			m_jobs.emplace_back([=] {
				job();
				if (finish) finish();
			});
		}
		m_cv.notify_one();
	}

	// Adds job and blocks until it is done
	auto add_job_sync(const RunnerFunc &job) -> void {
		std::condition_variable cv;
		std::mutex mutex;
		bool done = false;
		add_job(job, [&] {
			std::lock_guard lock(mutex);
			done = true;
			cv.notify_one();
		});
		std::unique_lock lock(mutex);
		cv.wait(lock, [&] { return done; });
	}
};
