from constants import *
from utils import *

import ctypes
import time

_api_version = "2.3.1"
print("SVS-Vistek Python API version {}".format(_api_version))

assert check_dll() == True

class Image(object):
    def __init__(self, cam):
        self.data = []
        self.width = cam.image_width
        self.height = cam.image_height
        self.channels = cam.image_channels
        self.camera_serial_number = cam.serial_number
        self.status_code = None
        self.timestamp = None

    @timethis
    def save(self, file_path="C:\\images"):
        save_image(self, file_path=file_path)

class Camera(object):
    index = -1
    name = None
    serial_number = None
    image_channels = 1
    image_height = 1
    image_width = 1
    interface_type = None
    exposure_time = 2000  # microseconds
    trigger_mode = None

    def __init__(self, serial_number, interface_type=InterfaceType.USB3):
        self.interface_type = interface_type
        self.serial_number = serial_number
        # Safeguard for serial_number placeholder. Delete if yours is not numeric.
        assert int(self.serial_number) > 0
        # TODO: load stored camera settings

    def configure(self, im_width, im_height, im_channels, exposure_time, trigger_mode=TriggerMode.SOFTWARE_TRIGGER):
        print("WARNING: Camera.configure() does NOT write any settings to the camera. Use SVCapture.set_setting().")
        self.image_channels = im_channels
        self.image_height = im_height
        self.image_width = im_width
        self.exposure_time = exposure_time
        self.trigger_mode = trigger_mode
        # Safeguard for unimplemented feature.
        assert self.trigger_mode != TriggerMode.CONTINUOUS

    def _check_configuration(self):
        pass

class SVSCapture(object):
    lib = None
    cameras = []
    #  Interface pointers
    ci_cl = None    # CameraLink
    ci_gige = None  # GigE
    ci_usb3 = None  # USB3

    def __init__(self):
        self.lib = ctypes.cdll.LoadLibrary(dll_path)
        assert self.lib != None

        self.lib.SVSCapture_new.argtypes = [ctypes.c_int]
        self.lib.SVSCapture_new.restype = ctypes.c_void_p

        self.lib.SVSCapture_set_feature_enum.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.SVSCapture_set_feature_enum.restype = ctypes.c_void_p

        self.lib.SVSCapture_set_feature_int.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        self.lib.SVSCapture_set_feature_int.restype = ctypes.c_void_p

        self.lib.SVSCapture_set_feature_float.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_double]
        self.lib.SVSCapture_set_feature_float.restype = ctypes.c_void_p

    def __enter__(self):
        return self

    @timethis
    def register_camera(self, cam):
        print("registering camera...")
        existing_cam = next((x for x in self.cameras if x.serial_number == cam.serial_number), None)

        if existing_cam:
            print("Camera with SN: {} already added.".format(existing_cam.serial_number))
            raise Exception("Cannot add twice camera with SN: {}".format(cam.serial_number))

        self._init_camera_interface(cam)
        # add device to list
        # NOTE: Camera index is not a good reference, serial number shall be used for robustness.
        self.lib.SVSCapture_reg_camera.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        self.lib.SVSCapture_reg_camera.restype = ctypes.c_int16

        cam.index = self.lib.SVSCapture_reg_camera(self._get_camera_interface(cam), str_to_bytearray(cam.serial_number))
        assert cam.index >= 0
        self.cameras.append(cam)

        # set up shooting properties
        # NOTE: Only software trigger has been tested.
        #       Coninuous mode shall provide access to the whole image buffer.
        set_properties = False

        if not set_properties:
            print("WARNING: The Python client application may fail!")

        if set_properties:
            self.lib.SVSCapture_set_feature_enum(self._get_camera_interface(cam), cam.index, str_to_bytearray(CameraSettings.TRIGGER_MODE), cam.trigger_mode)
            self.lib.SVSCapture_set_feature_int(self._get_camera_interface(cam), cam.index, str_to_bytearray(CameraSettings.HEIGHT), cam.image_height)
            self.lib.SVSCapture_set_feature_int(self._get_camera_interface(cam), cam.index, str_to_bytearray(CameraSettings.WIDTH), cam.image_width)
            self.lib.SVSCapture_set_feature_float(self._get_camera_interface(cam), cam.index, str_to_bytearray(CameraSettings.EXPOSURE_TIME), cam.exposure_time)

        if cam.index >= 0:
            print("---> done")
        else:
            print("---> FAILED")

        return cam.index >= 0

    def _init_camera_interface(self, cam):
        if cam.interface_type == InterfaceType.CAMERA_LINK and self.ci_cl == None:
            self.ci_cl = self.lib.SVSCapture_new(cam.interface_type)
            assert self.ci_cl != None
        elif cam.interface_type == InterfaceType.GIG_E and self.ci_gige == None:
            self.ci_gige = self.lib.SVSCapture_new(cam.interface_type)
            assert self.ci_gige != None
        elif cam.interface_type == InterfaceType.USB3 and self.ci_usb3 == None:
            self.ci_usb3 = self.lib.SVSCapture_new(cam.interface_type)
            assert self.ci_usb3 != None

    def _get_camera_interface(self, cam):
        if cam.interface_type == InterfaceType.CAMERA_LINK:
            return self.ci_cl
        elif cam.interface_type == InterfaceType.GIG_E:
            return self.ci_gige
        elif cam.interface_type == InterfaceType.USB3:
            return self.ci_usb3

    def set_setting(self, cam, name, value):
        settingDataType = self._get_setting_data_type(name)

        if settingDataType == CameraSettingDataType.INT:
            self.lib.SVSCapture_set_feature_int.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
            self.lib.SVSCapture_set_feature_int.restype = ctypes.c_void_p
            self.lib.SVSCapture_set_feature_int(self.obj, cam.index, str_to_bytearray(name), value)
        elif settingDataType == CameraSettingDataType.FLOAT:
            self.lib.SVSCapture_set_feature_float.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_double]
            self.lib.SVSCapture_set_feature_float.restype = ctypes.c_void_p
            self.lib.SVSCapture_set_feature_float(self.obj, cam.index, str_to_bytearray(name), value)
        elif settingDataType == CameraSettingDataType.ENUM:
            self.lib.SVSCapture_set_feature_enum.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
            self.lib.SVSCapture_set_feature_enum.restype = ctypes.c_void_p
            self.lib.SVSCapture_set_feature_enum(self.obj, cam.index, str_to_bytearray(name), value)
        else:
            print("WARNING: Camera setting {} requires custom code.".format(name))

    def _get_setting_data_type(self, name):
        if name in CameraSettings.get_enums():
            return CameraSettingDataType.ENUM
        elif name in CameraSettings.get_floats():
            return CameraSettingDataType.FLOAT
        elif name in CameraSettings.get_ints():
            return CameraSettingDataType.INT
        else:
            return None

    @timethis
    def start_acq(self, cam):
        self.lib.SVSCapture_start_acq.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.lib.SVSCapture_start_acq.restype = ctypes.c_void_p
        self.lib.SVSCapture_start_acq(self._get_camera_interface(cam), cam.index)

    def ramp_up(self, cam):
        time_sec = 3
        print("ramping up for {} seconds...".format(str(time_sec)))
        size = cam.image_width * cam.image_height * cam.image_channels
        _ = (ctypes.c_ubyte * size)()

        self.lib.SVSCapture_get_image.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_ubyte)]
        self.lib.SVSCapture_get_image.restype = ctypes.c_int

        time.sleep(time_sec)
        res = self.lib.SVSCapture_get_image(self._get_camera_interface(cam), cam.index, _)
    
    @timethis
    def get_image(self, cam):
        timeout_ms = 1000
        image = Image(cam)
        image.data = (ctypes.c_ubyte * (cam.image_width * cam.image_height * cam.image_channels))()

        self.lib.SVSCapture_get_image.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_ubyte)]
        self.lib.SVSCapture_get_image.restype = ctypes.c_int
        
        res = -1
        retries = 0
        time_start = time.time() * 1000
        while time.time() * 1000 < time_start + timeout_ms:
            res = self.lib.SVSCapture_get_image(self._get_camera_interface(cam), cam.index, image.data)
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

    @timethis
    def stop_acq(self, cam):
        self.lib.SVSCapture_stop_acq.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.lib.SVSCapture_stop_acq.restype = ctypes.c_void_p
        self.lib.SVSCapture_stop_acq(self._get_camera_interface(cam), cam.index)

    def close(self):
        self.lib.SVSCapture_close.argtypes = [ctypes.c_void_p]
        self.lib.SVSCapture_close.restype = ctypes.c_void_p
        # change add methods for all interfaces
        self.lib.SVSCapture_close(self.ci_usb3)
        self.ci_cl = None
        self.ci_gige = None
        self.ci_usb3 = None

    def __exit__(self, type, value, traceback):
        if self.ci_usb3:
            self.close()

        if self.lib:
            self.lib = None