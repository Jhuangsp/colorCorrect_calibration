#include "utils.h"

bool is_file_exist(string fileName) {
    std::ifstream infile(fileName);
    return infile.good();
}

cv::Mat convertColor(cv::Mat src, const int* avgcolor) {
    int row = src.rows;
    int col = src.cols;
    cv::Mat dst(row, col, CV_8UC3);
    src.convertTo(src, CV_16UC3);
    src = src.mul(cv::Scalar(avgcolor[3], avgcolor[3], avgcolor[3]));
    src = src.mul(cv::Scalar(1./avgcolor[0], 1./avgcolor[1], 1./avgcolor[2]));
    src.convertTo(dst, CV_8UC3);
    cout << avgcolor[0] << " " << avgcolor[1] << " " << avgcolor[2] << " " << avgcolor[3] << " " << endl;
    return dst;
}

int* PerfectReflectionAlgorithm(cv::Mat src, string path) {
    int row = src.rows;
    int col = src.cols;
    //Mat dst(row, col, CV_8UC3);
    int HistRGB[767] = { 0 };
    int MaxVal = 0;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            MaxVal = max(MaxVal, (int)src.at<cv::Vec3b>(i, j)[0]);
            MaxVal = max(MaxVal, (int)src.at<cv::Vec3b>(i, j)[1]);
            MaxVal = max(MaxVal, (int)src.at<cv::Vec3b>(i, j)[2]);
            int sum = src.at<cv::Vec3b>(i, j)[0] + src.at<cv::Vec3b>(i, j)[1] + src.at<cv::Vec3b>(i, j)[2];
            HistRGB[sum]++;
        }
    }
    int Threshold = 0;
    int sum = 0;
    for (int i = 766; i >= 0; i--) {
        sum += HistRGB[i];
        if (sum > row * col * 0.1) {
            Threshold = i;
            break;
        }
    }
    int AvgB = 0;
    int AvgG = 0;
    int AvgR = 0;
    int cnt = 0;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            int sumP = src.at<cv::Vec3b>(i, j)[0] + src.at<cv::Vec3b>(i, j)[1] + src.at<cv::Vec3b>(i, j)[2];
            if (sumP > Threshold) {
                AvgB += src.at<cv::Vec3b>(i, j)[0];
                AvgG += src.at<cv::Vec3b>(i, j)[1];
                AvgR += src.at<cv::Vec3b>(i, j)[2];
                cnt++;
            }
        }
    }
    AvgB /= cnt;
    AvgG /= cnt;
    AvgR /= cnt;
    //src.convertTo(src, CV_16UC3);
    //src = src.mul(Scalar(MaxVal, MaxVal, MaxVal));
    //src = src.mul(Scalar(1./AvgB, 1./AvgG, 1./AvgR));
    //src.convertTo(dst, CV_8UC3);
    int static param[4] = { AvgB, AvgG, AvgR, MaxVal };
    
    // Write file
    ofstream in;
    in.open(path + "colorCorrection.txt", ios::trunc);
    in << AvgB << endl
        << AvgG << endl
        << AvgR << endl
        << MaxVal << endl;
    in.close();

    return param;
}

int calibration(string path, cv::Size size, float cube_size){
    cv::Mat frame;
    string imgfolder = path + "*.png"; //"calib\\0\\"
    string outfile = path + "cmx_dis.xml";

    // Images list
    vector<cv::String> img_list;
    vector<cv::Mat> images;
    cv::glob(imgfolder, img_list);
    int num_imgs = img_list.size();
    std::cout << "Found " << num_imgs << " images." << endl;
    
    // Calibration loop
    for (int count = 0; count < num_imgs; count++) {
        std::cout << "Reading " + img_list[count] << endl;
        // Get an image
        frame = imread(img_list[count], -1);
        if (frame.empty()) {
            std::cout << "Not found" << endl;
            break;
        }
            
        // Convert to grayscale
        cv::Mat gray;
        cv::cvtColor(frame, gray, CV_BGR2GRAY);
            
        // Detect a chessboard
        //cv::Size size(PAT_COLS, PAT_ROWS);
        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCorners(gray, size, corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);
            
        // Chessboard detected
        int key = 0;
        if (found) {
            // Draw it
            cv::drawChessboardCorners(frame, size, corners, found);
                
            // collect image from stream
            //if (key == 32) { //space
            //    // Add to buffer
            //    images.push_back(gray);
            //}
            images.push_back(gray);
        }
            
        //// Show the image
        //std::ostringstream stream;
        //stream << "Captured " << images.size() << " image(s).";
        //cv::putText(frame, stream.str(), cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1, 1);
        //cv::imshow("Camera Calibration", frame);
        //key = waitKey(0);
    }
        
    // We have enough samples
    if (images.size() > 4) {
        std::cout << "Start calibration" << endl;
        //cv::Size size(PAT_COLS, PAT_ROWS);
        std::vector< std::vector<cv::Point2f> > corners2D;
        std::vector< std::vector<cv::Point3f> > corners3D;
            
        for (size_t i = 0; i < images.size(); i++) {
            // Detect a chessboard
            std::vector<cv::Point2f> tmp_corners2D;
            bool found = cv::findChessboardCorners(images[i], size, tmp_corners2D);
                
            // Chessboard detected
            if (found) {
                // Convert the corners to sub-pixel
                cv::cornerSubPix(images[i], tmp_corners2D, cvSize(11, 11), cvSize(-1, -1), cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 30, 0.1));
                corners2D.push_back(tmp_corners2D);
                    
                // Set the 3D position of patterns
                const float squareSize = cube_size;
                std::vector<cv::Point3f> tmp_corners3D;
                for (int j = 0; j < size.height; j++) {
                    for (int k = 0; k < size.width; k++) {
                        tmp_corners3D.push_back(cv::Point3f((float)(k*squareSize), (float)(j*squareSize), 0.0));
                    }
                }
                corners3D.push_back(tmp_corners3D);
            }
        }
            
        // Estimate camera parameters
        cv::Mat cameraMatrix, distCoeffs;
        std::vector<cv::Mat> rvec, tvec;
        cv::calibrateCamera(corners3D, corners2D, images[0].size(), cameraMatrix, distCoeffs, rvec, tvec);
        std::cout << cameraMatrix << std::endl;
        std::cout << distCoeffs << std::endl;
            
        // Save them
        std::cout << "saving result" << endl;
        if(is_file_exist(outfile))
            remove(outfile.c_str());
        cv::FileStorage tmp(outfile, cv::FileStorage::WRITE);
        tmp << "intrinsic" << cameraMatrix;
        tmp << "distortion" << distCoeffs;
        tmp.release();
            
    }
    
    // Load camera parameters
    std::cout << "Read camera matrix" << endl;
    cv::FileStorage fs;
    fs.open(outfile, cv::FileStorage::READ);
    cv::Mat cameraMatrix, distCoeffs;
    fs["intrinsic"] >> cameraMatrix;
    fs["distortion"] >> distCoeffs;
    
    // Create undistort map
    std::cout << "Create undistort map" << endl;
    cv::Mat mapx, mapy;
    cv::initUndistortRectifyMap(cameraMatrix, distCoeffs, cv::Mat(), cameraMatrix, frame.size(), CV_32FC1, mapx, mapy);
    
    // Main loop
    for (int count = 0; count < images.size(); count++) {
        // Key input
        int key = cv::waitKey(33);
        if (key == 0x1b) break;
        
        // Undistort
        cv::Mat image;
        std::cout << "remap " << count << endl;
        cv::remap(images[count], image, mapx, mapy, cv::INTER_LINEAR);
        std::cout << "remap " << count << endl;
        
        // Display the image
        //cv::imshow("camera", image);
        cv::imwrite(path + to_string(count) + "_gray.png", image);
    }

    cv::destroyAllWindows();
    return 0;
}