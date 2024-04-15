#include <opencv2/opencv.hpp>
#include <iostream>
#include <string.h>

using namespace cv;
using namespace std;

// 函数声明
bool isOverlapping(const cv::Rect& a, const cv::Rect& b);
cv::Rect mergeBoxes(const std::vector<cv::Rect>& boxes);
cv::Mat detectAndDrawFire(cv::Mat img, double minArea);
bool isImageFile(const string& filename);
int getFourccFromExtension(const string& extension);

int main() {
    int minArea = 10; // 设置最小的框面积

    // 载入图片
	string filename = "fire0.mp4";
    // string filename = "camera";
    // cv::Mat img = imread("./img/" + filename);

    string newFilename;
    string extensionPart;
	// 查找文件名中最后一个点（.）的位置
    size_t dotPosition = filename.rfind('.');
    if (dotPosition != string::npos) {
        // 分割文件名和扩展名
        string namePart = filename.substr(0, dotPosition);
        extensionPart = filename.substr(dotPosition);

        // 构建新的文件名
        newFilename = namePart + "_output" + extensionPart;

        // 保存修改后的图像
	}

   if (isImageFile(filename)) {
        Mat img = imread("./img/" + filename);
        if (img.empty()) {
            cout << "Could not read the image: " << filename << endl;
            return -1;
        }
        img = detectAndDrawFire(img, minArea);
        imshow("Processed Image", img);
        imwrite("./img/" + newFilename, img);
        waitKey(0); // 等待按键
    }
else {
    VideoCapture cap;
    if (filename == "camera") {  
        cap.open(0); // Open the default camera
        newFilename = "camera.avi";
    } else {
        cap.open("F:/c_work/FireDetection/img/" + filename); // Try to open the video file
    }

    if (!cap.isOpened()) {
        cout << "Error opening video source." << endl;
        return -1;
    }
    VideoWriter writer;
    int fourcc = getFourccFromExtension(extensionPart);    // 定义编码格式
    // int fourcc = static_cast<int>(cap.get(CAP_PROP_FOURCC));
    double fps = cap.get(CAP_PROP_FPS);
    // double fps = 25.0;
    Size frameSize(cap.get(CAP_PROP_FRAME_WIDTH), cap.get(CAP_PROP_FRAME_HEIGHT));
    writer.open("./img/" + newFilename, fourcc, fps, frameSize, true);

    Mat frame;
    int times = 0;
    while (true) {
        cap >> frame;
        if (frame.empty()) break;
        frame = detectAndDrawFire(frame, minArea);
        writer.write(frame); // Write the frame into the file/output stream
        imshow("Frame", frame);
        if (waitKey(1) == 27) break; // Exit on ESC
    }
}

    return 0;
}


cv::Mat detectAndDrawFire(cv::Mat img, double minArea) {
     // 转换到HSV色彩空间
    if(img.empty()) return img;
    cv::Mat imgHSV;
    cv::cvtColor(img, imgHSV, COLOR_BGR2HSV);
    imshow("imgHSV", imgHSV);

    // 定义火焰颜色的HSV范围
    Scalar lower_yellow(0, 160, 160);
    Scalar upper_yellow(60, 255, 255);

    // 阈值化得到火焰颜色的掩码
    cv::Mat flameMask;
    cv::inRange(imgHSV, lower_yellow, upper_yellow, flameMask);
    imshow("flameMask", flameMask);

    // 形态学开运算去除小噪声
    cv::Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(1, 1));
    cv::morphologyEx(flameMask, flameMask, MORPH_OPEN, kernel);

    // 使用findContours找到火焰区域
    vector<vector<Point>> contours;
    findContours(flameMask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    std::vector<cv::Rect> boxes;
    for (size_t i = 0; i < contours.size(); i++) {
        if (cv::contourArea(contours[i]) >= minArea) {
            boxes.push_back(cv::boundingRect(contours[i]));
        }
    }

    // 融合重叠框
    int nums = 2 * boxes.size();
    for(int i  = 0; i < nums; i++) {
        cout << i << "    ";
        cv::Rect box = boxes.front();
        boxes.erase(boxes.begin());
        std::vector<cv::Rect> boxesToMerge = {box};

        auto it = boxes.begin();
        while (it != boxes.end()) {
            if (isOverlapping(box, *it)) {
                boxesToMerge.push_back(*it);
                it = boxes.erase(it); // 从boxes中移除并加入到boxesToMerge
            } else {
                ++it; // 不重叠，继续检查下一个
            }
        }

        // 合并所有重叠的边界框
        cv::Rect mergedBox = mergeBoxes(boxesToMerge);
        boxes.push_back(mergedBox); // 将合并后的边界框存入输出列表
    }

    for (const auto& contour : boxes) {
        //double area = contourArea(contour); // 计算当前轮廓的面积
        double area = contour.width * contour.height; 

        if (area > minArea) { // 如果面积大于最小阈值
            //Rect boundingBox = boundingRect(contour); // 获取轮廓的边界框
            rectangle(img, contour, Scalar(0, 255, 0), 2); // 绘制边界框
        }
    }

    return img;
}


bool isImageFile(const string& filename) {
    // 图像文件的常见扩展名列表
    const vector<string> imageExtensions = {".jpg", ".jpeg", ".png", ".bmp", ".gif"};
    size_t dotPos = filename.rfind('.');
    if (dotPos == string::npos) return false; // 没有找到扩展名
    string ext = filename.substr(dotPos);
    for (const auto& imageExt : imageExtensions) {
        if (ext == imageExt) return true;
    }
    return false;
}

bool isOverlapping(const cv::Rect& a, const cv::Rect& b) {
    return (a & b).area() > 0;
}

cv::Rect mergeBoxes(const std::vector<cv::Rect>& boxes) {
    for(auto& box : boxes){
        cout << box;
    }
    
    if (boxes.empty()) {
        return cv::Rect();
    }

    // 初始化为第一个矩形的坐标和尺寸
    int min_x = boxes[0].x;
    int min_y = boxes[0].y;
    int max_x = boxes[0].x + boxes[0].width;
    int max_y = boxes[0].y + boxes[0].height;

    // 遍历boxes更新min_x, min_y, max_x, max_y
    for (const auto& box : boxes) {
        min_x = std::min(min_x, box.x);
        min_y = std::min(min_y, box.y);
        max_x = std::max(max_x, box.x + box.width);
        max_y = std::max(max_y, box.y + box.height);
    }

    // 计算融合后矩形的宽度和高度
    int width = max_x - min_x;
    int height = max_y - min_y;

    // 返回融合后的矩形
    return cv::Rect(min_x, min_y, width, height);
}

int getFourccFromExtension(const string& extension) {
    if (extension == ".avi") {
        // 通常用于 AVI 文件的编解码器
        return VideoWriter::fourcc('M', 'J', 'P', 'G');
    } else if (extension == ".mp4") {
        // 一些环境下，MP4 文件可以使用这些编解码器
        // 注意：在 Windows 上使用 'M', 'P', '4', '2' 或 'D', 'I', 'V', 'X'
        // 在 Linux 或 MacOS 上，可以尝试 'X', '2', '6', '4' 或 'a', 'v', 'c', '1'
        // 具体可用的编码依赖于你的系统和OpenCV配置
        // #ifdef _WIN32
        return VideoWriter::fourcc('X', 'V', 'I', 'D');
    } else if (extension == ".mkv") {
        // MKV 文件可以尝试使用 'X', '2', '6', '4'
        return VideoWriter::fourcc('X', '2', '6', '4');
    }
    // 默认返回 -1，表示未找到匹配的编解码器
    return -1;
}