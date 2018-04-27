from functools import wraps

import ctypes
import os
import time

enable_timing = True

# MIGRATED
def timethis(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        end = time.time()

        if enable_timing:
            print('method {0} : {1} seconds.'.format(func.__name__, round(end - start, 3)))

        return result
    return wrapper

# MIGRATED
class InterfaceType(object):
    GIG_E = 0
    USB3 = 1
    CAMERA_LINK= 2

# MIGRATED
class TriggerMode(object):
    CONTINUOUS = 0
    SOFTWARE_TRIGGER = 1

# MIGRATED
class CameraSettingDataType(object):
    INT = 1
    FLOAT = 2
    ENUM = 3

# MIGRATED
class Preferences(object):
    ACQUISITION_TIMEOUT = "AcquisitionTimeout"
    CHANNELS = "Channels"
    EXPOSURE_TIME = "ExposureTime"
    HEIGHT = "Height"
    TRIGGER_MODE = "TriggerMode"
    WIDTH = "Width"

    # camera settable preferences
    def get_prefs_enum(self):
        return [self.TRIGGER_MODE]

    def get_prefs_float(self):
        return [self.EXPOSURE_TIME]

    def get_prefs_int(self):
        return [self.HEIGHT, self.WIDTH]

class SVSCapture(object):
    cameras = []
    camera_interfaces = []
    lib = None
    obj = None

    def __init__(self):
        #assert os.environ["SVS_SDK_PATH++++"]
        dll_path = "C:\\Users\\Baychev\\source\\repos\\SVSCapture\\x64\\Release\\SVSCapture.dll"

        if os.path.isfile(dll_path):
            self.lib = ctypes.cdll.LoadLibrary(dll_path)
        else:
            msg = "SVSPythonAPI extension/wrapper DLL is not present at file system path:{}".format(dll_path)
            raise Exception(msg)

    def __enter__(self):
        return self

    def register_interface(self, interfaceType):
        # TODO: support multiple interfaces in the future.
        self.lib.SVSCapture_new.argtypes = [ctypes.c_int]
        self.lib.SVSCapture_new.restype = ctypes.c_void_p
        # add to camera_interfaces
        self.obj = self.lib.SVSCapture_new(interfaceType)

    """
    Invoke before using the camera.
    """
    def register_camera(self, cam):
        exists = False
        for c in self.cameras:
            if c.serial_number == cam.serial_number:
                print("Camera with serial_number {} already registered!".format(cam.serial_number))
                exists = True
                break

        if not exists:
            # TODO: check camera interface type and load a library instance for that type.
            print("adding camera with serial_number {}...".format(cam.serial_number))
            # add device to list
            # NOTE: Camera index is not a good reference, serial number shall be used for robustness.
            self.lib.SVSCapture_reg_camera.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
            self.lib.SVSCapture_reg_camera.restype = ctypes.c_int16
            cam.index = self.lib.SVSCapture_reg_camera(self.obj, cam.serial_number.encode('ascii'))
            # set user defined runtime camera preferences
            self.set_preferences(cam)
            self.cameras.append(cam)

    def set_preferences(self, cam):
        for p in Preferences().get_prefs_enum():
            self._set_preference(cam, p, CameraSettingDataType.ENUM)
        for p in Preferences().get_prefs_float():
            self._set_preference(cam, p, CameraSettingDataType.FLOAT)
        for p in Preferences().get_prefs_int():
            self._set_preference(cam, p, CameraSettingDataType.INT)

        self._set_acq_timeout(cam, cam.preferences[Preferences.ACQUISITION_TIMEOUT])

    def _set_preference(self, cam, name, settingDataType):
        if settingDataType == CameraSettingDataType.INT:
            self.lib.SVSCapture_set_feature_int.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
            self.lib.SVSCapture_set_feature_int.restype = ctypes.c_void_p
            self.lib.SVSCapture_set_feature_int(self.obj, cam.index, name.encode('ascii'), cam.preferences[name])
        elif settingDataType == CameraSettingDataType.FLOAT:
            self.lib.SVSCapture_set_feature_float.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_double]
            self.lib.SVSCapture_set_feature_float.restype = ctypes.c_void_p
            self.lib.SVSCapture_set_feature_float(self.obj, cam.index, name.encode('ascii'), cam.preferences[name])
        elif settingDataType == CameraSettingDataType.ENUM:
            self.lib.SVSCapture_set_feature_enum.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
            self.lib.SVSCapture_set_feature_enum.restype = ctypes.c_void_p
            self.lib.SVSCapture_set_feature_enum(self.obj, cam.index, name.encode('ascii'), cam.preferences[name])

    def _set_acq_timeout(self, cam, value):
        self.lib.SVSCapture_set_acq_timeout.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
        self.lib.SVSCapture_set_acq_timeout.restype = ctypes.c_void_p
        self.lib.SVSCapture_set_acq_timeout(self.obj, cam.index, value)

    def start_acq(self, cam):
        self.lib.SVSCapture_start_acq.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.lib.SVSCapture_start_acq.restype = ctypes.c_void_p
        self.lib.SVSCapture_start_acq(self.obj, cam.index)

    def get_image(self, cam):
        image = Image(cam)
        image.data = (ctypes.c_ubyte * (image.width * image.height * image.channels))()
        self.lib.SVSCapture_get_image.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_ubyte)]
        self.lib.SVSCapture_get_image.restype = ctypes.c_int

        res = -1
        retries = 0
        time_start = time.time() * 1000
        while time.time() * 1000 < time_start + cam.preferences[Preferences.ACQUISITION_TIMEOUT]:
            res = self.lib.SVSCapture_get_image(self.obj, cam.index, image.data)
            if res == 0:
                if retries > 0:
                    print('Got an image with {} retries'.format(retries))
                break
            else:
                print("res: {}".format(res))
            retries +=1
        if res != 0:
            print('Could not obtain an image from {} retries.'.format(retries))
  
        image.timestamp = int(time.time() * 1000)
        return image

    def stop_acq(self, cam):
        self.lib.SVSCapture_stop_acq.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.lib.SVSCapture_stop_acq.restype = ctypes.c_void_p
        self.lib.SVSCapture_stop_acq(self.obj, cam.index)

    def __exit__(self, type, value, traceback):
        # release DLLs
        if self.obj:
            self.lib.SVSCapture_close.argtypes = [ctypes.c_void_p]
            self.lib.SVSCapture_close.restype = ctypes.c_void_p
            self.lib.SVSCapture_close(self.obj)
            self.obj = None

        if self.lib:
            self.lib = None

# MIGRATED
class Camera(object):
    # TODO: get reference by SN
    index = -1
    serial_number = None
    preferences = {}

    def __init__(self, serial_number):
        self.serial_number = serial_number
        # remove if SN is not numeric.
        assert int(self.serial_number) > 0
        # user preferences
        self.preferences[Preferences.TRIGGER_MODE] = TriggerMode.SOFTWARE_TRIGGER
        self.preferences[Preferences.WIDTH] = 600
        self.preferences[Preferences.HEIGHT] = 300
        self.preferences[Preferences.CHANNELS] = 3
        self.preferences[Preferences.EXPOSURE_TIME] = 20000
        self.preferences[Preferences.ACQUISITION_TIMEOUT] = 100

    def load_preferences(self, file_path):
        print("load_settings is not implemented yet!")
        pass

    def print_settings(self):
        print("print_settings is not implemented yet!")
        pass

# MIGRATED
class Image(object):
    data = None
    timestamp = -1
    status_code = None

    def __init__(self, cam):
        self.width = cam.preferences[Preferences.WIDTH]
        self.height = cam.preferences[Preferences.HEIGHT]
        self.channels = cam.preferences[Preferences.CHANNELS]
        self.camera_serial_number = cam.serial_number

    @timethis
    def save(self, dir="C:\\images\\"):
        if not os.path.exists(dir):
            os.mkdir(dir)

        mat = np.asarray(self.data, dtype=np.uint8).reshape(self.height, self.width, self.channels)
        cv2.imwrite('{0}cv2_{1}.png'.format(dir, self.timestamp), mat)