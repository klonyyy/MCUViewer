#ifndef _GUI_HPP
#define _GUI_HPP

#include <thread>

#include "VarReader.hpp"
#include "imgui.h"

struct ScrollingBuffer
{
	int MaxSize;
	int Offset;
	ImVector<ImVec2> Data;
	ScrollingBuffer(int max_size = 200000)
	{
		MaxSize = max_size;
		Offset = 0;
		Data.reserve(MaxSize);
	}
	void AddPoint(float x, float y)
	{
		if (Data.size() < MaxSize)
			Data.push_back(ImVec2(x, y));
		else
		{
			Data[Offset] = ImVec2(x, y);
			Offset = (Offset + 1) % MaxSize;
		}
	}
	void Erase()
	{
		if (Data.size() > 0)
		{
			Data.shrink(0);
			Offset = 0;
		}
	}
};
class Gui
{
   public:
	Gui();
	~Gui();

   private:
	VarReader* vals;
	std::thread threadHandle;
	std::thread dataHandle;

	ScrollingBuffer sdata1, sdata2;
	float t = 0;
	bool done = false;

	void mainThread();
	void drawMenu();
	void dataHandler();
};

#endif