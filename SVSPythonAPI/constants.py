dll_path = "..\\x64\\Release\\SVSCapture.dll"

class InterfaceType(object):
    GIG_E = 0
    USB3 = 1
    CAMERA_LINK = 2

class InterfaceName(object):
    GIG_E = "Gig_E"
    USB3 = "USB3"
    CAMERA_LINK= "CameraLink"

class TriggerMode(object):
    OFF = 0
    ON = 1

class CameraSettingDataType(object):
    INT = 1
    FLOAT = 2
    ENUM = 3
    BOOL = 4
    STR = 5

class CameraSettings(object):
    ACQUISITION_TIMEOUT = "AcquisitionTimeout"
    CHANNELS = "Channels"
    EXPOSURE_TIME = "ExposureTime"
    HEIGHT = "Height"
    TRIGGER_MODE = "TriggerMode"
    WIDTH = "Width"

    """
    Settings of type ENUM
    """
    def get_enums(self):
        return [self.TRIGGER_MODE]

    """
    Settings of type FLOAT
    """
    def get_floats(self):
        return [self.EXPOSURE_TIME]

    """
    Settings of type INT
    """
    def get_ints(self):
        return [self.HEIGHT, self.WIDTH]
