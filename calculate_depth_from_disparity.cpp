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
cv::Rect select_roi;
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
        select_roi = cv::Rect(start_point, mouse_position);
    }
    else if (event == cv::EVENT_MOUSEMOVE && (flags & cv::EVENT_FLAG_LBUTTON))
    {
        select_roi = cv::Rect(start_point, mouse_position);
    }
    else if (event == cv::EVENT_LBUTTONUP)
    {
        select_roi = cv::Rect(start_point, mouse_position);

        // 计算选择区域的平均深度
        int count = 0;
        average_depth = 0.0;
        for (int i = select_roi.y; i < select_roi.y + select_roi.height; i++)
        {
            for (int j = select_roi.x; j < select_roi.x + select_roi.width; j++)
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

    // 计算最大和最小视差
    double min_disparity, max_disparity;
    cv::minMaxLoc(real_disparity, &min_disparity, &max_disparity);
    std::cout << "最小视差: " << min_disparity << std::endl;
    std::cout << "最大视差: " << max_disparity << std::endl;

    // 计算对应的距离
    float max_distance = (min_disparity != 0) ? (camera.baseline * camera.fx) / min_disparity / 1000.0f : 0.0f;
    float min_distance = (max_disparity != 0) ? (camera.baseline * camera.fx) / max_disparity / 1000.0f : 0.0f;
    std::cout << "最小距离: " << min_distance << "m" << std::endl;
    std::cout << "最大距离: " << max_distance << "m" << std::endl;

    // 创建色条
    int color_bar_height = disparity.rows;
    int color_bar_width = 80; // 调整色条宽度
    cv::Mat color_bar(color_bar_height, color_bar_width, CV_8U);
    for (int i = 0; i < color_bar_height; i++)
    {
        float ratio = static_cast<float>(i) / (color_bar_height - 1);                                            // 从0到1的比例
        float ratio_distance = max_distance - ratio * (max_distance - min_distance);                             // 将比例映射到最大和最小距离之间
        uchar value = static_cast<uchar>(255 * (max_distance - ratio_distance) / (max_distance - min_distance)); // 根据距离计算颜色值
        color_bar.row(i).setTo(value);
    }

    cv::Mat color_bar_color_map;
    cv::applyColorMap(color_bar, color_bar_color_map, cv::COLORMAP_JET);

    int order_of_magnitude = std::pow(10, static_cast<int>(std::log10(max_distance)));
    int actual_interval = std::min(100, order_of_magnitude);
    std::cout << "actual_interval: " << actual_interval << std::endl;

    // 在色条上添加距离标签
    float font_scale = 1.0; // 调整字体大小
    int thickness = 2;
    int baseline = 0;
    for (int distance = 0; distance <= max_distance; distance += actual_interval) {
        if (distance == 0) {
            continue;
        }
        // 居中打印
        cv::Size textSize = cv::getTextSize(std::to_string(distance), cv::FONT_HERSHEY_SIMPLEX, font_scale, thickness, &baseline);
        int x_center = (color_bar_width - textSize.width) / 2;
        float ratio = (max_distance - distance) / (max_distance - min_distance);
        int i = static_cast<int>(ratio * (color_bar_height - 1));
        cv::putText(color_bar_color_map, std::to_string(distance), cv::Point(x_center, i), cv::FONT_HERSHEY_SIMPLEX, font_scale, cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
        cv::putText(color_bar_color_map, std::to_string(distance), cv::Point(x_center, i), cv::FONT_HERSHEY_SIMPLEX, font_scale, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
    }


    // 拼接视差图和带有距离标签的色条图像
    cv::Mat combined_image;
    // 缩放色条
    cv::Mat color_bar_resized;
    cv::resize(color_bar_color_map, color_bar_resized, cv::Size(color_bar_color_map.cols / 1.5, color_bar_color_map.rows / 1.5));

    // 添加背景
    cv::Mat color_bar_with_background;
    int border = 5; // 边框宽度
    cv::copyMakeBorder(color_bar_resized, color_bar_with_background, border, border, border, border, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    // 使用伪彩色可视化
    cv::Mat color_map;
    cv::Mat disparity_norm;
    cv::normalize(disparity, disparity_norm, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::applyColorMap(disparity_norm, color_map, cv::COLORMAP_JET);

    const std::string windowTitle = "Depth Viewer";
    cv::namedWindow(windowTitle, cv::WINDOW_NORMAL);
    cv::resizeWindow(windowTitle, disparity.cols, disparity.rows);
    cv::setMouseCallback(windowTitle, onMouse);

    while (true)
    {
        cv::Mat display_image = color_map.clone();
        cv::rectangle(display_image, select_roi, cv::Scalar(0, 255, 0), 2);

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

        // 定义ROI并将色条复制到视差图
        int y_centered = (display_image.rows - color_bar_with_background.rows) / 2;
        cv::Rect show_roi(display_image.cols - 100, y_centered, color_bar_with_background.cols, color_bar_with_background.rows); // 定义色条的位置
        color_bar_with_background.copyTo(display_image(show_roi));
        cv::imshow(windowTitle, display_image);
        char key = (char)cv::waitKey(1);
        if (key == 27)
            break; // 按'Esc'退出
    }

    return 0;
}
