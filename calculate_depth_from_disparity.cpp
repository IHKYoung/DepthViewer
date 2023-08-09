#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>

struct CameraParameters
{
    float baseline;
    float fx;
    int disparity_factor;
};

cv::Mat disparity, real_disparity;
cv::Rect roi;
double average_depth = 0.0;
cv::Point mouse_position;
CameraParameters camera;

// 计算深度值
float computeDepth(float disparity_value)
{
    return (camera.baseline * camera.fx) / disparity_value / 1000.0f;
}

// 鼠标回调函数
void onMouse(int event, int x, int y, int flags, void *)
{
    static cv::Point start_point;
    mouse_position = cv::Point(x, y);

    if (event == cv::EVENT_LBUTTONDOWN)
    {
        start_point = mouse_position;
        roi = cv::Rect(start_point, mouse_position);
    }
    else if (event == cv::EVENT_MOUSEMOVE && (flags & cv::EVENT_FLAG_LBUTTON))
    {
        roi = cv::Rect(start_point, mouse_position);
    }
    else if (event == cv::EVENT_LBUTTONUP)
    {
        roi = cv::Rect(start_point, mouse_position);

        // 计算选择区域的平均深度
        int count = 0;
        average_depth = 0.0;
        for (int i = roi.y; i < roi.y + roi.height; i++)
        {
            for (int j = roi.x; j < roi.x + roi.width; j++)
            {
                float disparity_value = real_disparity.at<float>(i, j);
                if (disparity_value != 0)
                { // 避免除以零
                    average_depth += computeDepth(disparity_value);
                    count++;
                }
            }
        }
        average_depth /= count;
    }
}

int main(int argc, char **argv)
{
    // 验证和解析输入参数
    if (argc < 5)
    {
        std::cerr << "Usage: " << argv[0] << " <baseline> <fx> <disparity_factor> <image_path>\n";
        return 1;
    }

    camera.baseline = std::stof(argv[1]);
    camera.fx = std::stof(argv[2]);
    camera.disparity_factor = std::stof(argv[3]);
    std::string image_path = argv[4];

    // 验证参数
    std::cout << "输入参数: " << std::endl;
    std::cout << "baseline: " << std::fixed << std::setprecision(8) << camera.baseline << std::endl;
    std::cout << "fx: " << std::fixed << std::setprecision(8) << camera.fx << std::endl;
    std::cout << "disparity_factor: " << camera.disparity_factor << std::endl;
    std::cout << "image_path: " << image_path << std::endl;

    // 读取视差图
    disparity = cv::imread(image_path, cv::IMREAD_UNCHANGED);

    // 转换为真实视差
    real_disparity = disparity.clone();
    real_disparity.convertTo(real_disparity, CV_32F, 1.0 / camera.disparity_factor);

    // 使用伪彩色可视化
    cv::Mat color_map;
    cv::Mat disparity_norm;
    cv::normalize(disparity, disparity_norm, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::applyColorMap(disparity_norm, color_map, cv::COLORMAP_JET);

    const std::string windowTitle = "视差图";
    cv::namedWindow(windowTitle, cv::WINDOW_NORMAL);
    cv::resizeWindow(windowTitle, disparity.cols, disparity.rows);
    cv::setMouseCallback(windowTitle, onMouse);

    while (true)
    {
        cv::Mat display_image = color_map.clone();
        cv::rectangle(display_image, roi, cv::Scalar(0, 255, 0), 2);

        // 显示平均深度
        std::stringstream ss_avg;
        ss_avg << "Average Depth: " << average_depth << " meters";
        cv::putText(display_image, ss_avg.str(), cv::Point(5, 60), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);

        // 显示实时深度
        float disparity_value = real_disparity.at<float>(mouse_position.y, mouse_position.x);
        if (disparity_value != 0)
        {
            float depth = (camera.baseline * camera.fx) / disparity_value / 1000.0f;
            std::stringstream ss_depth;
            ss_depth << "Depth: " << depth << " meters";
            cv::putText(display_image, ss_depth.str(), cv::Point(5, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        }

        cv::imshow("视差图", display_image);
        char key = (char)cv::waitKey(1);
        if (key == 27)
            break; // 按'Esc'退出
    }

    return 0;
}
