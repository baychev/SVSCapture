from SVSCapture import *
from ctypes import *

print("This is a demo showing how to interface with a SVS Vistek USB3 camera.\n")

# NOTE: Concurrent use of multiple camera interfaces is not supported yet.
#       GigE will not work, setup is not implemented. 
#       CL may work.
sv_cap = SVSCapture(LibraryType.USB3)
print("Library setup complete.\n")

print("Register camera(s).")
# NOTE: Camera default settings: 
#   shooting_mode=ShootingMode.SOFTWARE_TRIGGER, 
#   image_height=1080, 
#   image_width=1920, 
#   exposure_time=2000
cam = Camera(serial_number="your_serial_number", image_height=600, image_width=600, exposure_time=20000)
result = sv_cap.register_camera(cam)
print('---> sn {0} result: {1}'.format(cam.serial_number, result))

if cam.index >= 0:
    # This is a time expensive operation.
    sv_cap.start_acq(cam)
    
    # Ramp up camera
    sv_cap.ramp_up(cam)

    for i in range(0, 3):
        image = sv_cap.get_image(cam)
        image.save() # save using OpenCV

    # This is a time expensive operation.
    sv_cap.stop_acq(cam)
else:
    print("No camera to work with!")

sv_cap.close()
print("Finished.")

