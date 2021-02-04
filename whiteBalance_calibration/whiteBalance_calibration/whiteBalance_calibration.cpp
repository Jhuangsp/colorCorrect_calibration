

#include <conio.h>
#include <iostream>
#include <filesystem>
#include <string>
#include "utils.h"
#include <VimbaC/Include/VimbaC.h>
#include <VimbaCPP/Include/VimbaCPP.h>
using namespace AVT::VmbAPI;
namespace fs = std::experimental::filesystem;

// 1.define observer that reacts on new frames
class FrameObserver : public IFrameObserver
{
public:
    // In your contructor call the constructor of the base class
    // and pass a camera objectt
    string name;
    cv::Mat cvMat;
    cv::Mat colorMat;
    cv::Mat show;
    CameraPtr pCamera;
    FrameObserver(CameraPtr pCamera, string s) : IFrameObserver(pCamera)
    {
        // Put your initialization code here
        cout << "Init FrameObserver" << endl;
        name = s;
        this->pCamera = pCamera;
    }

    void FrameReceived(const FramePtr pFrame)
    {
        VmbFrameStatusType eReceiveStatus;
        if (VmbErrorSuccess == pFrame->GetReceiveStatus(eReceiveStatus))
        {
            //cout << "Frame received" << endl;
            if (VmbFrameStatusComplete == eReceiveStatus)
            {
                // Put your code here to react on a successfully received frame
                //cout << "Frame received successfully" << endl;
                VmbUchar_t *pImage = NULL;
                VmbUint32_t timeout = 500;
                VmbUint32_t nWidth = 10;
                VmbUint32_t nHeight = 10;
                if (VmbErrorSuccess != pFrame->GetWidth(nWidth))
                    cout << "FAILED to aquire width of frame!" << endl;

                if (VmbErrorSuccess != pFrame->GetHeight(nHeight))
                    cout << "FAILED to aquire height of frame!" << endl;

                if (VmbErrorSuccess != pFrame->GetImage(pImage))
                    cout << "FAILED to acquire image data of frame!" << endl;
                cvMat = cv::Mat(nHeight, nWidth, CV_8UC1, pImage);
                cv::cvtColor(cvMat, colorMat, CV_BayerBG2BGR);
                cv::resize(colorMat, show, cv::Size(1080, 720));

                imshow("Our Great Window" + name, show);
                //imwrite("C:\\Users\\NOL\\Desktop\\" + name + ".png", cvMat);
                cv::waitKey(1);
            }
            else
            {
                // Put your code here to react on an unsuccessfully received frame
                cout << "Frame received failed" << endl;
            }
        }
        // When you are finished copying the frame , re-queue it
        m_pCamera->QueueFrame(pFrame);
    }
};

int collectImg(string path) {
    VmbErrorType err;                              // Every Vimba function returns an error code that
                                                   // should always be checked for VmbErrorSuccess
    VimbaSystem &sys = VimbaSystem::GetInstance(); // A reference to the VimbaSystem singleton
    CameraPtrVector cameras;                       // A list of known cameras
    FramePtrVector frames0(3);                     // A list of frames for streaming. We chose
                                                   // to queue 3 frames.

    err = sys.Startup();
    // Check version
    VmbVersionInfo_t m_VimbaVersion;
    err = sys.QueryVersion(m_VimbaVersion);
    if (err != VmbErrorSuccess) {
        cerr << "CamCtrlVmbAPI::Init : version Query failed: " << err << endl;
        sys.Shutdown();
        exit(-2);
    }
    cout << "Vimba " << m_VimbaVersion.major << "." << m_VimbaVersion.minor
        << " initialized" << endl << endl;
    // Open cameras
    err = sys.GetCameras(cameras);
    FeaturePtr pFeature;       // Any camera feature
    VmbInt64_t nPLS0;   // The payload size of one frame

    err = cameras[0]->Open(VmbAccessModeFull);

    // Set action trigger mode
    err = cameras[0]->GetFeatureByName("TriggerSelector", pFeature);
    err = pFeature->SetValue("FrameStart");
    err = cameras[0]->GetFeatureByName("TriggerSource", pFeature);
    err = pFeature->SetValue("Action0");
    err = cameras[0]->GetFeatureByName("TriggerMode", pFeature);
    err = pFeature->SetValue("On");

    // Set Action Command to camera
    int deviceKey = 11, groupKey = 22, groupMask = 33;
    cameras[0]->GetFeatureByName("ActionDeviceKey", pFeature);
    pFeature->SetValue(deviceKey);
    cameras[0]->GetFeatureByName("ActionGroupKey", pFeature);
    pFeature->SetValue(groupKey);
    cameras[0]->GetFeatureByName("ActionGroupMask", pFeature);
    pFeature->SetValue(groupMask);

    err = cameras[0]->GetFeatureByName("PayloadSize", pFeature);
    err = pFeature->GetValue(nPLS0);

    // Attach observer
    FrameObserver *pOb0;
    pOb0 = new FrameObserver(cameras[0], "0");
    IFrameObserverPtr pObserver0(pOb0);
    for (FramePtrVector::iterator iter0 = frames0.begin();
        frames0.end() != iter0;
        ++iter0) {
        (*iter0).reset(new Frame(nPLS0));
        err = (*iter0)->RegisterObserver(pObserver0); // 2. Register the observer before queuing the frame
        err = cameras[0]->AnnounceFrame(*iter0);
    }

    // StartCapture
    err = cameras[0]->StartCapture();
    // QueueFrame
    for (FramePtrVector::iterator iter0 = frames0.begin();
        frames0.end() != iter0;
        ++iter0) {
        err = cameras[0]->QueueFrame(*iter0);
    }
    // AcquisitionStart
    err = cameras[0]->GetFeatureByName("AcquisitionStart", pFeature);
    err = pFeature->RunCommand();

    // Send Action Command
    char key = 0;
    int count = 0;

    if (!fs::is_directory(path) || !fs::exists(path))
        fs::create_directory(path);
    while (key != 27) // press ESC to exit
    {
        if (key == 32) // press SPACE to pause and store image
        {
            cout << "press Enter to continue" << endl;
            char name[128];
            sprintf_s(name, "%s%02d%s", path, count, ".png");
            //string name0 = path + to_string(count) + ".png";
            //string name0 = path + number + ".png";
            cv::imwrite(name, pOb0->colorMat);
            cin.ignore();
            key = 0;
            count++;
        }
        // Set Action Command to Vimba API
        sys.GetFeatureByName("ActionDeviceKey", pFeature);
        pFeature->SetValue(deviceKey);
        sys.GetFeatureByName("ActionGroupKey", pFeature);
        pFeature->SetValue(groupKey);
        sys.GetFeatureByName("ActionGroupMask", pFeature);
        pFeature->SetValue(groupMask);
        sys.GetFeatureByName("ActionCommand", pFeature);
        pFeature->RunCommand();

        // Get keypress in non-blocking way
        if (_kbhit()) {
            key = _getch();
        }
    }

    // Program runtime ... e.g., Sleep(2000);
    //cin.ignore();

    // When finished , tear down the acquisition chain , close the camera and Vimba
    err = cameras[0]->GetFeatureByName("AcquisitionStop", pFeature);
    err = pFeature->RunCommand();
    err = cameras[0]->EndCapture();
    err = cameras[0]->FlushQueue();
    err = cameras[0]->RevokeAllFrames();
    err = cameras[0]->Close();
    err = sys.Shutdown();
    return 0;
}

int main()
{
    collectImg("data/");
    cv::Mat frame = cv::imread("data/00.png", -1);
    int * avgcolor = PerfectReflectionAlgorithm(frame, "data/");
    cv::Mat show = convertColor(frame, avgcolor);
    cv::imshow("White balance result", show);
    cv::waitKey(0);
    int succ = calibration("data/", cv::Size(9,5), 28.0);
    return 0;
} 
