#ifndef __SCROLLINGBUFFER_HPP
#define __SCROLLINGBUFFER_HPP

#include <mutex>
#include <vector>

#include "imgui.h"
#include "iostream"

template <typename T>
class ScrollingBuffer
{
   public:
	ScrollingBuffer()
	{
		data.reserve(maxSize);
	}
	~ScrollingBuffer() = default;

	void addPoint(T x)
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (data.size() < maxSize)
			data.push_back(x);
		else
		{
			data[offset] = x;
			offset = (offset + 1) % maxSize;
		}
	}

	int getSize()
	{
		std::lock_guard<std::mutex> lock(mtx);
		return data.size();
	}

	T* getFirstElement()
	{
		std::lock_guard<std::mutex> lock(mtx);
		return &data.data()[0];
	}

	int getOffset()
	{
		std::lock_guard<std::mutex> lock(mtx);
		return offset;
	}

	void erase()
	{
		std::lock_guard<std::mutex> lock(mtx);
		data.clear();
		offset = 0;
	}

   private:
	std::mutex mtx;
	static const int maxSize = 20000;
	int offset = 0;
	std::vector<T> data;
};

#endif