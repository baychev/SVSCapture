#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include "ImageUtils.h"

namespace utils
{
	int save_image(const char * file_name, int width, int height, unsigned char * im_buffer)
	{
		cv::Mat image(cv::Size((unsigned int)width, (unsigned int)height), CV_8UC3, im_buffer, cv::Mat::AUTO_STEP);

		try
		{
			cv::imwrite(file_name, image);
		}
		catch (cv::Exception & e)
		{
			cerr << e.msg << endl;
		}

		return 0;
	}
}