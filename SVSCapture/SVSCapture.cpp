// SVSCapture.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "conio.h"
#include "SVSCapture.h"

namespace svs
{
	SVSCapture::SVSCapture(LibraryType libType)
	{
		int res = InitLibrary(libType);

		if (res < 0)
		{
			const string msg = "SVS Vistek SDK not initialized!";
			printf(msg.c_str());
			throw std::invalid_argument(msg);
		}
	}

	int SVSCapture::GetFeatureBool(const char * cam_sn, const char * name)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->getDeviceFeatureList(SV_Beginner);
		vector<SVCamFeaturInf*> sv_camfeatureinf = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->featureInfolist;

		for (std::vector<SVCamFeaturInf*>::iterator i = sv_camfeatureinf.begin() + 1; i != sv_camfeatureinf.end(); i++)
		{
			std::string str((*i)->SVFeaturInf.name);
			if (str.compare(name) == 0)
			{
				return (*i)->booValue;
			}
		}

		return ERR_READ_FEATURE;
	}

	double SVSCapture::GetFeatureFloat(const char * cam_sn, const char * name)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->getDeviceFeatureList(SV_Beginner);
		vector<SVCamFeaturInf*> sv_camfeatureinf = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->featureInfolist;

		for (std::vector<SVCamFeaturInf*>::iterator i = sv_camfeatureinf.begin() + 1; i != sv_camfeatureinf.end(); i++)
		{
			std::string str((*i)->SVFeaturInf.name);
			if (str.compare(name) == 0)
			{
				return (*i)->doubleValue;
			}
		}

		return ERR_READ_FEATURE;
	}

	int SVSCapture::GetFeatureInt(const char * cam_sn, const char * name)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->getDeviceFeatureList(SV_Beginner);
		vector<SVCamFeaturInf*> sv_camfeatureinf = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->featureInfolist;

		for (std::vector<SVCamFeaturInf*>::iterator i = sv_camfeatureinf.begin() + 1; i != sv_camfeatureinf.end(); i++)
		{
			std::string str((*i)->SVFeaturInf.name);
			if (str.compare(name) == 0)
			{
				return (*i)->intValue;
			}
		}

		return ERR_READ_FEATURE;
	}

	const char * SVSCapture::GetFeatureStr(const char * cam_sn, const char * name)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->getDeviceFeatureList(SV_Beginner);
		vector<SVCamFeaturInf*> sv_camfeatureinf = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->featureInfolist;

		for (std::vector<SVCamFeaturInf*>::iterator i = sv_camfeatureinf.begin() + 1; i != sv_camfeatureinf.end(); i++)
		{
			std::string str((*i)->SVFeaturInf.name);
			if (str.compare(name) == 0)
			{
				return (*i)->strValue;
			}
		}

		return "-51";
	}

	void SVSCapture::Close()
	{
		try
		{
			delete sv_cam_sys;
		}
		catch (...)
		{
			printf("Cannot delete sv_cam_sys. Was it already destroyed?");
		}

		SVLibClose();
	}

	/*
	Integrate save_image with async thread
	*/
	int SVSCapture::GetImage(const char* cam_sn, unsigned char * im_buffer)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		SVCamAcquisition* sv_acq = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq;
		// set up software trigger
		SV_FEATURE_HANDLE hFeature = NULL;
		SVFeatureGetByName(sv_acq->hRemoteDev, "TriggerSoftware", &hFeature);
		// pull trigger
		SVFeatureCommandExecute(sv_acq->hRemoteDev, hFeature, acqTimeout, true);

		if (sv_acq->imageBufferInfo.size() != 0)
		{
			// Obtain the image data pointer and characteristics
			SV_BUFFER_INFO  *NewImageInfo = sv_acq->imageBufferInfo.front();

			// assuming RGB 
			int pDestLength = kChannels * NewImageInfo->iSizeX * NewImageInfo->iSizeY;
			SVUtilBufferBayerToRGB(*NewImageInfo, im_buffer, pDestLength);

			sv_acq->imageBufferInfo.pop_front();
			delete NewImageInfo;
		}
		else
		{
			return ERR_NO_IMAGE;
		}

		return SUCCESS;
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

#ifdef _WIN64
		dwRet = GetEnvironmentVariableA("SVS_SDK_GENTL", SVSCti, sizeof(SVSCti));
		if (0 == dwRet)
			return ERR_SVS_SDK_GENTL;
#else
		dwRet = GetEnvironmentVariableA("SVS_SDK_GENTL32", SVSCti, sizeof(SVSCti));
		if (0 == dwRet)
			return ERR_SVS_SDK_GENTL;
#endif

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
		bool sys_state = false;

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
				sys_state = sv_cam_sys->SVCamSystemInit(i);
		}

		if (!sys_state)
		{
			return ERR_SVS_SYS_INIT;
		}
		else
		{
			return SUCCESS;
		}
	}

	int SVSCapture::GetCameraIndex(const char * cam_sn)
	{
		int cam_idx = 0;

		for (std::vector<Camera*>::iterator cam = sv_cam_sys->sv_cam_list.begin(); cam != sv_cam_sys->sv_cam_list.end(); cam++)
		{
			std::string str((*cam)->devInfo.serialNumber);
			if (str.compare(cam_sn) == 0)
				return cam_idx;

			cam_idx += 1;
		}

		return -1;
	}

	// NOT USED
	std::string SVSCapture::GetCameraFeatureValue(const char * cam_sn, const char * name)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->getDeviceFeatureList(SV_Beginner);
		vector<SVCamFeaturInf*> sv_camfeatureinf = sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_feature->featureInfolist;

		for (std::vector<SVCamFeaturInf*>::iterator i = sv_camfeatureinf.begin() + 1; i != sv_camfeatureinf.end(); i++)
		{
			std::string str((*i)->SVFeaturInf.name);
			if (str.compare(name) == 0)
			{
				return (*i)->strValue;
			}
		}

		return "-51";
	}

	void SVSCapture::PrintFeatureInfo(const char* cam_sn)
	{
		int cam_idx = GetCameraIndex(cam_sn);
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
		unsigned int enumtimeout = 1000;

		// Refresh connected devices list
		// In case of successful enumeration, a device info will be stored in a devInfoList.
		sv_cam_sys->EnumDevices(enumtimeout);

		if (sv_cam_sys->devInfoList.size() == 0)
			return ERR_DEVINFOLIST_EMPTY;

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
				//return sv_cam_sys->sv_cam_list.size() - 1;
				return SUCCESS;
			}
		}

		return ERR_SN_MISMATCH;
	}

	void SVSCapture::SetAcquisitionTimeout(const char* cam_sn, int value)
	{
		// TODO: key value pair with cam index
		acqTimeout = value;
	}


	void SVSCapture::SetFeatureFloat(const char* cam_sn, const char* name, double value)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		SV_FEATURE_HANDLE hFeature = NULL;
		Camera * cam = sv_cam_sys->sv_cam_list.at(cam_idx);
		SVFeatureGetByName(cam->sv_cam_acq->hRemoteDev, name, &hFeature);
		SVFeatureSetValueFloat(cam->sv_cam_acq->hRemoteDev, hFeature, value);
	}

	// Width, Height, ExposureTime
	void SVSCapture::SetFeatureInt(const char* cam_sn, const char* name, int value)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		SV_FEATURE_HANDLE hFeature = NULL;
		Camera * cam = sv_cam_sys->sv_cam_list.at(cam_idx);
		SVFeatureGetByName(cam->sv_cam_acq->hRemoteDev, name, &hFeature);
		SVFeatureSetValueInt64(cam->sv_cam_acq->hRemoteDev, hFeature, value);
	}

	void SVSCapture::SetFeatureEnum(const char* cam_sn, const char* name, int value)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		SV_FEATURE_HANDLE hFeature = NULL;
		Camera * cam = sv_cam_sys->sv_cam_list.at(cam_idx);
		SVFeatureGetByName(cam->sv_cam_acq->hRemoteDev, name, &hFeature);
		SVFeatureSetValueInt64Enum(cam->sv_cam_acq->hRemoteDev, hFeature, value);
	}

	void SVSCapture::StartAcquisition(const char* cam_sn)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		unsigned int bufcount = 4;
		sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq->AcquisitionStart(bufcount);
	}

	void SVSCapture::StopAcquisition(const char* cam_sn)
	{
		int cam_idx = GetCameraIndex(cam_sn);
		sv_cam_sys->sv_cam_list.at(cam_idx)->sv_cam_acq->AcquisitionStop();
	}
}

svs::SVSCapture* SVSCapture_new(int lib_type)
{
	return new svs::SVSCapture((svs::LibraryType)lib_type);
}

int SVSCapture_get_feature_enum(svs::SVSCapture * sv_cap, const char * cam_sn, const char * name)
{
	return sv_cap->GetFeatureInt(cam_sn, name);
}

int SVSCapture_get_feature_bool(svs::SVSCapture * sv_cap, const char * cam_sn, const char * name)
{
	return sv_cap->GetFeatureBool(cam_sn, name);
}

double SVSCapture_get_feature_float(svs::SVSCapture * sv_cap, const char * cam_sn, const char * name)
{
	return sv_cap->GetFeatureFloat(cam_sn, name);
}

int SVSCapture_get_feature_int(svs::SVSCapture * sv_cap, const char * cam_sn, const char * name)
{
	return sv_cap->GetFeatureInt(cam_sn, name);
}

//const char * SVSCapture_get_feature_str(svs::SVSCapture * sv_cap, const char * cam_sn, const char * name)
//{
//	return sv_cap->GetFeatureStr(cam_sn, name);
//}

int SVSCapture_reg_camera(svs::SVSCapture* sv_cap, const char* sn)
{
	return sv_cap->RegisterCamera(sn);
}

void SVSCapture_set_feature_enum(svs::SVSCapture * sv_cap, const char * cam_sn, const char * name, int value)
{
	sv_cap->SetFeatureEnum(cam_sn, name, value);
}

void SVSCapture_set_feature_float(svs::SVSCapture* sv_cap, const char* cam_sn, const char* name, double value)
{
	sv_cap->SetFeatureFloat(cam_sn, name, value);
}

void SVSCapture_set_feature_int(svs::SVSCapture* sv_cap, const char* cam_sn, const char* name, int value)
{
	sv_cap->SetFeatureInt(cam_sn, name, value);
}

void SVSCapture_set_acq_timeout(svs::SVSCapture* sv_cap, const char* cam_sn, int value)
{
	sv_cap->SetAcquisitionTimeout(cam_sn, value);
}

void SVSCapture_start_acq(svs::SVSCapture* sv_cap, const char* cam_sn)
{
	sv_cap->StartAcquisition(cam_sn);
}

int SVSCapture_get_image(svs::SVSCapture* sv_cap, const char* cam_sn, unsigned char * im_buffer)
{
	return sv_cap->GetImage(cam_sn, im_buffer);
}

void SVSCapture_stop_acq(svs::SVSCapture* sv_cap, const char* cam_sn)
{
	sv_cap->StopAcquisition(cam_sn);
}

void SVSCapture_close(svs::SVSCapture* sv_cap)
{
	sv_cap->Close();
}