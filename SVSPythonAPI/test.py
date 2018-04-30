from svsapi import *
from constants import *
from ctypes import *

import time
import unittest

class Test_test1(unittest.TestCase):
    serial_number = "59992"

    def test_Client(self):
        print("This demo shows how to interface with a SVS Vistek USB3 camera.\n")
        with SVSCapture() as sv_cap:
            cam = Camera(serial_number)
            status = sv_cap.register_camera(cam)
            assert status
            # Read a setting
            width = sv_cap.get_setting(cam, CameraSettings.WIDTH)
            assert width > 0
            # Write a setting, comment out to test
            #sv_cap.set_setting(cam, CameraSettings.TRIGGER_MODE, TriggerMode.ON)
            # This is a time expensive operation.
            sv_cap.start_acq(cam)
            # Ramp up camera, buffer is empty upon start.
            sv_cap.ramp_up(cam)
            # Take a few images and save
            for i in range(0, 3):
                image = sv_cap.get_image(cam)
                # Save with OpenCV code, pip install it beforehand.
                #image.save()
            # This is a time expensive operation.
            sv_cap.stop_acq(cam)

        print("Demo finished.")

    def test_FrameAvailability(self):
        print("Testing program for frame losses...")
        with SVSCapture() as sv_cap:
            cam = Camera(serial_number)
            status = sv_cap.register_camera(cam)
            sv_cap.start_acq(cam)
            sv_cap.ramp_up(cam)
            steps = 10
            skips = 0
            for i in range(0, steps):
                time.sleep(0.1) # This needs to take into account camera frame rate!
                image = sv_cap.get_image(cam)
                if image.status_code != 0:
                    skips +=1
            print("---> result: {0} lost frames from {1} grabbed frames.".format(skips, steps))
            sv_cap.stop_acq(cam)

if __name__ == '__main__':
    unittest.main()