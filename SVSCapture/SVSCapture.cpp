// SVSCapture.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "conio.h"
#include <opencv2/opencv.hpp>
#include "SVSCapture.h"

SVSCapture::SVSCapture(LibraryType libType)
{
	int res = InitLibrary(libType);

	if (res < 0)
		throw std::invalid_argument("SVS Vistek SDK not initialized!");

	SetThresholdLimits();
}

void SVSCapture::Close()
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

/*
Integrate save_image with async thread
*/
int SVSCapture::GetImage(int cam_idx, unsigned char * im_buffer)
{
	printf("get_image\n");
	SVCamAcquisition* sv_acq = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq;
	// set up software trigger
	SV_FEATURE_HANDLE hFeature = NULL;
	SVFeatureGetByName(sv_acq->hRemoteDev, "TriggerSoftware", &hFeature);
	// pull trigger
	SVFeatureCommandExecute(sv_acq->hRemoteDev, hFeature, kTimeOut, true);

	printf("im_buffer: %d\n", sv_acq->imageBufferInfo.size());

	if (sv_acq->imageBufferInfo.size() != 0)
	{
		// Obtain the image data pointer and characteristics
		SV_BUFFER_INFO  *NewImageInfo = sv_acq->imageBufferInfo.front();
		printf("new image:  %d \n", NewImageInfo->pImagePtr);
		// assuming RGB 
		int pDestLength = kChannels * NewImageInfo->iSizeX * NewImageInfo->iSizeY;
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

/*
NOTE: Not implemented.
Pass a vector for <int, int> outputs
*/
int SVSCapture::GetROI(unsigned char * im_buffer)
{

	return -999;
}

int SVSCapture::InitLibrary(LibraryType libType)
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

void SVSCapture::PrintFeatureInfo(int cam_idx)
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

int SVSCapture::RegisterCamera(const char* sn)
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
void SVSCapture::SetShootingMode(int cam_idx, ShootingMode mode)
{
	SVCamAcquisition * sv_acq = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq;
	printf("set shooting mode.\n");

	//if(mode == SOFTWARE_TRIGGER)
	SV_FEATURE_HANDLE hFeature = NULL;
	SVFeatureGetByName(sv_acq->hRemoteDev, "TriggerMode", &hFeature);
	SVFeatureSetValueInt64Enum(sv_acq->hRemoteDev, hFeature, 1);
}

/* NOTE: This should be running in a separate thread due to exceptions risks and latency overhead, 
			but it takes about 1ms to execute and the file_name being passed is precise down to the millisecond.
*/
int SVSCapture::SaveImage(const char * file_name, int width, int height, unsigned char *im_buffer)
{
	cv::Mat image(cv::Size((unsigned int)width, (unsigned int)height), CV_8UC3, im_buffer, cv::Mat::AUTO_STEP);

	try
	{
		imwrite(file_name, image);
	}
	catch (cv::Exception & e)
	{
		cerr << e.msg << endl;
	}

	return SUCCESS;
}

void SVSCapture::SetFeatureFloat(int cam_idx, const char* name, double value)
{
	SV_FEATURE_HANDLE hFeature = NULL;
	Camera * cam = sv_cam_sys->sv_cam_list.at(cam_idx);
	SVFeatureGetByName(cam->sv_cam_acq->hRemoteDev, name, &hFeature);
	SVFeatureSetValueFloat(cam->sv_cam_acq->hRemoteDev, hFeature, value);
}

// Width, Height, ExposureTime
void SVSCapture::SetFeatureInt(int cam_idx, const char* name, int value)
{
	SV_FEATURE_HANDLE hFeature = NULL;
	Camera * cam = sv_cam_sys->sv_cam_list.at(cam_idx);
	SVFeatureGetByName(cam->sv_cam_acq->hRemoteDev, name, &hFeature);
	SVFeatureSetValueInt64(cam->sv_cam_acq->hRemoteDev, hFeature, value);
}

void SVSCapture::SetThresholdLimits()
{
	threshold.min_R = 0;
	threshold.max_R = 255;
	threshold.min_G = 0;
	threshold.max_G = 255;
	threshold.min_B = 0;
	threshold.max_B = 255;
}

void SVSCapture::StartAcquisition(int cam_idx)
{
	unsigned int bufcount = 4;
	sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq->AcquisitionStart(bufcount);
}

void SVSCapture::StopAcquisition(int cam_idx)
{
	sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq->AcquisitionStop();
}

/*
C linkage functions
*/
SVSCapture* SVSCapture_new(int lib_type) 
{ 
	return new SVSCapture((LibraryType)lib_type); 
}

int SVSCapture_reg_camera(SVSCapture* sv_cap, const char* sn) 
{ 
	return sv_cap->RegisterCamera(sn); 
}

void SVSCapture_set_shooting_mode(SVSCapture* sv_cap, int cam_idx, int mode) 
{ 
	sv_cap->SetShootingMode(cam_idx, (ShootingMode)mode); 
}

void SVSCapture_set_feature_int(SVSCapture* sv_cap, int cam_idx, const char* name, int value) 
{ 
	sv_cap->SetFeatureInt(cam_idx, name, value); 
}

void SVSCapture_set_feature_float(SVSCapture* sv_cap, int cam_idx, const char* name, double value) 
{ 
	sv_cap->SetFeatureFloat(cam_idx, name, value); 
}

void SVSCapture_start_acq(SVSCapture* sv_cap, int cam_idx) 
{ 
	sv_cap->StartAcquisition(cam_idx); 
}

int SVSCapture_get_image(SVSCapture* sv_cap, int cam_idx, unsigned char * im_buffer) 
{ 
	return sv_cap->GetImage(cam_idx, im_buffer); 
}

void SVSCapture_stop_acq(SVSCapture* sv_cap, int cam_idx) 
{ 
	sv_cap->StopAcquisition(cam_idx); 
}

void SVSCapture_close(SVSCapture* sv_cap) 
{ 
	sv_cap->Close(); 
}

void SVSCapture_save_image(SVSCapture* sv_cap, const char* file_name, int width, int height, unsigned char * im_buffer) 
{ 
	sv_cap->SaveImage(file_name, width, height, im_buffer); 
}
