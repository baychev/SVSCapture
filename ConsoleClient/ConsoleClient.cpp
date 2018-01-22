// ConsoleClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "conio.h"
#include <iostream>
#include <string>

#include "SVSCapture.h"

using namespace std;

/*
Entry point to test the DLL

Intended purpose of this program:
- simplify access to the SVGenSDK
- get image frames from a continuous shooting mode camera
- save frames to file system
*/
int main()
{
	const char* _sn = "59992";
	SVSCapture * sv_cap = new SVSCapture(_sn);

	int width = 160;
	sv_cap->set_feature_int("Width", width);
	int height = 160;
	sv_cap->set_feature_int("Height", height);
	int exposure_microseconds = 15000;
	sv_cap->set_feature_float("ExposureTime", exposure_microseconds);
	int frame_rate = 25;
	sv_cap->set_feature_float("AcquisitionFrameRate", frame_rate);

	sv_cap->start_acquisition();

	sv_cap->print_feature_info();

	// allocate memory for image output
	unsigned char * image = new unsigned char[width * height * 3];
	printf("Image addr: %d\n", &image);

	for (int i = 0; i < 3; i++)
	{
		Sleep(200);
		__int64 start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		int res = sv_cap->get_image(image);
		__int64 end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		printf("image res: %d, time(ms): %d\n", res, end - start);

		start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		res = sv_cap->save_image(width, height, image);
		end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		printf("time to save image(ms): %d\n", end - start);
	}

	delete[] image;

	sv_cap->stop_acquisition();

	printf("Press any key to exit...");
	_getch();

	sv_cap->close();

	return 0;
}