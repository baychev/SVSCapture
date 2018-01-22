// ConsoleClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "conio.h"
#include <iostream>
#include <string>

#include "SVSCapture.h"
#include "Timer.h"

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
	SVSCapture * sv_cap = new SVSCapture(USB3);
	int cam_idx = sv_cap->register_camera(_sn);
	sv_cap->set_shooting_mode(cam_idx, SOFTWARE_TRIGGER);

	int width = 640;
	sv_cap->set_feature_int(cam_idx, "Width", width);
	int height = 320;
	sv_cap->set_feature_int(cam_idx, "Height", height);
	int exposure_microseconds = 15000;
	sv_cap->set_feature_float(cam_idx, "ExposureTime", exposure_microseconds);

	sv_cap->start_acquisition(cam_idx);
	sv_cap->print_feature_info(cam_idx);

	// allocate memory for image output
	unsigned char * image = new unsigned char[width * height * 3];

	Timer tm = Timer();

	for (int i = 0; i < 3; i++)
	{
		Sleep(200);
		__int64 start = tm.time();
		int res = sv_cap->get_image(cam_idx, image);
		printf("image res: %d, time(ms): %d\n", res, tm.diff(start));

		start = tm.time();
		res = sv_cap->save_image(width, height, image);
		printf("time to save image(ms): %d\n", tm.diff(start));
	}

	delete[] image;

	sv_cap->stop_acquisition(cam_idx);

	printf("Press any key to exit...");
	_getch();

	sv_cap->close();

	return 0;
}