// ConsoleClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "conio.h"
#include <iostream>
#include <string>
#include "ImageUtils.h"
#include "SVSCapture.h"
#include "Stopwatch.h"

using namespace std;
using namespace svs;

/*
Entry point to test the DLL's C linkage

Intended purpose of this program:
- simplify access to the SVGenSDK
- get image frames with a software trigger from camera
- save frames to file system
*/
int main()
{
	const char* _sn = "your_cameras_serial_number";
	SVSCapture * sv_cap = C::SVSCapture_new(USB3);
	int cam_idx = C::SVSCapture_reg_camera(sv_cap, _sn);
	C::SVSCapture_set_shooting_mode(sv_cap, cam_idx, SOFTWARE_TRIGGER);

	int width = 640;
	C::SVSCapture_set_feature_int(sv_cap, cam_idx, "Width", width);
	int height = 320;
	C::SVSCapture_set_feature_int(sv_cap, cam_idx, "Height", height);
	int exposure_microseconds = 20000;
	C::SVSCapture_set_feature_float(sv_cap, cam_idx, "ExposureTime", exposure_microseconds);

	C::SVSCapture_start_acq(sv_cap, cam_idx);
	
	// This function has no C linkage!
	sv_cap->PrintFeatureInfo(cam_idx);

	// allocate memory for image output
	unsigned char * image = new unsigned char[width * height * 3];

	Stopwatch stopwatch = Stopwatch();

	// Ramp up camera acquisition.
	for (int i = 0; i < 5; i++)
	{
		Sleep(100);
		int res = C::SVSCapture_get_image(sv_cap, cam_idx, image);
	}

	// grab 3 images
	for (int i = 0; i < 3; i++)
	{
		Sleep(200);
		stopwatch.start();
		int res = C::SVSCapture_get_image(sv_cap, cam_idx, image);
		stopwatch.stop();
		printf("image res: %d, time(ms): %d\n", res, stopwatch.time());
		stopwatch.reset();

		stopwatch.start();
		string image_path = "C:\\images\\" + to_string(stopwatch.start_ms) + ".png";
		C::SVSCapture_save_image(sv_cap, image_path.c_str(), width, height, image);
		stopwatch.stop();
		printf("time to save image(ms): %d\n", stopwatch.time());
		stopwatch.reset();
	}

	delete[] image;

	C::SVSCapture_stop_acq(sv_cap, cam_idx);

	printf("Press any key to exit...");
	_getch();

	C::SVSCapture_close(sv_cap);

	return 0;
}