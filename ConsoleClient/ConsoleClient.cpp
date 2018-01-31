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
Entry point to test the DLL's C linkage

Intended purpose of this program:
- simplify access to the SVGenSDK
- get image frames with a software trigger from camera
- save frames to file system
*/
int main()
{
	const char* _sn = "59992";
	SVSCapture * sv_cap = SVSCapture_new(USB3);
	int cam_idx = SVSCapture_reg_camera(sv_cap, _sn);
	SVSCapture_set_shooting_mode(sv_cap, cam_idx, SOFTWARE_TRIGGER);

	int width = 640;
	SVSCapture_set_feature_int(sv_cap, cam_idx, "Width", width);
	int height = 320;
	SVSCapture_set_feature_int(sv_cap, cam_idx, "Height", height);
	int exposure_microseconds = 15000;
	SVSCapture_set_feature_float(sv_cap, cam_idx, "ExposureTime", exposure_microseconds);

	SVSCapture_start_acq(sv_cap, cam_idx);
	
	// This function has no C linkage!
	sv_cap->PrintFeatureInfo(cam_idx);

	// allocate memory for image output
	unsigned char * image = new unsigned char[width * height * 3];

	Timer tm = Timer();

	// Ramp up camera acquisition.
	Sleep(2000);

	for (int i = 0; i < 3; i++)
	{
		Sleep(200);
		__int64 start = tm.time();
		int res = SVSCapture_get_image(sv_cap, cam_idx, image);
		printf("image res: %d, time(ms): %d\n", res, tm.diff(start));

		start = tm.time();
		string image_path = "C:\\images\\" + to_string(start) + ".png";
		SVSCapture_save_image(sv_cap, image_path.c_str(), width, height, image);
		printf("time to save image(ms): %d\n", tm.diff(start));
	}

	delete[] image;

	SVSCapture_stop_acq(sv_cap, cam_idx);

	printf("Press any key to exit...");
	_getch();

	SVSCapture_close(sv_cap);

	return 0;
}