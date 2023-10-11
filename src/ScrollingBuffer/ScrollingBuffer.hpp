#ifndef __SCROLLINGBUFFER_HPP
#define __SCROLLINGBUFFER_HPP

#include <array>
#include <cstring>
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

	uint32_t getSize() const
	{
		std::lock_guard<std::mutex> lock(mtx);
		return isFull ? maxSize : offset;
	}

	T* getFirstElement() const
	{
		std::lock_guard<std::mutex> lock(mtx);
		return &data[0];
	}
	void copyData()
	{
		std::lock_guard<std::mutex> lock(mtx);
		std::memcpy(&dataCopy[0], &data[0], maxSize * sizeof(data[0]));
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

	T getNewestValue()
	{
		std::lock_guard<std::mutex> lock(mtx);
		return data[offset > 0 ? offset - 1 : 0];
	}

	T getOldestValue()
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (isFull)
			return data[offset < maxSize ? offset : 0];
		else
			return data[0];
	}

	uint32_t getOffset() const
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

	void setMaxSize(uint32_t newMaxSize)
	{
		std::lock_guard<std::mutex> lock(mtx);
		maxSize = newMaxSize;
	}

	uint32_t getMaxSize() const
	{
		return maxSize;
	}

	uint32_t getIndexFromvalue(double value)
	{
		for (uint32_t t = 0; t < getSize(); t++)
		{
			double first = dataCopy[t];

			if ((t + 1) >= getSize())
				return getSize() - 1;

			double second = dataCopy[t + 1];
			if (value >= first && value < second)
				return t;
		}
		return 0;
	}

	std::vector<double> getLinearData(size_t startIndex, size_t stopIndex)
	{
		std::vector<double> vec;

		if (startIndex < stopIndex)
			vec.insert(data[startIndex], data[stopIndex]);
		else if (startIndex > stopIndex)
		{
			vec.insert(data[startIndex], data.end());
			vec.insert(data.begin(), data[stopIndex]);
		}

		return vec;
	}

   private:
	mutable std::mutex mtx;
	uint32_t maxSize = 10000;
	uint32_t offset = 0;
	bool isFull = false;
	static constexpr uint32_t arraySizeMax = 20000;
	mutable std::array<T, arraySizeMax> data;
	mutable std::array<T, arraySizeMax> dataCopy;
};

#endif