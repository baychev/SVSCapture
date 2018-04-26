from functools import wraps

import ctypes
import cv2
import numpy as np
import os.path
import time

path = "x64\\Release\\SVSCapture.dll"

if os.path.isfile(path):
    lib = ctypes.cdll.LoadLibrary(path)
else:
    msg = 'DLL not present at relative file path: ' + path
    raise Exception(msg)

enable_timing = False

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

# NOTE: enum.Enum is inconsistent
class LibraryType(object):
    GIG_E = 0
    USB3 = 1
    CAMERA_LINK= 2

class ShootingMode(object):
    CONTINUOUS = 0
    SOFTWARE_TRIGGER = 1

class Image(object):
    def __init__(self, timestamp=-1, data=None, height=1080, width=1920, camera_sn=None):
        self.timestamp = timestamp
        self.data = data
        self.width = width
        self.height = height
        self.channels = 3
        self.camera_serial_number = camera_sn

    @timethis
    def save(self, dir="C:\\images\\"):
        if not os.path.exists(dir):
            os.mkdir(dir)
        
        mat = np.asarray(self.data, dtype=np.uint8).reshape(self.height, self.width, self.channels)
        cv2.imwrite('{0}cv2_{1}.png'.format(dir, self.timestamp), mat)

class Camera(object):
    index = -1
    name = None
    serial_number = None
    image_channels = 3

    def __init__(self, serial_number, shooting_mode=ShootingMode.SOFTWARE_TRIGGER, image_height=1080, image_width=1920, exposure_time=2000):
        self.serial_number = serial_number
        
        # Safeguard for serial_number placeholder. Delete if yours is not numeric.
        int(self.serial_number)
        
        self.image_height = image_height
        self.image_width = image_width
        self.shooting_mode = shooting_mode
        self.exposure_time = exposure_time

class SVSCapture(object):
    def __init__(self, libType):
        lib.SVSCapture_new.argtypes = [ctypes.c_int]
        lib.SVSCapture_new.restype = ctypes.c_void_p

        lib.SVSCapture_set_feature_enum.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        lib.SVSCapture_set_feature_enum.restype = ctypes.c_void_p

        lib.SVSCapture_set_feature_int.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
        lib.SVSCapture_set_feature_int.restype = ctypes.c_void_p

        lib.SVSCapture_set_feature_float.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_double]
        lib.SVSCapture_set_feature_float.restype = ctypes.c_void_p

        self.obj = lib.SVSCapture_new(libType)

    @timethis
    def register_camera(self, cam):
        # add device to list
        # NOTE: Camera index is not a good reference, serial number shall be used for robustness.
        lib.SVSCapture_reg_camera.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.SVSCapture_reg_camera.restype = ctypes.c_int16

        cam.index = lib.SVSCapture_reg_camera(self.obj, cam.serial_number.encode('ascii'))

        # set up shooting properties
        # NOTE: Only software trigger has been tested.
        #       Coninuous mode shall provide access to the whole image buffer.
        lib.SVSCapture_set_feature_enum(self.obj, cam.index, "TriggerMode".encode('ascii'), cam.shooting_mode)
        lib.SVSCapture_set_feature_int(self.obj, cam.index, "Height".encode('ascii'), cam.image_height)
        lib.SVSCapture_set_feature_int(self.obj, cam.index, "Width".encode('ascii'), cam.image_width)
        lib.SVSCapture_set_feature_float(self.obj, cam.index, "ExposureTime".encode('ascii'), cam.exposure_time)

        return cam.index >= 0

    @timethis
    def start_acq(self, cam):
        lib.SVSCapture_start_acq.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.SVSCapture_start_acq.restype = ctypes.c_void_p
        lib.SVSCapture_start_acq(self.obj, cam.index)

    def ramp_up(self, cam):
        time_sec = 3
        print("ramping up for {} seconds...".format(str(time_sec)))
        size = cam.image_width * cam.image_height * cam.image_channels
        _ = (ctypes.c_ubyte * size)()

        lib.SVSCapture_get_image.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_ubyte)]
        lib.SVSCapture_get_image.restype = ctypes.c_int

        time.sleep(time_sec)
        res = lib.SVSCapture_get_image(self.obj, cam.index, _)
    
    @timethis
    def get_image(self, cam):
        timeout_ms = 1000
        image = Image(height=cam.image_height, width=cam.image_width, camera_sn=cam.serial_number)
        image.data = (ctypes.c_ubyte * (cam.image_width * cam.image_height * cam.image_channels))()

        lib.SVSCapture_get_image.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_ubyte)]
        lib.SVSCapture_get_image.restype = ctypes.c_int
        
        res = -1
        retries = 0
        time_start = time.time() * 1000
        while time.time() * 1000 < time_start + timeout_ms:
            res = lib.SVSCapture_get_image(self.obj, cam.index, image.data)
            if res == 0:
                if retries > 0:
                    print('Got an image with {} retries'.format(retries))
                break
            
            retries +=1
        if res != 0:
            print('Could not obtain an image from {} retries.'.format(retries))
  
        image.timestamp = int(time.time() * 1000)
        return image

    @timethis
    def stop_acq(self, cam):
        lib.SVSCapture_stop_acq.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.SVSCapture_stop_acq.restype = ctypes.c_void_p
        lib.SVSCapture_stop_acq(self.obj, cam.index)

    def close(self):
        lib.SVSCapture_close.argtypes = [ctypes.c_void_p]
        lib.SVSCapture_close.restype = ctypes.c_void_p
        lib.SVSCapture_close(self.obj)
        self.obj = None
