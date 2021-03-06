// ConsoleClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "conio.h"
#include <iostream>
#include <string>
#include "ImageUtils.h"
#include "SVCaptureAPI.h"
#include "Stopwatch.h"

using namespace std;
using namespace svs;

/*
Test for the C linkage functions.

Intended purpose of this program:
- simplify access to the SVGenSDK
- get image frames with a software trigger from camera
- save frames to the file system
*/
int main()
{
	const char* _sn = "your_serial_number";
	SVCaptureAPI * sv_cap = SVCaptureAPI_new(USB3);
	int cam_idx = SVCaptureAPI_reg_camera(sv_cap, _sn);

	// Write camera settings
	int trigger_mode = TriggerMode::ON;
	SVCaptureAPI_set_feature_enum(sv_cap, _sn, "TriggerMode", trigger_mode);
	int width = 640;
	SVCaptureAPI_set_feature_int(sv_cap, _sn, "Width", width);
	int height = 320;
	SVCaptureAPI_set_feature_int(sv_cap, _sn, "Height", height);
	int exposure_microseconds = 20000;
	SVCaptureAPI_set_feature_float(sv_cap, _sn, "ExposureTime", exposure_microseconds);

	SVCaptureAPI_start_acq(sv_cap, _sn);
	
	// This function has no C linkage!
	sv_cap->PrintFeatureInfo(_sn);

	// allocate memory for image output
	unsigned char * image = new unsigned char[width * height * 3];

	Stopwatch stopwatch = Stopwatch();

	// Ramp up camera acquisition.
	for (int i = 0; i < 3; i++)
	{
		Sleep(100);
		int res = SVCaptureAPI_get_image(sv_cap, _sn, image);
	}

	// 3 times grab an image and save as PNG
	for (int i = 0; i < 3; i++)
	{
		Sleep(200);
		stopwatch.start();
		int res = SVCaptureAPI_get_image(sv_cap, _sn, image);
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
		int res = SVCaptureAPI_get_image(sv_cap, _sn, image);

		if (res != 0)
		{
			errors += 1;
		}
	}

	printf("%d errors from %d retries at %d ms intervals.\n", errors, retries, wait_ms);

	delete[] image;

	SVCaptureAPI_stop_acq(sv_cap, _sn);

	printf("Press any key to exit...");
	_getch();

	SVCaptureAPI_close(sv_cap);

	return 0;
}