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

using namespace std;

/*
Camera interface types
*/
enum SVSCAPTURE_API LibraryType
{
	USB3 = 0,
	GIG_E = 1,
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
	int register_camera(const char * sn);
	/*
	NOTE: only software trigger is implemented.
	Args:
	cam_idx: camera index (in sv_cam_list)
	mode: shooting mode
	*/
	void set_shooting_mode(int cam_idx, ShootingMode mode);
	/*
	Set camera int feature value.
	Note: must be devisible by 8.
	Args:
	cam_idx: camera index (in sv_cam_list)
	name: feature name
	value: feature value
	*/
	void set_feature_int(int cam_idx, const char * name, int value);
	/*
	Set camera float feature value.
	Args:
	cam_idx: camera index (in sv_cam_list)
	name: feature name
	value: feature value
	*/
	void set_feature_float(int cam_idx, const char * name, double value);
	/*
	Open acquisition stream.
	Args:
	cam_idx: camera index (in sv_cam_list)
	*/
	void start_acquisition(int cam_idx);
	/*
	Get image from stream as an RGB byte array.
	Args:
	cam_idx: camera index (in sv_cam_list)
	im_buffer: image bufffer to store the result
	*/
	int get_image(int cam_idx, unsigned char * im_buffer);
	/*
	Close acquisition stream.
	Args:
	cam_idx: camera index (in sv_cam_list)
	*/
	void stop_acquisition(int cam_idx);
	/*
	Close the library.
	*/
	void close();

	/*
	Save image to file system.
	Note: default location is C\images
	*/
	int save_image(int width, int height, unsigned char *im_buffer);

	/*
	Print camera settings.
	Args:
	cam_idx: camera index (in sv_cam_list)
	*/
	void print_feature_info(int cam_idx);

	/*
	Made public to aid troubleshooting
	*/
	SVCamSystem * sv_cam_sys = NULL;

private:
	const UINT32 time_out = 40;
	const int channels = 3;
	/*
	Initialize library, find attached devices.
	Note: need to create and maintain a separate SVCamSystem instance per interface type(GigE, USB, CL).
	*/
	int init_library(LibraryType libType);

	/*
	Convert raw bytes to bitmap.
	Only RGB code is added.
	*/
	int to_bitmap(SV_BUFFER_INFO * ImageInfo, unsigned char *image_RGB);

	/*
	Convert image buffer to a opencv matrix.
	Args:
	im_buffer: address of image buffer
	arr_buffer: address of array buffer
	*/
	int to_array(unsigned char *im_buffer, unsigned char *arr_buffer);
};

#ifdef __cplusplus
extern "C" {
#endif
	SVSCAPTURE_API SVSCapture* SVSCapture_new(int lib_type) { return new SVSCapture((LibraryType)lib_type); }
	int  SVSCAPTURE_API SVSCapture_reg_camera(SVSCapture* sv_cap, char* sn) { return sv_cap->register_camera(sn); }
	void SVSCAPTURE_API SVSCapture_set_shooting_mode(SVSCapture* sv_cap, int cam_idx, int mode) { sv_cap->set_shooting_mode(cam_idx, (ShootingMode)mode); }
	void SVSCAPTURE_API SVSCapture_set_feature_int(SVSCapture* sv_cap, int cam_idx, const char* name, int value) { sv_cap->set_feature_int(cam_idx, name, value); }
	void SVSCAPTURE_API SVSCapture_set_feature_float(SVSCapture* sv_cap, int cam_idx, const char* name, double value) { sv_cap->set_feature_int(cam_idx, name, value); }
	void SVSCAPTURE_API SVSCapture_start_acq(SVSCapture* sv_cap, int cam_idx) { sv_cap->start_acquisition(cam_idx); }
	int  SVSCAPTURE_API SVSCapture_get_image(SVSCapture* sv_cap, int cam_idx, unsigned char * im_buffer) { return sv_cap->get_image(cam_idx, im_buffer); }
	void SVSCAPTURE_API SVSCapture_stop_acq(SVSCapture* sv_cap, int cam_idx) { sv_cap->stop_acquisition(cam_idx); }
	void SVSCAPTURE_API SVSCapture_close(SVSCapture* sv_cap) { sv_cap->close(); }
	void SVSCAPTURE_API SVSCapture_save_image(SVSCapture* sv_cap, int width, int height, unsigned char * im_buffer) { sv_cap->save_image(width, height, im_buffer); }
#ifdef __cplusplus
}
#endif