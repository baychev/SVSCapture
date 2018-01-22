#pragma once

using namespace std;

class SVCam {

public:
	SVCam(string sn);
	~SVCam();
	void close();
	int get_image(char* buffer);
	void init();
	char* test();

private:
	string serial_number;
	void start_acq();
	void stop_acq();
};

extern "C"
{
	SVCam* SVCam_new(string sn) { return new SVCam(sn); };
	void SVCam_close(SVCam* sv_cam) { sv_cam->close(); };
	int SVCam_get_image(SVCam* sv_cam, char* buffer) { sv_cam->get_image(buffer); };
	void SVCam_init(SVCam* sv_cam) { sv_cam->init(); };
	char* SVCam_test(SVCam* sv_cam) { sv_cam->test(); };
}

