#pragma once

#include <iostream>

using namespace std;

namespace utils
{
	struct Point
	{
		int x;
		int y;
	};

	struct BoundingBox
	{
		Point top_left;
		Point bottom_right;
	};

	struct RGBThreshold
	{
		unsigned int min_R;
		unsigned int max_R;
		unsigned int min_G;
		unsigned int max_G;
		unsigned int min_B;
		unsigned int max_B;
	};

	int get_roi(unsigned char * im_buffer);
	int save_image(const char * file_name, int width, int height, unsigned char *im_buffer);
}

