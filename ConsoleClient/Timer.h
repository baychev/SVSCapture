#pragma once

#include <chrono>

class Timer
{
public:
	Timer();
	~Timer();
	__int64 time();
	__int64 diff(__int64 start);
};

