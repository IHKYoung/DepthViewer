#include <iomanip>
#include <iostream>
#include <opencv2/opencv.hpp>

struct CameraParameters {
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
float computeDepth(float disparity_value) {
    return (camera.baseline * camera.fx) / disparity_value / 1000.0f;
}

// 鼠标回调函数
void onMouse(int event, int x, int y, int flags, void *) {
    static cv::Point start_point;
    mouse_position = cv::Point(x, y);

    if (event == cv::EVENT_LBUTTONDOWN) {
        start_point = mouse_position;
        select_roi = cv::Rect(start_point, mouse_position);
    } else if (event == cv::EVENT_MOUSEMOVE && (flags & cv::EVENT_FLAG_LBUTTON)) {
        select_roi = cv::Rect(start_point, mouse_position);
    } else if (event == cv::EVENT_LBUTTONUP) {
        select_roi = cv::Rect(start_point, mouse_position);

        // 计算选择区域的平均深度
        int count = 0;
        average_depth = 0.0;
        for (int i = select_roi.y; i < select_roi.y + select_roi.height; i++) {
            for (int j = select_roi.x; j < select_roi.x + select_roi.width; j++) {
                float disparity_value = real_disparity.at<float>(i, j);
                if (disparity_value != 0) { // 避免除以零
                    average_depth += computeDepth(disparity_value);
                    count++;
                }
            }
        }
        average_depth /= count;
    }
}

int main(int argc, char **argv) {
    // 验证和解析输入参数
    if (argc < 5) {
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
    float max_distance = computeDepth(min_disparity);
    float min_distance = computeDepth(max_disparity);
    std::cout << "最小距离: " << min_distance << "m" << std::endl;
    std::cout << "最大距离: " << max_distance << "m" << std::endl;

    // 创建色条
    int color_bar_height = disparity.rows;
    int color_bar_width = 90; // 调整色条宽度
    cv::Mat color_bar(color_bar_height, color_bar_width, CV_32F);
    for (int i = 0; i < color_bar_height; i++) {
        float ratio = static_cast<float>(i) / (color_bar_height - 1);          // 从0到1的比例
        float value = min_disparity + ratio * (max_disparity - min_disparity); // 根据比例计算视差值
        color_bar.row(i).setTo(value);
    }

    cv::Mat color_bar_color_map;
    cv::Mat color_bar_normalized;
    cv::normalize(color_bar, color_bar_normalized, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::applyColorMap(color_bar_normalized, color_bar_color_map, cv::COLORMAP_JET);

    // 在色条上添加视差标签
    float font_scale = 1.0; // 调整字体大小
    int thickness = 2;

    std::vector<double> disparities;
    std::vector<int> ratios = {1, 2, 3, 5, 8}; // 定义比例系数

    for (int ratio : ratios) {
        disparities.push_back(max_disparity / ratio);
    }

    for (double disparity : disparities) {
        // // 将视差转换为距离，并四舍五入到最近的整数
        // int distance = static_cast<int>(std::round(computeDepth(disparity)));
        // 将视差转换为距离
        double distance_value = computeDepth(disparity);

        // 使用stringstream进行格式化
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << distance_value;
        std::string distance = ss.str();

        // 居中打印
        cv::Size textSize = cv::getTextSize(distance, cv::FONT_HERSHEY_SIMPLEX, font_scale, thickness, 0);
        int x_center = (color_bar_width - textSize.width) / 2;
        float ratio = (disparity - min_disparity) / (max_disparity - min_disparity);
        int i = static_cast<int>(ratio * (color_bar_height - 1));

        // 调整y方向上的位置，使其居中
        int y_center = i - (textSize.height / 2);
        cv::putText(color_bar_color_map, distance, cv::Point(x_center, y_center), cv::FONT_HERSHEY_SIMPLEX, font_scale,
                    cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
        cv::putText(color_bar_color_map, distance, cv::Point(x_center, y_center), cv::FONT_HERSHEY_SIMPLEX, font_scale,
                    cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
    }

    // 拼接视差图和带有距离标签的色条图像
    cv::Mat combined_image;
    // 缩放色条
    cv::Mat color_bar_resized;
    cv::resize(color_bar_color_map, color_bar_resized,
               cv::Size(color_bar_color_map.cols / 1.5, color_bar_color_map.rows / 1.5));

    // 添加背景
    cv::Mat color_bar_with_background;
    int border = 5; // 边框宽度
    cv::copyMakeBorder(color_bar_resized, color_bar_with_background, border, border, border, border,
                       cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    // 使用伪彩色可视化，先将0视差值设为黑色
    cv::Mat disparity_norm;
    cv::normalize(real_disparity, disparity_norm, 0, 255, cv::NORM_MINMAX, CV_8U, real_disparity != 0);
    cv::Mat color_map;
    cv::applyColorMap(disparity_norm, color_map, cv::COLORMAP_JET);
    color_map.setTo(cv::Scalar(0, 0, 0), disparity_norm == 0);

    const std::string windowTitle = "Depth Viewer";
    cv::namedWindow(windowTitle, cv::WINDOW_NORMAL);
    cv::resizeWindow(windowTitle, disparity.cols, disparity.rows);
    cv::setMouseCallback(windowTitle, onMouse);

    while (true) {
        cv::Mat display_image = color_map.clone();
        cv::rectangle(display_image, select_roi, cv::Scalar(0, 255, 0), 2);

        // 显示平均深度
        std::stringstream ss_avg;
        ss_avg << "Average Depth: " << average_depth << " meters";
        cv::putText(display_image, ss_avg.str(), cv::Point(5, 60), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                    cv::Scalar(255, 255, 255), 2, cv::LINE_AA);

        // 显示实时深度
        float disparity_value = real_disparity.at<float>(mouse_position.y, mouse_position.x);
        if (disparity_value != 0) {
            float depth = (camera.baseline * camera.fx) / disparity_value / 1000.0f;
            std::stringstream ss_depth;
            ss_depth << "Depth: " << depth << " meters";
            cv::putText(display_image, ss_depth.str(), cv::Point(5, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                        cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        }

        // 定义ROI并将色条复制到视差图
        int y_centered = (display_image.rows - color_bar_with_background.rows) / 2;
        cv::Rect show_roi(display_image.cols - 100, y_centered, color_bar_with_background.cols,
                          color_bar_with_background.rows); // 定义色条的位置
        color_bar_with_background.copyTo(display_image(show_roi));
        cv::imshow(windowTitle, display_image);
        char key = (char) cv::waitKey(1);
        if (key == 27)
            break; // 按'Esc'退出
    }

    return 0;
}
