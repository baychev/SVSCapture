from svsapi import *
from constants import *
from ctypes import *

import time

_serial_number = "59992"

print("This demo shows how to interface with a SVS Vistek USB3 camera.\n")

with SVSCapture() as sv_cap:
    cam = Camera(_serial_number)
    # NOTE: Need to set acq timeout through code. It depends on the camera's frame rate.
    status = sv_cap.register_camera(cam)
    assert status
    # Read a setting
    width = sv_cap.get_setting(cam, CameraSettings.WIDTH)
    assert width > 0
    # Write a setting
    #sv_cap.set_setting(cam, CameraSettings.TRIGGER_MODE, TriggerMode.SOFTWARE_TRIGGER)
    # This is a time expensive operation.
    sv_cap.start_acq(cam)
    # Ramp up camera
    sv_cap.ramp_up(cam)
    # Take images and save
    for i in range(0, 1):
        image = sv_cap.get_image(cam)
        #image.save() # save with OpenCV code, install it.

    # Test frame availability with a high number of frame grabs.
    print("performing availability test...")
    steps = 10
    skips = 0
    for i in range(0, steps):
        time.sleep(0.1) # This needs to take account camera frame rate!
        image = sv_cap.get_image(cam)
        if image.status_code != 0:
            skips +=1
    print("---> result: {0} skips in grabing {1} frames.".format(skips, steps))
    # This is a time expensive operation.
    sv_cap.stop_acq(cam)

print("Demo finished.")