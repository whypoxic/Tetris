// texture.cpp
// Created by why on 2025/9/17.
// Texture 实现文件

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Angel.h"
#include "Texture.h"

// 纹理加载函数
GLuint loadTexture(const char* path){
    int w,h,n;
    stbi_set_flip_vertically_on_load(true); // 翻转 Y 轴
    unsigned char *data=stbi_load(path,&w,&h,&n,0);
    if(!data){std::cerr<<"could not loud texture "<<path<<"\n";return 0;}
    GLuint tex; glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    GLenum format=(n==4)?GL_RGBA:GL_RGB;
    glTexImage2D(GL_TEXTURE_2D,0,format,w,h,0,format,GL_UNSIGNED_BYTE,data);
    stbi_image_free(data);
    return tex;
}