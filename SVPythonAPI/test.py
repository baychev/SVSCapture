import constants as sv_const
import os
import svcaptureapi as sv
import time

_serial_number = "your_serial_number"

def test_UnitTest():
        print("test_UnitTest: Running a unit test with assertions...\n")
        with sv.SVCapture() as sv_cap:
            cam = sv.Camera(_serial_number)
            status = sv_cap.register_camera(cam)
            assert status
            # This is a time expensive operation.
            sv_cap.start_acq(cam)
            # Ramp up camera, buffer is empty upon start.
            sv_cap.ramp_up(cam)
            image = sv_cap.get_image(cam)
            # Save with OpenCV code, pip install it beforehand.
            image.save()
            assert os.path.isfile("C:\\images\\cv2_{}.png".format(image.timestamp))
            # Read a setting
            width = 0
            width = sv_cap.get_setting(cam, sv_const.CameraSettings.WIDTH)
            assert width > 0
            # Write a setting, comment out to test
            sv_cap.set_setting(cam, sv_const.CameraSettings.TRIGGER_MODE, sv_const.TriggerMode.OFF)
            trigger_mode = sv_cap.get_setting(cam, sv_const.CameraSettings.TRIGGER_MODE)
            assert trigger_mode == sv_const.TriggerMode.OFF
            # This is a time expensive operation.
            sv_cap.stop_acq(cam)

        print("Unit test run finished.")

def test_GetImage():
    print("test_GetImage: Get an image with a software trigger. Save with OpenCV.\n")
    with sv.SVCapture() as sv_cap:
        cam = sv.Camera(_serial_number)
        status = sv_cap.register_camera(cam)
        sv_cap.start_acq(cam)
        sv_cap.ramp_up(cam)
        image = sv_cap.get_image(cam)
        image.save()
        sv_cap.stop_acq(cam)
        print("test completed.")

def test_CameraSettings():
        print("test_CameraSettings: Read and write a camera setting.\n")
        with sv.SVCapture() as sv_cap:
            cam = sv.Camera(_serial_number)
            status = sv_cap.register_camera(cam)
            # Read a setting
            exposure_before = sv_cap.get_setting(cam, sv_const.CameraSettings.EXPOSURE_TIME)
            assert exposure_before >= 2000
            delta = 100
            # Write a setting
            sv_cap.set_setting(cam, sv_const.CameraSettings.EXPOSURE_TIME, exposure_before+delta)
            exposure_after = sv_cap.get_setting(cam, sv_const.CameraSettings.EXPOSURE_TIME)
            assert exposure_before + delta == exposure_after
            print("test completed.")

def test_FrameAvailability():
    print("test_FrameAvailability: Test for frame losses...\n")
    with sv.SVCapture() as sv_cap:
        cam = sv.Camera(_serial_number)
        status = sv_cap.register_camera(cam)
        sv_cap.start_acq(cam)
        sv_cap.ramp_up(cam)
        steps = 100
        skips = 0
        for i in range(0, steps):
            time.sleep(0.1) # This needs to take into account camera frame rate!
            image = sv_cap.get_image(cam)
            if image.status_code != 0:
                skips +=1
        print("---> result: {0} lost frames from {1} grabbed frames.".format(skips, steps))
        sv_cap.stop_acq(cam)
        print("test completed.")

if __name__ == '__main__':
    # NOTE: Test with just one method at a time.
    #       Garbage collection on the DLL pointer is slow and 
    #       fast loading/unloading of the DLL does not coincide with a real world use case scenario.
    
    #test_GetImage()
    #test_CameraSettings()
    #test_FrameAvailability()
    test_UnitTest()