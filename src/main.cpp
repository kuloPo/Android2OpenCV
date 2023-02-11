#include "h264.h"

int main(int argc, char** argv) {
	FILE* f = _popen("adb exec-out \"while true; do screenrecord --output-format=h264 -; done\"", "rb");
    
	H264_Stream_Decode decoder(f);
	cv::Mat frame;
	while (decoder.next()) {
		frame = decoder.get_frame();
		cv::imshow("", frame);
		cv::waitKey(1);
	}

    return 0;
}
