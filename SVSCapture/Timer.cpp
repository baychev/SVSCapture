#include "stdafx.h"
#include "Timer.h"

using namespace std;

Timer::Timer()
{
}


Timer::~Timer()
{
}

__int64 Timer::time()
{
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

__int64 Timer::diff(__int64 start)
{
	_int64 end = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
	return end - start;
}
