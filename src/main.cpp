#include "h264.h"

int main(int argc, char** argv) {
    const char* cmd = "adb exec-out \"while true; do screenrecord --output-format=h264 -; done\"";
    FILE* f = nullptr;
#ifdef WIN32
    f = _popen(cmd, "rb");
#else 
    f = popen(cmd, "r");
#endif
    
    H264_Stream_Decode decoder(f);
    cv::Mat frame;
    while (decoder.next()) {
        frame = decoder.get_frame();
        cv::imshow("", frame);
        cv::waitKey(1);
    }

    return 0;
}
