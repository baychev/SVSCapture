#pragma once

#include <chrono>

class Stopwatch
{
public:
	Stopwatch();
	~Stopwatch();
	void start();
	void stop();
	void reset();
	__int64 time();
	__int64 start_ms;
	__int64 stop_ms;
};

