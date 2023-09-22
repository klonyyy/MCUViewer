#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <vector>

using std::chrono::operator""ms;

template <typename T>
class RingBuffer
{
   public:
	explicit RingBuffer(size_t capacity) : buffer(capacity), capacity(capacity), read_idx(0), write_idx(0), size(0) {}

	bool push(const T& item)
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (!cond_full.wait_for(lock, 100ms, [this]() { return size < capacity; }))
			return false;

		buffer[write_idx] = item;
		write_idx = (write_idx + 1) % capacity;
		size++;

		cond_empty.notify_one();
		return true;
	}

	T pop()
	{
		std::unique_lock<std::mutex> lock(mutex);
		cond_empty.wait(lock, [this]() { return size > 0; });

		T item = buffer[read_idx];
		read_idx = (read_idx + 1) % capacity;
		size--;

		cond_full.notify_one();

		return item;
	}

	size_t getSize()
	{
		std::unique_lock<std::mutex> lock(mutex);
		return size;
	}

   private:
	std::vector<T> buffer;
	size_t capacity;
	size_t read_idx;
	size_t write_idx;
	size_t size;

	std::mutex mutex;
	std::condition_variable cond_empty;
	std::condition_variable cond_full;
};