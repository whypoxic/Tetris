// TextRenderer.cpp
// Created by why on 2025/9/22.
// .h 实现文件


/*
 * === TextRenderer ===
// 初始化
TextRenderer textRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, programColor);

// 每帧绘制
textRenderer.drawText("Score: " + std::to_string(score),
                      20, 20, 2.0f, glm::vec3(1.0f, 1.0f, 1.0f));
                      // 参数说明：str文本内容,  x,y = 像素位置, scale = 缩放倍数, color = 颜色
 */

#include "TextRender.h"
#include "stb_easy_font.h"
#include <Angel.h>
#include <vector>

TextRenderer::TextRenderer(int w, int h, GLuint shaderProgram)
    : windowWidth(w), windowHeight(h), programColor(shaderProgram) {}

void TextRenderer::drawText(const std::string& text,
                            float x, float y,
                            float scale,
                            const glm::vec3& color)
{
    // 生成 stb_easy_font 顶点
    float quads[99999]; // 顶点缓存
    int num_quads = stb_easy_font_print(0, 0, const_cast<char*>(text.c_str()),// 修改const char
        NULL, quads, sizeof(quads));
    float* verts = (float*)quads;

    int num_vertices = num_quads * 6; // 每个 quad 2 三角形 = 6 顶点
    std::vector<glm::vec2> triVerts(num_vertices);

    for (int i = 0; i < num_quads; i++) {
        float* q = verts + i * 16; // quad 中每个顶点4float

        // 提取4个顶点
        float x0=q[0],  y0=q[1];
        float x1=q[4],  y1=q[5];
        float x2=q[8],  y2=q[9];
        float x3=q[12], y3=q[13];

        // 缩放
        x0*=scale; y0*=scale;
        x1*=scale; y1*=scale;
        x2*=scale; y2*=scale;
        x3*=scale; y3*=scale;

        // 偏移
        x0+=x; y0+=y;
        x1+=x; y1+=y;
        x2+=x; y2+=y;
        x3+=x; y3+=y;

        // 转换到OpenGL [-1,1]
        auto toGL = [&](float xx,float yy){
            return glm::vec2(
                2.0f*xx / windowWidth - 1.0f,
                1.0f - 2.0f*yy / windowHeight
            );
        };

        glm::vec2 v0 = toGL(x0,y0);
        glm::vec2 v1 = toGL(x1,y1);
        glm::vec2 v2 = toGL(x2,y2);
        glm::vec2 v3 = toGL(x3,y3);

        // 写两个三角形
        triVerts[i*6 + 0] = v0;
        triVerts[i*6 + 1] = v1;
        triVerts[i*6 + 2] = v2;

        triVerts[i*6 + 3] = v0;
        triVerts[i*6 + 4] = v2;
        triVerts[i*6 + 5] = v3;
    }

    // 开启混合半透明
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(programColor); // 使用颜色着色器

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, triVerts.size() * sizeof(glm::vec2), triVerts.data(), GL_DYNAMIC_DRAW);

    GLuint posLoc = glGetAttribLocation(programColor, "vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    // 设置颜色
    GLuint colorLoc = glGetAttribLocation(programColor, "vColor");
    glVertexAttrib3f(colorLoc, color.r, color.g, color.b);

    glDrawArrays(GL_TRIANGLES, 0, num_vertices);

    glDisableVertexAttribArray(posLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glDisable(GL_BLEND);
}