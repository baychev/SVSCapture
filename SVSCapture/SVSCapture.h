#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <iostream>
#include "ImageUtils.h"
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
	Camera shooting modes
	*/
	enum SVSCAPTURE_API ShootingMode
	{
		CONTINUOUS = 0,
		SOFTWARE_TRIGGER = 1
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
		ERR_BMP_NOIMAGE_PTR = -42
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
		/*
		Find camera by SN, add it to device info list.
		Args:
		sn: serial number
		*/
		int RegisterCamera(const char * sn);
		/*
		Set camera int feature value.
		Note: must be devisible by 8.
		Args:
		cam_idx: camera index (in sv_cam_list)
		name: feature name
		value: feature value
		*/
		void SetFeatureInt(int cam_idx, const char * name, int value);
		/*
		Set camera float feature value.
		Args:
		cam_idx: camera index (in sv_cam_list)
		name: feature name
		value: feature value
		*/
		void SetFeatureFloat(int cam_idx, const char * name, double value);
		/*
		Open acquisition stream.
		Args:
		cam_idx: camera index (in sv_cam_list)
		*/
		void StartAcquisition(int cam_idx);
		/*
		Get image from stream as an RGB byte array.
		Args:
		cam_idx: camera index (in sv_cam_list)
		im_buffer: image bufffer to store the result
		NOTE: this should create an OpenCV Mat3b
		*/
		int GetImage(int cam_idx, unsigned char * im_buffer);
		/*
		Close acquisition stream.
		Args:
		cam_idx: camera index (in sv_cam_list)
		*/
		void StopAcquisition(int cam_idx);
		/*
		Close the library.
		*/
		void Close();
		/*
		Print camera settings.
		Args:
		cam_idx: camera index (in sv_cam_list)
		*/
		void PrintFeatureInfo(int cam_idx);

	private:
		SVCamSystem * sv_cam_sys = NULL;
		const UINT32 kTimeOut = 40;
		const int kChannels = 3;
		/*
		Initialize library, find attached devices.
		Note: need to create and maintain a separate SVCamSystem instance per interface type(GigE, USB, CL).
		*/
		int InitLibrary(LibraryType libType);
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
	int  SVSCAPTURE_API SVSCapture_get_image(svs::SVSCapture* sv_cap, int cam_idx, unsigned char * im_buffer);
	/*
	Register camera for future interaction.
	*/
	int  SVSCAPTURE_API SVSCapture_reg_camera(svs::SVSCapture* sv_cap, const char* sn);
	/*
	Save image to the file system. Use default compression in OpenCV.
	*/
	void SVSCAPTURE_API SVSCapture_set_feature_float(svs::SVSCapture* sv_cap, int cam_idx, const char* name, double value);
	void SVSCAPTURE_API SVSCapture_set_feature_int(svs::SVSCapture* sv_cap, int cam_idx, const char* name, int value);
	void SVSCAPTURE_API SVSCapture_start_acq(svs::SVSCapture* sv_cap, int cam_idx);
	void SVSCAPTURE_API SVSCapture_stop_acq(svs::SVSCapture* sv_cap, int cam_idx);
#ifdef __cplusplus
}
#endif