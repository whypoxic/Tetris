//
// Created by why on 2025/9/22.
//

#pragma once
#include <string>
#include "stb_easy_font.h"
#include <Angel.h>

/*
 * === TextRenderer ===
// 初始化
TextRenderer textRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, programColor);

// 每帧绘制
textRenderer.drawText("Score: " + std::to_string(score),
                      20, 20, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f));
                      // 参数说明：str文本内容,  x,y = 像素位置, scale = 缩放倍数, color = 颜色
 */

class TextRenderer {
public:
    // 构造函数：传入窗口宽高与着色器 program
    TextRenderer(int w, int h, GLuint shaderProgram);

    // 绘制文本：text = 字符串，x,y = 屏幕像素位置，scale = 缩放倍数，color = 颜色
    void drawText(const std::string& text,
                  float x, float y,
                  float scale,
                  const glm::vec3& color);

private:
    int windowWidth;
    int windowHeight;
    GLuint programColor;
};
