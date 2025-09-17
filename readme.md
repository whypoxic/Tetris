# 俄罗斯方块 openGL 实现

## by whypoxic

## 使用介绍

``````
包括了基本的俄罗斯方块玩法，支持键盘操作。
↑ 旋转
← → 左右移动
↓ 加速下落（长按）
N 生成新方块
ESC 退出
```````
## 主要文件介绍

- main.cpp 主要代码文件
  
- CMakeLists.txt CMake 配置文件
  
- README.md 说明文件
  
- Initshader 着色器配置
  
- shaders 着色器文件夹
  - fshader_color 纯色图形片段着色器
  - fshader_texture 纹理图片片段着色器
  - vshader_color 顶点着色器
  - vshader_texture 顶点着色器

- include 头文件文件夹
  - Angel.h Angel 库头文件,包含了 GL,GLU,GLUT 等库
  - texture.h 纹理类定义
  - stb_image.h 图片加载库

- photos 资源文件夹
  