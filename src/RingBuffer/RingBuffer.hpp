#ifndef __RIGNBUFFER_HPP
#define __RIGNBUFFER_HPP

#include <array>
#include <iostream>
#include <mutex>
#include <optional>

template <typename T, size_t capacity>
class RingBuffer
{
   public:
	explicit RingBuffer() : read_idx(0), write_idx(0), size_(0) {}

	bool push(const T& item)
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (size_ == capacity)
			return false;

		buffer[write_idx] = item;
		write_idx = (write_idx + 1) % capacity;
		size_++;
		return true;
	}

	std::optional<T> pop()
	{
		std::unique_lock<std::mutex> lock(mutex);

		if (size_ == 0)
			return std::nullopt;

		T item = buffer[read_idx];
		read_idx = (read_idx + 1) % capacity;
		size_--;
		return item;
	}

	size_t size()
	{
		std::unique_lock<std::mutex> lock(mutex);
		return size_;
	}

	void clear()
	{
		while (size_)
			pop();
	}

   private:
	std::array<T, capacity> buffer;
	size_t read_idx;
	size_t write_idx;
	size_t size_;

	std::mutex mutex;
};

#endif