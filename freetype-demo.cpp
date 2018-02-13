#include "freetype/ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftglyph.h"
#include "freetype/ftoutln.h"
#include "freetype/fttrigon.h"
#include <cstdio>
#include <string>
#include <iostream>
#define GLEW_STATIC
#include "../include/GL/glew.h"
#include "../include/GLFW/glfw3.h"

// If you are on windows, you can link those and then compile 'freetype-demo.cpp' easliy
// Otherwise you should set linker by yourself.
//#pragma comment ( lib, "lib/freetype.lib" )
//#pragma comment ( lib, "lib/freetype.dll.a" )
//#pragma comment ( lib, "lib/glew32s.lib" )
//#pragma comment ( lib, "lib/libglfw3dll.a" )
//#pragma comment ( lib, "lib/OPENGL32.LIB" )


GLFWwindow *window;
char *WindowName = "freetype-demo";
const GLuint WindowWidth = 800, WindowHeight = 800;

void InitWindow();

class CharTexture
{
public:
    GLuint texID = 0;
    float width = 0, height = 0;
    float adv_x, adv_y;
    float start_x, start_y;
};

class TextRenderer
{
public:
    int width, height;
    bool Load(const char* file, int width, int height);
    void DrawText(std::string str, int x0, int y0);

private:
    FT_Library library;
    FT_Face face;
    CharTexture textures[256];

    CharTexture* GetTextChar(char ch);
    bool LoadChar(char ch);
};

bool TextRenderer::LoadChar(char ch)
{
    if(FT_Load_Char(face, ch, FT_LOAD_RENDER))
        return false;

    CharTexture &charTex = textures[ch];

    FT_Glyph glyph;
    if(FT_Get_Glyph(face->glyph, &glyph))
        return false;

    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD);
    FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;

    FT_Bitmap& bitmap = bitmap_glyph->bitmap;

    int width = bitmap.width;
    int height = bitmap.rows;

    charTex.width = width * 1.0f / WindowWidth * 2;
    charTex.height = height * 1.0f / WindowWidth * 2;
    charTex.adv_x = face->glyph->advance.x *1.0f / 64 / WindowWidth * 2;
    charTex.adv_y = face->size->metrics.y_ppem *1.0f / WindowWidth * 2;
    charTex.start_x = (float)bitmap_glyph->left *1.0f / WindowWidth * 2;
    charTex.start_y = ((float)bitmap_glyph->top - height) *1.0f / WindowWidth * 2 ;

    glGenTextures(1, &charTex.texID);
    glBindTexture(GL_TEXTURE_2D, charTex.texID);
    char* pbuf = new char[width * height * 4];
    for(int j = 0; j < height; j++)
    {
        for(int i = 0; i < width; i++)
        {
            unsigned char vl = (i >= bitmap.width || j >= bitmap.rows) ? 0 : bitmap.buffer[i + bitmap.width * j];
            pbuf[(4 * i + (height - j - 1) * width * 4)] = 0xFF;
            pbuf[(4 * i + (height - j - 1) * width * 4) + 1] = 0xFF;
            pbuf[(4 * i + (height - j - 1) * width * 4) + 2] = 0xFF;
            pbuf[(4 * i + (height - j - 1) * width * 4) + 3] = vl;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pbuf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);    // 设置纹理包裹方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // 设置纹理过滤
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    delete[] pbuf;
    return true;
}

CharTexture* TextRenderer::GetTextChar(char ch)
{
    if(!textures[ch].texID)
        LoadChar(ch);
    return &textures[ch];
}

void TextRenderer::DrawText(std::string str, int x0, int y0)
{
    float x = x0 / WindowWidth * 2, y = y0 / WindowHeight * 2;
    float w, h, ch_x, ch_y;


    glEnable(GL_TEXTURE_2D);
    for(int i = 0; i < str.length(); i++)
    {
        CharTexture* charTex = GetTextChar(str[i]);
        glBindTexture(GL_TEXTURE_2D, charTex->texID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        w = charTex->width; h = charTex->height;

        ch_x = x + charTex->start_x; ch_y = y + charTex->start_y;

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);glVertex3f(ch_x, ch_y, 1.0f);
        glTexCoord2f(1.0f, 0.0f);glVertex3f(ch_x + w, ch_y, 1.0f);
        glTexCoord2f(1.0f, 1.0f);glVertex3f(ch_x + w, ch_y + h, 1.0f);
        glTexCoord2f(0.0f, 1.0f);glVertex3f(ch_x, ch_y + h, 1.0f);
        glEnd();

        x += charTex->adv_x;
    }
}

bool TextRenderer::Load(const char* file, int width, int height)
{
    if(FT_Init_FreeType(&library))
    {
        //printf("library init failed\n");
        return false;
    }

    if(FT_New_Face(library, file, 0, &face))
    {
        //printf("load ttf failed\n");
        return false;
    }

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);

    if(FT_Set_Pixel_Sizes(face, width, height))
        return false;

    this->width = width;
    this->height = height;
    return true;
}

TextRenderer textRenderer;

int main(){

    InitWindow();

    textRenderer.Load(".\\res\\Arial.ttf", 32, 32);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.0f, 0.0f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        textRenderer.DrawText("freetype-demo", 0, 0);

        glfwSwapBuffers(window);
    }

    glfwTerminate();


    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void InitWindow()
{
    glfwInit();
    // Do not do this settings, otherwise textrendering can not work.
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(WindowWidth, WindowHeight, WindowName, nullptr, nullptr);

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
}
