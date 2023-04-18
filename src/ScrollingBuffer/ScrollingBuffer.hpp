#ifndef __SCROLLINGBUFFER_HPP
#define __SCROLLINGBUFFER_HPP

#include <string.h>

#include <array>
#include <mutex>
template <typename T>
class ScrollingBuffer
{
   public:
	ScrollingBuffer() = default;
	~ScrollingBuffer() = default;

	void addPoint(T x)
	{
		std::lock_guard<std::mutex> lock(mtx);
		data[offset] = x;
		offset = (offset + 1) % maxSize;
		if (offset == 0)
			isFull = true;
	}

	int getSize()
	{
		std::lock_guard<std::mutex> lock(mtx);
		return isFull ? data.size() : offset;
	}

	T* getFirstElement() const
	{
		std::lock_guard<std::mutex> lock(mtx);
		return &data[0];
	}
	void copyData()
	{
		std::lock_guard<std::mutex> lock(mtx);
		memcpy(&dataCopy[0], &data[0], maxSize * sizeof(data[0]));
	}

	T* getFirstElementCopy() const
	{
		std::lock_guard<std::mutex> lock(mtx);
		return &dataCopy[0];
	}

	T* getLastElement()
	{
		std::lock_guard<std::mutex> lock(mtx);
		return &data[offset > 0 ? offset - 1 : 0];
	}

	int getOffset()
	{
		std::lock_guard<std::mutex> lock(mtx);
		return offset;
	}

	void erase()
	{
		std::lock_guard<std::mutex> lock(mtx);
		offset = 0;
		isFull = false;
	}

   private:
	mutable std::mutex mtx;
	static constexpr int maxSize = 10000;
	int offset = 0;
	bool isFull = false;
	mutable std::array<T, maxSize> data;
	mutable std::array<T, maxSize> dataCopy;
};

#endif