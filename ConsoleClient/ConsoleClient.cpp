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
	const char* _sn = "59992";
	SVSCapture * sv_cap = SVSCapture_new(USB3);
	int cam_idx = SVSCapture_reg_camera(sv_cap, _sn);

	// Read camera settings
	//const char * setting_1 = SVSCapture_get_feature_str(sv_cap, _sn, "TriggerMode");
	double setting_2 = SVSCapture_get_feature_float(sv_cap, _sn, "ExposureTime");
	int setting_3 = SVSCapture_get_feature_int(sv_cap, _sn, "Width");

	int shooting_mode = SOFTWARE_TRIGGER;
	SVSCapture_set_feature_enum(sv_cap, _sn, "TriggerMode", shooting_mode);
	int width = 640;
	SVSCapture_set_feature_int(sv_cap, _sn, "Width", width);
	int height = 320;
	SVSCapture_set_feature_int(sv_cap, _sn, "Height", height);
	int exposure_microseconds = 20000;
	SVSCapture_set_feature_float(sv_cap, _sn, "ExposureTime", exposure_microseconds);

	SVSCapture_start_acq(sv_cap, _sn);
	
	// This function has no C linkage!
	sv_cap->PrintFeatureInfo(_sn);

	// allocate memory for image output
	unsigned char * image = new unsigned char[width * height * 3];

	Stopwatch stopwatch = Stopwatch();

	// Ramp up camera acquisition.
	for (int i = 0; i < 5; i++)
	{
		Sleep(100);
		int res = SVSCapture_get_image(sv_cap, _sn, image);
	}

	// 3 times grab an image and save as PNG
	for (int i = 0; i < 3; i++)
	{
		Sleep(200);
		stopwatch.start();
		int res = SVSCapture_get_image(sv_cap, _sn, image);
		stopwatch.stop();
		printf("image res: %d, time(ms): %d\n", res, stopwatch.time());
		stopwatch.reset();

		stopwatch.start();
		string image_path = "C:\\images\\" + to_string(stopwatch.start_ms) + ".png";
		utils::save_image(image_path.c_str(), width, height, image);
		stopwatch.stop();
		printf("time to save image(ms): %d\n", stopwatch.time());
		stopwatch.reset();
	}

	// availability test
	printf("Performing availability test...\n");
	int retries = 100;
	int errors = 0;
	int wait_ms = 50;
	for (int i = 0; i < retries; i++)
	{
		Sleep(wait_ms);
		int res = SVSCapture_get_image(sv_cap, _sn, image);

		if (res != 0)
		{
			errors += 1;
		}
	}

	printf("%d errors from %d retries at %d ms intervals.\n", errors, retries, wait_ms);

	// unnecessary, but be explicit
	delete[] image;

	SVSCapture_stop_acq(sv_cap, _sn);

	printf("Press any key to exit...");
	_getch();

	SVSCapture_close(sv_cap);

	return 0;
}