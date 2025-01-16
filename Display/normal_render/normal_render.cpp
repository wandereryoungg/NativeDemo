/***********************************************************
 * Author        : 公众号: Android系统攻城狮
 * Create time   : 2023-07-24 01:26:21 星期一
 * Filename      : normal_render.cpp
 * Description   : Opengles渲染三角形示例
 ************************************************************/
#define LOG_TAG "normal_render"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <WindowSurface.h>
#include <stdio.h>
#include <stdlib.h>
#include <log/log.h>
#include <utils/Trace.h>
#include <cutils/trace.h>

#undef ATRACE_TAG
#define ATRACE_TAG ATRACE_TAG_ALWAYS

using namespace android;

GLuint gProgram;
GLuint gvPositionHandle;
const GLfloat gTriangleVertices[] = {0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f};

static const char gVertexShader[] =
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "}\n";

static const char gFragmentShader[] =
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*)malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    printf("%s   error: %s\n", __func__, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, pixelShader);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*)malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    printf("%s   error: %s\n", __func__, buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

void initShader() {
    gProgram = createProgram(gVertexShader, gFragmentShader);
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
}

void renderFrame() {
    ATRACE_CALL();
    ATRACE_BEGIN("young_glClearColor");
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    ATRACE_END();
    ATRACE_BEGIN("young_glClear");
    glClear(GL_COLOR_BUFFER_BIT);
    ATRACE_END();
    ATRACE_BEGIN("young_glUseProgram");
    glUseProgram(gProgram);
    ATRACE_END();
    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    glEnableVertexAttribArray(gvPositionHandle);
    ATRACE_BEGIN("young_glDrawArrays");
    glDrawArrays(GL_TRIANGLES, 0, 3);
    ATRACE_END();
}

int main(int argc, char** argv) {
    ATRACE_CALL();
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLint configAttribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                                EGL_RED_SIZE, 8,
                                EGL_GREEN_SIZE, 8,
                                EGL_BLUE_SIZE, 8,
                                EGL_ALPHA_SIZE, 8,
                                EGL_NONE};
    EGLint majorVersion;
    EGLint minorVersion;
    EGLContext context;
    EGLSurface surface;
    EGLDisplay eglDisplay;
    EGLint numConfigs = -1;
    EGLConfig config;
    ATRACE_BEGIN("young_windowSurface");
    WindowSurface windowSurface;
    ATRACE_END();
    EGLNativeWindowType window;

    ATRACE_BEGIN("young_eglGetDisplay");
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    ATRACE_END();

    ATRACE_BEGIN("young_eglInitialize");
    eglInitialize(eglDisplay, &majorVersion, &minorVersion);
    usleep(20000);
    ATRACE_END();
    ALOGI("majorVersion: %d minorVersion: %d\n", majorVersion, minorVersion);

    ATRACE_BEGIN("young_getSurface");
    window = windowSurface.getSurface();
    usleep(20000);
    ATRACE_END();

    eglChooseConfig(eglDisplay, configAttribs, &config, 1, &numConfigs);

    ATRACE_BEGIN("young_eglCreateWindowSurface");
    surface = eglCreateWindowSurface(eglDisplay, config, window, NULL);
    usleep(20000);
    ATRACE_END();

    context = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    eglMakeCurrent(eglDisplay, surface, surface, context);

    ATRACE_BEGIN("young_renderFrame");
    initShader();
    renderFrame();
    usleep(20000);
    ATRACE_END();

    ATRACE_BEGIN("young_eglSwapBuffers");
    eglSwapBuffers(eglDisplay, surface);
    ATRACE_END();
    getchar();
    return 0;
}
