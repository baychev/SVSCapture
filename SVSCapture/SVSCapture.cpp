// SVSCapture.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "conio.h"
#include "SVSCapture.h"

SVSCapture::SVSCapture(LibraryType libType)
{
	int res = init_library(libType);

	if (res < 0)
		throw std::invalid_argument("SVS Vistek SDK not initialized!");
}

int SVSCapture::init_library(LibraryType libType)
{
	char SVGenicamRoot[1024] = { 0 };
	char SVGenicamCache[1024] = { 0 };
	char SVSCti[1024] = { 0 };
	char SVCLProtocol[1024] = { 0 };

	DWORD dwRet = GetEnvironmentVariableA("SVS_GENICAM_ROOT", SVGenicamRoot, sizeof(SVGenicamRoot));
	if (0 == dwRet)
		return ERR_SVS_GENICAM_ROOT;

	dwRet = GetEnvironmentVariableA("SVS_SDK_GENTL", SVSCti, sizeof(SVSCti));
	if (0 == dwRet)
		return ERR_SVS_SDK_GENTL;

	dwRet = GetEnvironmentVariableA("SVS_GENICAM_CLPROTOCOL", SVCLProtocol, sizeof(SVCLProtocol));
	if (0 == dwRet)
		return ERR_SVS_GENICAM_CLPROTOCOL;

	dwRet = GetEnvironmentVariableA("SVS_GENICAM_CACHE", SVGenicamCache, sizeof(SVCLProtocol));
	if (0 == dwRet)
		return ERR_SVS_GENICAM_CACHE;

	// load DLLs and intialize device modules
	SV_RETURN  ret = SVLibInit(SVSCti, SVGenicamRoot, SVGenicamCache, SVCLProtocol);
	if (ret != SV_ERROR_SUCCESS)
		return ERR_SVLIB_INIT;

	SV_TL_TYPE  tl_type;

	switch (libType)
	{
	case USB3:
		tl_type = TL_U3V;
		break;
	case GIG_E:
		tl_type = TL_GEV;
		break;
	case CAMERA_LINK:
		tl_type = TL_CL;
		break;
	}

	sv_cam_sys = new SVCamSystem(tl_type);

	// open the system module
	uint32_t tlCount = 0;
	ret = SVLibSystemGetCount(&tlCount);
	bool State = false;

	// initialize device and get transport layer info
	for (uint32_t i = 0; i < tlCount; i++)
	{
		SV_TL_INFO tlInfo = { 0 };
		ret = SVLibSystemGetInfo(i, &tlInfo);
		if (SV_ERROR_SUCCESS != ret)
		{
			continue;
		}

		if ((TL_GEV == tl_type && 0 == _stricmp("GEV", tlInfo.tlType)) ||
			(TL_U3V == tl_type && 0 == _stricmp("U3V", tlInfo.tlType)) ||
			(TL_CL == tl_type && 0 == _stricmp("CL", tlInfo.tlType)))
			State = sv_cam_sys->SVCamSystemInit(i);
	}

	unsigned int enumtimeout = 1000;
	if (State)
		sv_cam_sys->EnumDevices(enumtimeout); // I case of successfully enumeration a device info will be stored in a devInfoList.

	if (sv_cam_sys->devInfoList.size() == 0)
		return ERR_DEVINFOLIST_EMPTY;

	return SUCCESS;
}

int SVSCapture::register_camera(const char* sn)
{
	for (std::vector<Camera*>::iterator cam = sv_cam_sys->sv_cam_list.begin(); cam != sv_cam_sys->sv_cam_list.end(); cam++)
	{
		std::string str((*cam)->devInfo.serialNumber);
		if (str.compare(sn) == 0)
			return ERR_CAM_ALREADY_REGISTERED;
	}

	//Open by serial number in order to verify SN.
	//SV_DEVICE_INFO*  devinf = NULL;
	for (std::vector<SV_DEVICE_INFO*>::iterator dInfo = sv_cam_sys->devInfoList.begin(); dInfo != sv_cam_sys->devInfoList.end(); dInfo++)
	{
		std::string str((*dInfo)->serialNumber);
		if (str.compare(sn) == 0)
		{
			//devinf = *dInfo;
			sv_cam_sys->openDevice(*(*dInfo));
			return sv_cam_sys->sv_cam_list.size() - 1;
		}
	}

	return ERR_SN_MISMATCH;
}

// TODO: add code for continuous shooting mode.
void SVSCapture::set_shooting_mode(int cam_idx, ShootingMode mode)
{
	SVCamAcquisition * sv_acq = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq;
	printf("set shooting mode.\n");

	//if(mode == SOFTWARE_TRIGGER)
	SV_FEATURE_HANDLE hFeature = NULL;
	SVFeatureGetByName(sv_acq->hRemoteDev, "TriggerMode", &hFeature);
	SVFeatureSetValueInt64Enum(sv_acq->hRemoteDev, hFeature, 1);
}

void SVSCapture::start_acquisition(int cam_idx)
{
	unsigned int bufcount = 4;
	sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq->AcquisitionStart(bufcount);
}

/*
Integrate save_image with async thread
*/
int SVSCapture::get_image(int cam_idx, unsigned char * im_buffer)
{
	printf("get_image\n");
	SVCamAcquisition* sv_acq = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq;
	// set up software trigger
	SV_FEATURE_HANDLE hFeature = NULL;
	SVFeatureGetByName(sv_acq->hRemoteDev, "TriggerSoftware", &hFeature);
	// pull trigger
	SVFeatureCommandExecute(sv_acq->hRemoteDev, hFeature, time_out, true);

	printf("im_buffer: %d\n", sv_acq->imageBufferInfo.size());

	if (sv_acq->imageBufferInfo.size() != 0)
	{
		// Obtain the image data pointer and characteristics
		SV_BUFFER_INFO  *NewImageInfo = sv_acq->imageBufferInfo.front();
		printf("new image:  %d \n", NewImageInfo->pImagePtr);
		// assuming RGB 
		int pDestLength = channels * NewImageInfo->iSizeX * NewImageInfo->iSizeY;
		SVUtilBufferBayerToRGB(*NewImageInfo, im_buffer, pDestLength);

		// uncomment for integrated save
		//save_image(NewImageInfo->iSizeX, NewImageInfo->iSizeY, im_buffer);
		sv_acq->imageBufferInfo.pop_front();
		delete NewImageInfo;
	}
	else
	{
		return ERR_NO_IMAGE;
	}

	return SUCCESS;
}

void SVSCapture::stop_acquisition(int cam_idx)
{
	sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq->AcquisitionStop();
}

void SVSCapture::close()
{
	try
	{
		delete sv_cam_sys;
	}
	catch (...)
	{
	}

	SVLibClose();
}

/* NOTE: Ideally, this should be running in a separate thread, but it takes about 1ms to execute.
Convert to RGB with OpenCV
Use thread safe fopen_s
*/
int SVSCapture::save_image(int width, int height, unsigned char *im_buffer)
{
	// Check image alignment
	if (width % 4 == 0)
	{
		BITMAPINFO *bitmapinfo;

		// Generate and fill a bitmap info structure
		bitmapinfo = (BITMAPINFO *)new char[sizeof(BITMAPINFOHEADER)];

		bitmapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapinfo->bmiHeader.biWidth = width;
		bitmapinfo->bmiHeader.biHeight = -height;
		bitmapinfo->bmiHeader.biBitCount = 24;
		bitmapinfo->bmiHeader.biPlanes = 1;
		bitmapinfo->bmiHeader.biClrUsed = 0;
		bitmapinfo->bmiHeader.biClrImportant = 0;
		bitmapinfo->bmiHeader.biCompression = BI_RGB;
		bitmapinfo->bmiHeader.biSizeImage = width * height * channels;
		bitmapinfo->bmiHeader.biXPelsPerMeter = 0;
		bitmapinfo->bmiHeader.biYPelsPerMeter = 0;

		// save to file_path
		__int64 now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		string fn = "C:\\images\\" + std::to_string(now) + ".bmp";
		FILE* im_file = fopen(fn.c_str(), "wb");

		if (im_file == NULL)
		{
			return ERR_IMAGE_FILE_EXISTS;
		}

		BITMAPFILEHEADER bmfh;                         // Other BMP header
		int nBitsOffset = sizeof(BITMAPFILEHEADER) + bitmapinfo->bmiHeader.biSize;
		LONG lImageSize = bitmapinfo->bmiHeader.biSizeImage;
		LONG lFileSize = nBitsOffset + lImageSize;
		bmfh.bfType = 'B' + ('M' << 8);
		bmfh.bfOffBits = nBitsOffset;
		bmfh.bfSize = lFileSize;
		bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

		// Write the bitmap file header               // Saving the first header to file
		UINT nWrittenFileHeaderSize = fwrite(&bmfh, 1, sizeof(BITMAPFILEHEADER), im_file);

		// And then the bitmap info header            // Saving the second header to file
		UINT nWrittenInfoHeaderSize = fwrite(&bitmapinfo->bmiHeader, 1, sizeof(BITMAPINFOHEADER), im_file);

		// Finally, write the image data itself
		//-- the data represents our drawing          // Saving the file content in lpBits to file
		UINT nWrittenDIBDataSize = fwrite(im_buffer, 1, lImageSize, im_file);
		fclose(im_file); // closing the file.

		delete[] bitmapinfo;

		return SUCCESS;
	}
}

// Width, Height, ExposureTime
void SVSCapture::set_feature_int(int cam_idx, const char* name, int value)
{
	SV_FEATURE_HANDLE hFeature = NULL;
	Camera * cam = sv_cam_sys->sv_cam_list.at(cam_idx);
	SVFeatureGetByName(cam->sv_cam_acq->hRemoteDev, name, &hFeature);
	SVFeatureSetValueInt64(cam->sv_cam_acq->hRemoteDev, hFeature, value);
}

void SVSCapture::set_feature_float(int cam_idx, const char* name, double value)
{
	SV_FEATURE_HANDLE hFeature = NULL;
	Camera * cam = sv_cam_sys->sv_cam_list.at(cam_idx);
	SVFeatureGetByName(cam->sv_cam_acq->hRemoteDev, name, &hFeature);
	SVFeatureSetValueFloat(cam->sv_cam_acq->hRemoteDev, hFeature, value);
}

int SVSCapture::to_bitmap(SV_BUFFER_INFO * ImageInfo, unsigned char *image_RGB)
{
	if (ImageInfo == NULL)
		return ERR_BMP_NOIMAGE;

	if (ImageInfo->pImagePtr == NULL)
		return ERR_BMP_NOIMAGE_PTR;

	// Check if a RGB image( Bayer buffer format) arrived
	bool isImgRGB = false;
	int pDestLength = (int)(ImageInfo->iImageSize);

	//  Bayer buffer format(up id: 8)
	if ((ImageInfo->iPixelType & SV_GVSP_PIX_ID_MASK) >= 8)
	{
		isImgRGB = true;
		pDestLength = channels * pDestLength;
	}

	// 8 bit Format
	if ((ImageInfo->iPixelType & SV_GVSP_PIX_EFFECTIVE_PIXELSIZE_MASK) == SV_GVSP_PIX_OCCUPY8BIT)
	{
		// Convert to 24 bit
		if (isImgRGB)
			SVUtilBufferBayerToRGB(*ImageInfo, image_RGB, pDestLength);
	}

	// 12 bit Format
	if ((ImageInfo->iPixelType & SV_GVSP_PIX_EFFECTIVE_PIXELSIZE_MASK) == SV_GVSP_PIX_OCCUPY12BIT)
	{

		if (isImgRGB)
		{
			// Convert to 24 bit and display image
			SVUtilBufferBayerToRGB(*ImageInfo, image_RGB, pDestLength);
		}
	}

	return SUCCESS;
}

int SVSCapture::to_array(unsigned char *im_buffer, unsigned char *arr_buffer)
{
	return -999;
}

void SVSCapture::print_feature_info(int cam_idx)
{
	sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->getDeviceFeatureList(SV_Beginner);
	vector<SVCamFeaturInf*> sv_camfeatureinf = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->featureInfolist;

	printf("--------------------------------------------------------------------\n");

	printf("******************Camera Feature Info*****************\n");
	printf(" ");
	for (std::vector<SVCamFeaturInf*>::iterator i = sv_camfeatureinf.begin() + 1; i != sv_camfeatureinf.end(); i++)
	{
		printf("|");
		for (int j = 0; j < (*i)->SVFeaturInf.level; j++)
			printf("--");

		printf((*i)->SVFeaturInf.displayName);
		printf(": "); printf((*i)->strValue);   printf("\n ");
	}

	printf("--------------------------------------------------------------------\n");
}