from constants import *

import ctypes as ct
import time
import utils

_api_version = "2.3.1"
print("SVS-Vistek Python API version {}".format(_api_version))

assert utils.check_dll() == True

class Image(object):
    def __init__(self, cam):
        self.data = []
        self.width = cam.image_width
        self.height = cam.image_height
        self.channels = cam.image_channels
        self.camera_serial_number = cam.serial_number
        self.status_code = None
        self.timestamp = None

    @utils.timethis
    def save(self, file_path="C:\\images"):
        utils.save_image(self, file_path=file_path)

class Camera(object):
    name = None
    serial_number = None
    image_channels = 0
    image_height = 0
    image_width = 0
    interface_type = None
    exposure_time = 2000  # microseconds
    trigger_mode = None

    def __init__(self, serial_number, interface_type=InterfaceType.USB3):
        self.interface_type = interface_type
        self.serial_number = serial_number
        # Safeguard for serial_number placeholder. Delete if yours is not numeric.
        assert int(self.serial_number) > 0

class SVCapture(object):
    lib = None
    cameras = []
    #  Interface pointers
    _interface_cameralink = None    # CameraLink
    _interface_gige = None  # GigE
    _interface_usb3 = None  # USB3

    def __init__(self):
        self.lib = ct.cdll.LoadLibrary(dll_path)
        assert self.lib != None

        self.lib.SVCaptureAPI_new.argtypes = [ct.c_int]
        self.lib.SVCaptureAPI_new.restype = ct.c_void_p

        self.lib.SVCaptureAPI_set_feature_enum.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p, ct.c_int]
        self.lib.SVCaptureAPI_set_feature_enum.restype = ct.c_void_p

        self.lib.SVCaptureAPI_set_feature_int.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p, ct.c_int]
        self.lib.SVCaptureAPI_set_feature_int.restype = ct.c_void_p

        self.lib.SVCaptureAPI_set_feature_float.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p, ct.c_double]
        self.lib.SVCaptureAPI_set_feature_float.restype = ct.c_void_p

    def __enter__(self):
        return self

    @utils.timethis
    def register_camera(self, cam):
        print("registering camera...")
        existing_cam = next((x for x in self.cameras if x.serial_number == cam.serial_number), None)

        if existing_cam:
            print("Camera with SN: {} already added.".format(existing_cam.serial_number))
            raise Exception("Cannot add twice camera with SN: {}".format(cam.serial_number))

        self._init_camera_interface(cam)
        # add device to list
        self.lib.SVCaptureAPI_reg_camera.argtypes = [ct.c_void_p, ct.c_char_p]
        self.lib.SVCaptureAPI_reg_camera.restype = ct.c_int16

        status = self.lib.SVCaptureAPI_reg_camera(self._get_camera_interface(cam), cam.serial_number.encode(ASCII))
        assert status == 0
        self.load_settings(cam)
        self.cameras.append(cam)

        if status == 0:
            print("... registered.")
        else:
            print("... FAILED!")

        return status == 0

    def _init_camera_interface(self, cam):
        if cam.interface_type == InterfaceType.CAMERA_LINK and self._interface_cameralink == None:
            self._interface_cameralink = self.lib.SVCaptureAPI_new(cam.interface_type)
            assert self._interface_cameralink != None
        elif cam.interface_type == InterfaceType.GIG_E and self._interface_gige == None:
            self._interface_gige = self.lib.SVCaptureAPI_new(cam.interface_type)
            assert self._interface_gige != None
        elif cam.interface_type == InterfaceType.USB3 and self._interface_usb3 == None:
            self._interface_usb3 = self.lib.SVCaptureAPI_new(cam.interface_type)
            assert self._interface_usb3 != None

    def _get_camera_interface(self, cam):
        if cam.interface_type == InterfaceType.CAMERA_LINK:
            return self._interface_cameralink
        elif cam.interface_type == InterfaceType.GIG_E:
            return self._interface_gige
        elif cam.interface_type == InterfaceType.USB3:
            return self._interface_usb3

    # not tested
    def set_setting(self, cam, name, value):
        settingDataType = self._get_setting_data_type(name)

        if settingDataType == CameraSettingDataType.INT:
            self.lib.SVCaptureAPI_set_feature_int(self._get_camera_interface(cam), cam.serial_number.encode(ASCII), name.encode(ASCII), value)
        elif settingDataType == CameraSettingDataType.FLOAT:
            self.lib.SVCaptureAPI_set_feature_float(self._get_camera_interface(cam), cam.serial_number.encode(ASCII), name.encode(ASCII), value)
        elif settingDataType == CameraSettingDataType.ENUM:
            self.lib.SVCaptureAPI_set_feature_enum(self._get_camera_interface(cam), cam.serial_number.encode(ASCII), name.encode(ASCII), value)
        else:
            print("WARNING: Camera setting {} requires custom code.".format(name))

    def _get_setting_data_type(self, name):
        settings = CameraSettings()
        if name in settings.get_enums():
            return CameraSettingDataType.ENUM
        elif name in settings.get_floats():
            return CameraSettingDataType.FLOAT
        elif name in settings.get_ints():
            return CameraSettingDataType.INT
        else:
            return None

    def get_setting(self, cam, name):
        settingDataType = self._get_setting_data_type(name)

        if settingDataType == CameraSettingDataType.INT:
            self.lib.SVCaptureAPI_get_feature_int.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
            self.lib.SVCaptureAPI_get_feature_int.restype = ct.c_int
            return self.lib.SVCaptureAPI_get_feature_int(self._get_camera_interface(cam), cam.serial_number.encode(ASCII), name.encode(ASCII))
        elif settingDataType == CameraSettingDataType.FLOAT:
            self.lib.SVCaptureAPI_get_feature_float.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
            self.lib.SVCaptureAPI_get_feature_float.restype = ct.c_double
            return self.lib.SVCaptureAPI_get_feature_float(self._get_camera_interface(cam), cam.serial_number.encode(ASCII), name.encode(ASCII))
        elif settingDataType == CameraSettingDataType.ENUM:
            self.lib.SVCaptureAPI_get_feature_int.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_char_p]
            self.lib.SVCaptureAPI_get_feature_int.restype = ct.c_int
            return self.lib.SVCaptureAPI_get_feature_int(self._get_camera_interface(cam), cam.serial_number.encode(ASCII), name.encode(ASCII))
        else:
            raise Exception("Getting setting type {} not implemented.".format(settingDataType))

    @utils.timethis
    def load_settings(self, cam):
        cam.image_channels = 3
        cam.image_width = self.get_setting(cam, CameraSettings.WIDTH)
        cam.image_height = self.get_setting(cam, CameraSettings.HEIGHT)
        cam.exposure_time = self.get_setting(cam, CameraSettings.EXPOSURE_TIME)
        print("---> camera settings W:{0}, H:{1}, C:{2}, EXPOSURE:{3}".format(cam.image_width, cam.image_height, cam.image_channels, cam.exposure_time))

    @utils.timethis
    def start_acq(self, cam):
        self.lib.SVCaptureAPI_start_acq.argtypes = [ct.c_void_p, ct.c_char_p]
        self.lib.SVCaptureAPI_start_acq.restype = ct.c_void_p
        self.lib.SVCaptureAPI_start_acq(self._get_camera_interface(cam), cam.serial_number.encode(ASCII))

    def ramp_up(self, cam):
        time_sec = 3
        print("ramping up for {} seconds...".format(str(time_sec)))
        size = cam.image_width * cam.image_height * cam.image_channels
        _ = (ct.c_ubyte * size)()

        self.lib.SVCaptureAPI_get_image.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(ct.c_ubyte)]
        self.lib.SVCaptureAPI_get_image.restype = ct.c_int

        time.sleep(time_sec)
        res = self.lib.SVCaptureAPI_get_image(self._get_camera_interface(cam), cam.serial_number.encode(ASCII), _)
    
    @utils.timethis
    def get_image(self, cam):
        timeout_ms = 1000
        image = Image(cam)
        image.data = (ct.c_ubyte * (cam.image_width * cam.image_height * cam.image_channels))()

        self.lib.SVCaptureAPI_get_image.argtypes = [ct.c_void_p, ct.c_char_p, ct.POINTER(ct.c_ubyte)]
        self.lib.SVCaptureAPI_get_image.restype = ct.c_int
        
        res = -1
        retries = 0
        time_start = time.time() * 1000
        while time.time() * 1000 < time_start + timeout_ms:
            res = self.lib.SVCaptureAPI_get_image(self._get_camera_interface(cam), cam.serial_number.encode(ASCII), image.data)
            if res == 0:
                if retries > 0:
                    print('Got an image with {} retries'.format(retries))
                break
            
            retries +=1
        if res != 0:
            print('Could not obtain an image from {} retries.'.format(retries))
  
        image.timestamp = int(time.time() * 1000)
        image.status_code = res
        return image

    @utils.timethis
    def stop_acq(self, cam):
        self.lib.SVCaptureAPI_stop_acq.argtypes = [ct.c_void_p, ct.c_char_p]
        self.lib.SVCaptureAPI_stop_acq.restype = ct.c_void_p
        self.lib.SVCaptureAPI_stop_acq(self._get_camera_interface(cam), cam.serial_number.encode(ASCII))

    def close(self):
        self.lib.SVCaptureAPI_close.argtypes = [ct.c_void_p]
        self.lib.SVCaptureAPI_close.restype = ct.c_void_p

        if self._interface_cameralink:
            self.lib.SVCaptureAPI_close(self._interface_cameralink)
        if self._interface_gige:
            self.lib.SVCaptureAPI_close(self._interface_gige)
        if self._interface_usb3:
            self.lib.SVCaptureAPI_close(self._interface_usb3)

    def __exit__(self, type, value, traceback):
        self.cameras = None

        if self._interface_usb3:
            self.close()

        if self.lib:
            del self.lib