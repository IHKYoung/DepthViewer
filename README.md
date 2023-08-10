# Depth Viewer for Calculate Value from Disparity Map

用于从视差图计算深度值，在彩色可视化的视差图上，通过**鼠标落点和鼠标框选**实时显示深度值，以验证视差的效果。

## 功能

- **实时深度查看**：通过鼠标在视差图上的位置，实时计算和显示对应的深度值。
- **区域平均深度计算**：可以通过鼠标框选视差图上的某个区域，计算并显示该区域的平均深度。
- **视差图可视化**：提供视差图的彩色可视化。
- 🌟**增加距离标尺**：在可视化右侧增加色条对应的距离标尺。

## 安装步骤

### 依赖

- OpenCV

### 编译

1. 克隆仓库：

   ```bash
   git clone https://github.com/IHKYoung/DepthViewer.git
   cd DepthViewer
   ```

2. 创建并进入构建目录：

   ```bash
   mkdir build
   cd build
   ```

3. 运行CMake以生成构建文件：

   ```bash
   cmake ..
   ```

4. 编译项目：

   ```bash
   make
   ```

## 用法

```bash
./calculate_depth_from_disparity <baseline> <fx> <disparity_factor> <image_path>
```

- `baseline`: 相机基线值。
- `fx`: 相机的焦距。
- `disparity_factor`: 视差因子。
- `image_path`: 视差图的路径。

## 示例

使用如下命令来计算和可视化深度：

```bash
./calculate_depth_from_disparity 159.934545 1887.365601 32.0 /path/to/your/image.png
```

## 贡献

欢迎任何形式的贡献，包括报告问题、添加新功能等。

## 许可

此项目使用MIT许可证。有关详细信息，请参阅[LICENSE](https://github.com/IHKYoung/DepthViewer/blob/main/LICENSE)文件。