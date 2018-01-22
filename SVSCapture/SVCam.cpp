#include "stdafx.h"
#include <iostream>
#include "SVCam.h"

SVCam::SVCam(string sn)
{
	serial_number = sn;
}

SVCam::~SVCam()
{
	std::cout << "called destructor." << std::endl;
}

int SVCam::get_image(char * buffer)
{
	char* img = NULL;
	buffer = img;
	return 0;
}

void SVCam::init()
{
	start_acq();
}

void SVCam::close()
{
	stop_acq();
}

void SVCam::start_acq()
{
}

void SVCam::stop_acq()
{
}

char* SVCam::test()
{
	return NULL;
}