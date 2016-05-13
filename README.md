# GraduationDesignTest

我的本科毕业设计代码——基于纹理特征的遥感图像分类方法研究

# 说明

* 平台Windows7-64位。
* 使用QT5.5.0+MinGW4.9.4构建，用到的库有GDAL2.0,libsvm,boost,tinyxml。
* 目前完成了遥感图像多尺度分割算法、LBP和改进LBP纹理、Tamura纹理、GLCM纹理、Gabor纹理的提取。其中Gabor纹理尚未全部完成。
* 整个项目仍需整合，当前的分类是针对单张图像的，将来要在多尺度分割生成的区域对象上进行纹理提取并分类。
* 除以上外，项目还包括一个金字塔匹配核的实现，两种控制台进度条的实现以及其他一些小算法。

# License

The MIT License (MIT)
