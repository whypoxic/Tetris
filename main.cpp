// 俄罗斯方块 Tetris
// 使用 OpenGL 3.3 + GLFW + GLAD + Angel.h
// version 1.0
// by whypoxic @ 2025
// 2025-09-15

// 2025-09-16 增加了行满检测与消除逻辑
// 2025-09-16 增加了游戏结束逻辑
// 2025-09-17 增加了 Game Over 纹理画面
// 2025-09-17 增加了分数系统


// ====== 说明 ======
// ↑ 旋转
// ← → 左右移动
// ↓ 加速下落（长按）
// N 生成新方块
// ESC 退出
// ==================

// ====== main.cpp ======

#include "Angel.h"
// #include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "texture.h" // 纹理加载
#include "stb_easy_font.h" // 字体渲染


// === 棋盘大小 ===
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

// === 窗口宽高（画布大小）===
int WINDOW_WIDTH  = 800;
int WINDOW_HEIGHT = 800;

// === 每格大小 ===
const float CELL_SIZE = 0.08f;

// === 棋盘边界坐标 ===
float minX = -BOARD_WIDTH * CELL_SIZE * 0.5f;
float minY = -BOARD_HEIGHT * CELL_SIZE * 0.5f;
float maxX = minX + BOARD_WIDTH * CELL_SIZE;
float maxY = minY + BOARD_HEIGHT * CELL_SIZE;

// === 方块形状（4x4矩阵） ===
const int TETROMINO_SIZE = 4;
using Shape = std::vector<std::vector<int>>;

// === 方块下落控制 ===
float fallInterval = 0.5f;   // 每0.5秒下落一格 （ 长按↓加速 *0.5
float fallTimer = 0.0f;
bool accelerate = false;      // 是否按下加速键

// ======= 棋盘数组 数组内容(0 = 空, 1..7 为方块ID) =======
int board[BOARD_HEIGHT][BOARD_WIDTH]; // board[y][x]
std::vector<glm::vec3> colorTable;    // id -> color

// === 操作变量 ===
const float RepeatInterval = 0.05f; // 键盘灵敏度（长按间隔）

// === Game Over 相关 ===
bool gameOver = false;
const char* gameOverTexturePath = "photos/gameover.png"
                                  ;
// === 分数系统 ===
int score = 0;// 分数

// ======= 方块定义 =======
// 定义形状 7
Shape tetromino_I = {
    {0,1,0,0},
    {0,1,0,0},
    {0,1,0,0},
    {0,1,0,0}
};
Shape tetromino_O = {
    {1,1,0,0},
    {1,1,0,0},
    {0,0,0,0},
    {0,0,0,0}
};
Shape tetromino_T = {
    {0,1,0,0},
    {1,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
};
Shape tetromino_S = {
    {0,1,1,0},
    {1,1,0,0},
    {0,0,0,0},
    {0,0,0,0}
};
Shape tetromino_Z = {
    {1,1,0,0},
    {0,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
};
Shape tetromino_J = {
    {1,0,0,0},
    {1,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
};
Shape tetromino_L = {
    {0,0,1,0},
    {1,1,1,0},
    {0,0,0,0},
    {0,0,0,0}
};

// 方块列表 (顺序与 colorTable 对应，id = index+1)
std::vector<Shape> allShapes = {tetromino_I,tetromino_O,tetromino_T,tetromino_S,tetromino_Z,tetromino_J,tetromino_L};
std::vector<glm::vec3> shapeColors = { // 颜色对应方块
    {0.0f,1.0f,1.0f},   // I 蓝绿色
    {1.0f,1.0f,0.0f},   // O 黄色
    {1.0f,0.0f,1.0f},   // T 紫色
    {0.0f,1.0f,0.0f},   // S 绿色
    {1.0f,0.0f,0.0f},   // Z 红色
    {0.0f,0.0f,1.0f},   // J 蓝色
    {1.0f,0.5f,0.0f}    // L 橙色
};

// === 当前方块 ===
struct Tetromino {
    Shape shape;
    int x; // 左上角列索引（0 ~ BOARD_WIDTH-1）
    int y; // 左上角行索引（0 ~ BOARD_HEIGHT-1）
    glm::vec3 color;
    int id; // 1..7
} currentPiece;

// OpenGL程序
GLuint programColor, programTexture; // 着色器程序（颜色/纹理）

// 旋转方块90度（顺）
Shape rotate90(const Shape& s) {
    Shape rotated = s;
    for (int i=0; i<TETROMINO_SIZE; i++) {
        for (int j=0; j<TETROMINO_SIZE; j++) {
            rotated[j][TETROMINO_SIZE-1-i] = s[i][j];
        }
    }
    return rotated;
}

// 基于索引 检查是否超出边界或与board重叠
bool isOutOfBounds(const Tetromino& piece) {
    for (int i=0; i<TETROMINO_SIZE; i++) {
        for (int j=0; j<TETROMINO_SIZE; j++) {
            if (piece.shape[i][j]) {
                int boardX = piece.x + j;
                int boardY = piece.y + (TETROMINO_SIZE - 1 - i); // 转换为 board 索引（行）
                // 边界检测
                if (boardX < 0 || boardX >= BOARD_WIDTH || boardY < 0 || boardY >= BOARD_HEIGHT) {
                    return true;
                }
                // 与已固定方块重叠检测
                if (board[boardY][boardX] != 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

// 锁定当前方块到 board（固化）
void lockPieceToBoard() {
    for (int i=0; i<TETROMINO_SIZE; ++i) {
        for (int j=0; j<TETROMINO_SIZE; ++j) {
            if (currentPiece.shape[i][j]) {
                int bx = currentPiece.x + j;
                int by = currentPiece.y + (TETROMINO_SIZE - 1 - i);
                if (by >= 0 && by < BOARD_HEIGHT && bx >= 0 && bx < BOARD_WIDTH) {
                    board[by][bx] = currentPiece.id; // 写入 ID（用于绘色）
                }
            }
        }
    }
}

// 绘制单个小格子（使用着色器绘制填充 + 用 GL_LINE_LOOP 用着色器绘制边框）
void drawCell(float x, float y, glm::vec3 color) {
    glUseProgram(programColor);// 使用颜色着色器

    float size = CELL_SIZE;

    // === 矩形填充 （双三角形六顶点模式）===
    glm::vec2 verticesFill[6] = {
        {x, y},
        {x + size, y},
        {x + size, y + size},
        {x, y},
        {x + size, y + size},
        {x, y + size}
    };

    glm::vec3 colorsFill[6] = {
        color, color, color, color, color, color
    };

    GLuint vaoFill, vboFill;
    glGenVertexArrays(1, &vaoFill);
    glBindVertexArray(vaoFill);

    glGenBuffers(1, &vboFill);
    glBindBuffer(GL_ARRAY_BUFFER, vboFill);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(verticesFill) + sizeof(colorsFill),
                 nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verticesFill), verticesFill);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(verticesFill), sizeof(colorsFill), colorsFill);

    GLuint pLoc = glGetAttribLocation(programColor, "vPosition");
    glEnableVertexAttribArray(pLoc);
    glVertexAttribPointer(pLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    GLuint cLoc = glGetAttribLocation(programColor, "vColor");
    glEnableVertexAttribArray(cLoc);
    glVertexAttribPointer(cLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)sizeof(verticesFill));

    // 画填充
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDeleteBuffers(1, &vboFill);
    glDeleteVertexArrays(1, &vaoFill);

    // === 边框线（黑色）===
    glm::vec2 verticesBorder[4] = {
        {x, y},
        {x + size, y},
        {x + size, y + size},
        {x, y + size}
    };

    glm::vec3 colorsBorder[4] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f)
    };

    GLuint vaoBorder, vboBorder;
    glGenVertexArrays(1, &vaoBorder);
    glBindVertexArray(vaoBorder);

    glGenBuffers(1, &vboBorder);
    glBindBuffer(GL_ARRAY_BUFFER, vboBorder);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(verticesBorder) + sizeof(colorsBorder),
                 nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verticesBorder), verticesBorder);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(verticesBorder), sizeof(colorsBorder), colorsBorder);

    glEnableVertexAttribArray(pLoc);
    glVertexAttribPointer(pLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(cLoc);
    glVertexAttribPointer(cLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)sizeof(verticesBorder));

    // 画边框线
    glLineWidth(1.0f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glDeleteBuffers(1, &vboBorder);
    glDeleteVertexArrays(1, &vaoBorder);
}

// 绘制整个 board（所有已固化方块）
void drawBoard() {
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            int id = board[y][x];
            if (id != 0) {
                // 把 board 索引转换为 OpenGL 坐标 (minX/minY 右上角)
                float ox = minX + x * CELL_SIZE;
                float oy = minY + y * CELL_SIZE;
                drawCell(ox, oy, colorTable[id]);
            }
        }
    }
}

// 绘制当前下落方块（使用 minX/minY 映射）
void drawCurrentPiece() {
    for (int i=0; i<TETROMINO_SIZE; i++) {
        for (int j=0; j<TETROMINO_SIZE; j++) {
            if (currentPiece.shape[i][j]) {
                float ox = minX + (currentPiece.x + j) * CELL_SIZE;
                float oy = minY + (currentPiece.y + (TETROMINO_SIZE - 1 - i)) * CELL_SIZE;
                drawCell(ox, oy, currentPiece.color);
            }
        }
    }
}

void clearFullLines();
// 随机生成一个方块（居顶并居中），并分配 id/color
void spawnNewPiece() {
    clearFullLines();// 生成新方块前先检查并消行

    int idx = rand() % allShapes.size();
    currentPiece.shape = allShapes[idx];
    currentPiece.id = idx + 1; // id 1..7

    // 横向居中
    currentPiece.x = (BOARD_WIDTH - TETROMINO_SIZE) / 2;
    // 顶部起始行（让方块在界内的最高位置）
    currentPiece.y = BOARD_HEIGHT - TETROMINO_SIZE;
    currentPiece.color = shapeColors[idx];


    std::cout << "new cube idx=" << idx << " id=" << currentPiece.id << std::endl;

    accelerate = false; // 重置加速状态

    // 如果生成时即与已固定方块重叠  游戏结束
    if (isOutOfBounds(currentPiece)) {
        std::cout << "Game Over! Board reset." << std::endl;
        // 清空棋盘
        // for (int r = 0; r < BOARD_HEIGHT; ++r)
        //     for (int c = 0; c < BOARD_WIDTH; ++c)
        //         board[r][c] = 0;
        gameOver = true; // 标记游戏结束

    }
}

// 绘制棋盘边框
void drawBorder() {
    glUseProgram(programColor);// 使用颜色着色器

    glm::vec2 verts[4] = {
        {minX, minY},
        {maxX, minY},
        {maxX, maxY},
        {minX, maxY}
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1,&vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    GLuint pLoc = glGetAttribLocation(programColor, "vPosition");
    glEnableVertexAttribArray(pLoc);
    glVertexAttribPointer(pLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // 白色
    GLuint cLoc = glGetAttribLocation(programColor, "vColor");
    glVertexAttrib3f(cLoc, 1.0f, 1.0f, 1.0f);

    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

// 方块掉落 (封装)
void updateFall(double deltaTime) {
    fallTimer += deltaTime;
    float interval = accelerate ? (fallInterval * 0.1f) : fallInterval;

    if (fallTimer >= interval) {
        Tetromino tmp = currentPiece;
        tmp.y -= 1; // 向下移动一格
        // 如果 能下移 -> 执行下移
        // 否则 到底/碰撞 -> 执行固定、生成新方块
        if (!isOutOfBounds(tmp)) {
            currentPiece = tmp;
        } else {
            lockPieceToBoard();
            spawnNewPiece();
        }
        fallTimer = 0.0f;
    }
}

// 满行检查与消行 （每次固定方块后调用）
void clearFullLines() {
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (board[y][x] == 0) {
                full = false;
                break;
            }
        }//满行判别
        if (full) {
            score += 100; // 每消一行加100分
            std::cout << "Line cleared! Score: " << score << std::endl;

            // 把这一行之上的所有行往下移一行
            for (int yy = y; yy < BOARD_HEIGHT - 1; ++yy) {
                for (int x = 0; x < BOARD_WIDTH; ++x) {
                    board[yy][x] = board[yy + 1][x];
                }
            }
            // 最上面一行清空
            // for (int x = 0; x < BOARD_WIDTH; ++x) {
            //     board[BOARD_HEIGHT - 1][x] = 0;
            // }
            y--; // 这一行要重新检查（因为下移后新的一行可能也满）
        }
    }
}

// 绘制 Game Over 屏幕(使用纹理 + 透明度)
void drawGameOverScreen() {
    glUseProgram(programTexture); // 使用纹理着色器

    const float alpha = 0.8f; // 透明度

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 绑定纹理
    GLuint gameOverTexture = loadTexture(gameOverTexturePath);
    glBindTexture(GL_TEXTURE_2D, gameOverTexture);

    // 设置透明度 uniform
    GLuint alphaLoc = glGetUniformLocation(programTexture, "uAlpha");
    glUniform1f(alphaLoc, alpha);

    // 顶点数据：位置 + UV
    float verts[] = {
        // pos    // uv
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    unsigned int idx[] = {0,1,2,2,3,0};

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    // 设置顶点属性
    GLuint pLoc = glGetAttribLocation(programTexture, "vPosition");
    glEnableVertexAttribArray(pLoc);
    glVertexAttribPointer(pLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    GLuint uvLoc = glGetAttribLocation(programTexture, "vTexCoord");
    glEnableVertexAttribArray(uvLoc);
    glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // 绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // 清理
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);

    glDisable(GL_BLEND);
}

// ===== 绘制分数系统 =====
void drawScore() {

    /*
     *文本传入，用一个quads接受
     *quads中：四个float为一个顶点的xyuv；我们只需要xy
     *然后转换为OpenGL坐标系
     *转化成两个三角形顶点
     */

    float scale = 3.0f; // 字体缩放

    char buf[32];// 文本缓冲区
    sprintf(buf, "Score: %d", score);

    float quads[99999]; // stb_easy_font 顶点缓存
    int num_quads = stb_easy_font_print(10, 10, buf, NULL, quads, sizeof(quads));
    float* verts = (float*)quads;

    // std::cout << "num_quads=" << num_quads << std::endl;
    // for (int i = 0; i < num_quads * 4 ; i+=4) {
    //     std::cout << "(" << verts[i] << "," << verts[i+1] << ") ";
    //     //std::cout << verts[i] << " ";
    // }
    // //std::cout << std::endl;

    // 右上角偏移
    float xOffset = WINDOW_WIDTH - strlen(buf) * scale * 8.0f - 10.0f; // 每字符（8px * 缩放） 宽 + 10px边距
    float yOffset = 10.0f; // 顶部边距10px

    // 转换每个 quad 为两组三角形顶点
    int num_vertices = num_quads * 6; // 每个 quad 2 三角形 = 6 顶点
    std::vector<glm::vec2> triVerts(num_vertices);
    for(int i=0; i<num_quads; i++) {
        float* q = verts + i * 16; // quad 中 一个顶点四个float

        // 提取4个顶点的xy
        float x0=q[0],  y0=q[1];
        float x1=q[4],  y1=q[5];
        float x2=q[8],  y2=q[9];
        float x3=q[12], y3=q[13];

        // 偏移
        x0*=scale; y0*=scale;
        x1*=scale; y1*=scale;
        x2*=scale; y2*=scale;
        x3*=scale; y3*=scale;

        // 偏移到屏幕位置
        x0+=xOffset; y0+=yOffset;
        x1+=xOffset; y1+=yOffset;
        x2+=xOffset; y2+=yOffset;
        x3+=xOffset; y3+=yOffset;

        // 转换到OpenGL [-1,1]
        auto toGL = [&](float x,float y){
            return glm::vec2(
                2.0f*x / WINDOW_WIDTH - 1.0f,
                1.0f - 2.0f*y / WINDOW_HEIGHT
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

    // 设置文字颜色为白色
    GLuint colorLoc = glGetAttribLocation(programColor, "vColor");
    glVertexAttrib3f(colorLoc, 1.0f, 1.0f, 1.0f);

    glDrawArrays(GL_TRIANGLES, 0, num_vertices);

    glDisableVertexAttribArray(posLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glDisable(GL_BLEND);

    /*
     * 测试用例

    glm::vec2 verts[6] = {{-0.9f,0.9f},{-0.8f,0.9f},{-0.8f,0.8f},{-0.9f,0.9f},{-0.8f,0.8f},{-0.9f,0.8f}};
    glUseProgram(programColor);
    GLuint vao, vbo;
    glGenVertexArrays(1,&vao); glBindVertexArray(vao);
    glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    GLuint posLoc = glGetAttribLocation(programColor,"vPosition");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc,2,GL_FLOAT,GL_FALSE,0,(void*)0);
    GLuint colorLoc = glGetAttribLocation(programColor,"vColor");
    glVertexAttrib3f(colorLoc,1,1,1);
    glDrawArrays(GL_TRIANGLES,0,6);
    */
}


// ================= 处理键盘输入（长按状态） ==================
// 按键状态
bool keyLeftPressed = false;
bool keyRightPressed = false;
bool keyDownPressed = false;
// 长按间隔（s）
float repeatInterval = RepeatInterval;//键盘灵敏度（长按间隔）
float keyTimerLeft = 0.0f;
float keyTimerRight = 0.0f;
float keyTimerDown = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // 按下时设置按键状态
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_LEFT) keyLeftPressed = true;
        if (key == GLFW_KEY_RIGHT) keyRightPressed = true;
        if (key == GLFW_KEY_DOWN) keyDownPressed = true;
        if (key == GLFW_KEY_UP) {
            Tetromino tmp = currentPiece;
            tmp.shape = rotate90(tmp.shape);
            if (!isOutOfBounds(tmp))
                currentPiece = tmp;
        }
        if (key == GLFW_KEY_N) {
            spawnNewPiece();
        }
    }

    // 释放时清除按键状态
    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_LEFT) keyLeftPressed = false;
        if (key == GLFW_KEY_RIGHT) keyRightPressed = false;
        if (key == GLFW_KEY_DOWN) keyDownPressed = false;
    }
}

// 处理长按
void handleKeyRepeat(double deltaTime) {
    Tetromino tmp;
    // 左键长按
    if (keyLeftPressed) {
        keyTimerLeft += deltaTime;
        if (keyTimerLeft >= repeatInterval) {
            tmp = currentPiece;
            tmp.x -= 1;
            if (!isOutOfBounds(tmp)) currentPiece = tmp;
            keyTimerLeft = 0.0f;
        }
    } else keyTimerLeft = 0.0f;

    // 右键长按
    if (keyRightPressed) {
        keyTimerRight += deltaTime;
        if (keyTimerRight >= repeatInterval) {
            tmp = currentPiece;
            tmp.x += 1;
            if (!isOutOfBounds(tmp)) currentPiece = tmp;
            keyTimerRight = 0.0f;
        }
    } else keyTimerRight = 0.0f;

    // 下键长按，加速下落
    if (keyDownPressed) {
        accelerate = true;  // 长按下键加速
    } else {
        accelerate = false;
        keyTimerDown = 0.0f;
    }
}

int main(int argc,char**argv) {
    srand((unsigned)time(nullptr));

    // 初始化 board 为 0
    memset(board, 0, sizeof(board));
    // colorTable 索引从 0 开始，0 表示空
    colorTable.resize(8);
    colorTable[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 7; ++i)
        colorTable[i+1] = shapeColors[i];

    if (!glfwInit()) {
        std::cout<<"Failed to init GLFW\n"; return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH,WINDOW_HEIGHT,"Tetris Demo by whypoxic",NULL,NULL);
    if (!window) { std::cout<<"Failed to create GLFW window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout<<"Failed to init GLAD\n";return -1;
    }
    glfwSetKeyCallback(window,key_callback);

    // shader初始化
    std::string vshader_color="shaders/vshader_color.glsl";
    std::string fshader_color="shaders/fshader_color.glsl";
    programColor = InitShader(vshader_color.c_str(),fshader_color.c_str());
    std::string vshader_texture="shaders/vshader_texture.glsl";
    std::string fshader_texture="shaders/fshader_texture.glsl";
    programTexture = InitShader(vshader_texture.c_str(),fshader_texture.c_str());
    //glUseProgram(programColor);//默认使用颜色着色器


    // 开始循环下落
    double lastTime = glfwGetTime();
    spawnNewPiece();// 初始化一个方块
    while(!glfwWindowShouldClose(window)){
        glClear(GL_COLOR_BUFFER_BIT);

        // 计算 deltaTime
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // 绘制：先边框，再已固定的方块，再当前方块;化分数
        drawBorder();
        drawBoard();
        drawCurrentPiece();
        drawScore();

        // === 结束与否 ===
        if (!gameOver) {
            // 处理长按
            handleKeyRepeat(deltaTime);
            // 自动下落
            updateFall(deltaTime);
        }
        else {
            drawGameOverScreen();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
