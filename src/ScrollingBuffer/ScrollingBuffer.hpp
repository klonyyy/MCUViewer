#ifndef __SCROLLINGBUFFER_HPP
#define __SCROLLINGBUFFER_HPP

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
		return data.size();
	}

	T* getFirstElement()
	{
		return &data[0];
	}

	int getOffset()
	{
		return offset;
	}

   private:
	static const int maxSize = 20000;
	int offset = 0;
	std::vector<T> data;
};

#endif