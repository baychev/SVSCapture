from functools import wraps
from constants import dll_path

import ctypes
import cv2
import numpy as np
import os
import time

# timing tracer flag
trace_execution = True

def timethis(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        if trace_execution:
            start = time.time()
            result = func(*args, **kwargs)
            end = time.time()
            print('method {0} : {1} seconds.'.format(func.__name__, round(end - start, 3)))
        else:
            result = func(*args, **kwargs)

        return result
    return wrapper

def check_dll():
    if os.path.isfile(dll_path):
        return True
    else:
        raise Exception("SVSPythonAPI.dll is not present at relative file path: {}".format(dll_path))

def check_env_variables():
    pass

def save_image(image, file_path="C:\\images"):
    if not os.path.exists(file_path):
        os.mkdir(file_path)
        
    mat = np.asarray(image.data, dtype=np.uint8).reshape(image.height, image.width, image.channels)
    cv2.imwrite('{0}\\cv2_{1}.png'.format(file_path, image.timestamp), mat)