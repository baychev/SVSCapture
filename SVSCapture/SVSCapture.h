#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <iostream>
#include "SVCamSystem.h"

#ifdef SVSCAPTURE_EXPORTS
#define SVSCAPTURE_API __declspec(dllexport)
#else
#define SVSCAPTURE_API __declspec(dllimport)
#endif

namespace svs
{
	/*
	Camera interface types
	*/
	enum SVSCAPTURE_API LibraryType
	{
		USB3 = 1,
		GIG_E = 0,
		CAMERA_LINK = 2
	};

	/*
	Camera trigger mode. ON activates software trigger.
	*/
	enum SVSCAPTURE_API TriggerMode
	{
		OFF = 0,
		ON = 1
	};

	/*
	Operational status codes
	*/
	enum SVSCAPTURE_API Status
	{
		SUCCESS = 0,
		ERR_SVS_GENICAM_ROOT = -1,
		ERR_SVS_SDK_GENTL = -2,
		ERR_SVS_GENICAM_CLPROTOCOL = -3,
		ERR_SVS_GENICAM_CACHE = -4,
		ERR_SVLIB_INIT = -5,
		ERR_DEVINFOLIST_EMPTY = -6,
		ERR_SVS_SYS_INIT = -7,
		ERR_SN_MISMATCH = -11,
		ERR_CAM_ALREADY_REGISTERED = -12,
		ERR_NO_IMAGE = -21,
		ERR_IMAGE_FILE_EXISTS = -31,
		ERR_BMP_NOIMAGE = -41,
		ERR_BMP_NOIMAGE_PTR = -42,
		ERR_READ_FEATURE = -51
	};

	/*
	SVS Vistek SVGenSDK wrapper.
	64 bit libraries are imported only!
	*/
	class SVSCAPTURE_API SVSCapture
	{
	public:
		/*
		Initialize the SDK library with a given camera interface type.
		Args:
		libType: camera interface type
		*/
		SVSCapture(LibraryType libType);

		int GetFeatureBool(const char* cam_sn, const char * name);
		double GetFeatureFloat(const char* cam_sn, const char * name);
		int GetFeatureInt(const char* cam_sn, const char * name);
		void GetFeatureStr(const char* cam_sn, const char * name, char * value);

		/*
		Find camera by SN, add it to device info list.
		Args:
		sn: serial number
		*/
		int RegisterCamera(const char * sn);

		void SetAcquisitionTimeout(const char* cam_sn, int value);

		void SetFeatureEnum(const char* cam_sn, const char * name, int value);
		/*
		Set camera int feature value.
		Note: must be devisible by 8.
		Args:
		cam_sn: camera serial number
		name: feature name
		value: feature value
		*/
		void SetFeatureInt(const char* cam_sn, const char * name, int value);
		/*
		Set camera float feature value.
		Args:
		cam_sn: camera serial number
		name: feature name
		value: feature value
		*/
		void SetFeatureFloat(const char* cam_sn, const char * name, double value);
		/*
		Open acquisition stream.
		Args:
		cam_sn: camera serial number
		*/
		void StartAcquisition(const char* cam_sn);
		/*
		Get image from stream as an RGB byte array.
		Args:
		cam_sn: camera serial number
		im_buffer: image bufffer to store the result
		NOTE: this should create an OpenCV Mat3b
		*/
		int GetImage(const char* cam_sn, unsigned char * im_buffer);
		/*
		Close acquisition stream.
		Args:
		cam_sn: camera serial number
		*/
		void StopAcquisition(const char* cam_sn);
		/*
		Close the library.
		*/
		void Close();
		/*
		Print camera settings.
		Args:
		cam_sn: camera serial number
		*/
		void PrintFeatureInfo(const char* cam_sn);

	private:
		SVCamSystem * sv_cam_sys = NULL;
		UINT32 acqTimeout = 50; // Modify this value based on camera frame rate.
		const int kChannels = 3;
		/*
		Initialize library, find attached devices.
		Note: need to create and maintain a separate SVCamSystem instance per interface type(GigE, USB, CL).
		*/
		int InitLibrary(LibraryType libType);
		int GetCameraIndex(const char* cam_sn);
		std::string GetCameraFeatureValue(const char* cam_sn, const char * name);
	};
}
/*
NOTE: deviate from general function naming convention in order to improve readability.
*/
#ifdef __cplusplus
extern "C" {
#endif
	/*
	Initialize the SDK library with a given camera interface type.
	*/
	SVSCAPTURE_API svs::SVSCapture* SVSCapture_new(int lib_type);
	/*
	Close the library, free up resources.
	*/
	void SVSCAPTURE_API SVSCapture_close(svs::SVSCapture* sv_cap);
	/*
	Get an image from a camera.
	*/
	int  SVSCAPTURE_API SVSCapture_get_image(svs::SVSCapture* sv_cap, const char* cam_sn, unsigned char * im_buffer);
	/*
	Register camera for future interaction.
	*/
	int SVSCAPTURE_API SVSCapture_get_feature_bool(svs::SVSCapture* sv_cap, const char* cam_sn, const char* name);
	double SVSCAPTURE_API SVSCapture_get_feature_float(svs::SVSCapture* sv_cap, const char* cam_sn, const char* name);
	int SVSCAPTURE_API SVSCapture_get_feature_int(svs::SVSCapture* sv_cap, const char* cam_sn, const char* name);
	void SVSCAPTURE_API SVSCapture_get_feature_str(svs::SVSCapture* sv_cap, const char* cam_sn, const char* name, char * value);
	int  SVSCAPTURE_API SVSCapture_reg_camera(svs::SVSCapture* sv_cap, const char* sn);
	void SVSCAPTURE_API SVSCapture_set_feature_enum(svs::SVSCapture* sv_cap, const char* cam_sn, const char* name, int value);
	void SVSCAPTURE_API SVSCapture_set_feature_float(svs::SVSCapture* sv_cap, const char* cam_sn, const char* name, double value);
	void SVSCAPTURE_API SVSCapture_set_feature_int(svs::SVSCapture* sv_cap, const char* cam_sn, const char* name, int value);
	void SVSCAPTURE_API SVSCapture_set_acq_timeout(svs::SVSCapture* sv_cap, const char* cam_sn, int value);
	void SVSCAPTURE_API SVSCapture_start_acq(svs::SVSCapture* sv_cap, const char* cam_sn);
	void SVSCAPTURE_API SVSCapture_stop_acq(svs::SVSCapture* sv_cap, const char* cam_sn);
#ifdef __cplusplus
}
#endif