#include "stdafx.h"
#include "Stopwatch.h"

using namespace std;

Stopwatch::Stopwatch()
{
}


Stopwatch::~Stopwatch()
{
}

void Stopwatch::start()
{
	start_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

void Stopwatch::stop()
{
	stop_ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

void Stopwatch::reset()
{
	start_ms = 0;
	stop_ms = 0;
}

__int64 Stopwatch::time()
{
	return stop_ms - start_ms;
}