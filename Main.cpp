#include <gl/glew.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#include <GLFW/glfw3native.h>
#include <cstdio>
#include <memory>

#include "SpoutGL/SpoutReceiver.h"
#include "renderstream.hpp"

GLint toGlInternalFormat(RSPixelFormat format)
{
    switch (format)
    {
    case RS_FMT_BGRA8:
    case RS_FMT_BGRX8:
        return GL_RGBA8;
    case RS_FMT_RGBA32F:
        return GL_RGBA32F;
    case RS_FMT_RGBA16:
        return GL_RGBA16;
    case RS_FMT_RGBA8:
    case RS_FMT_RGBX8:
        return GL_RGBA8;
    default:
        throw std::runtime_error("Unhandled RS pixel format");
    }
}

GLint toGlFormat(RSPixelFormat format)
{
    switch (format)
    {
    case RS_FMT_BGRA8:
    case RS_FMT_BGRX8:
        return GL_BGRA;
    case RS_FMT_RGBA32F:
    case RS_FMT_RGBA16:
    case RS_FMT_RGBA8:
    case RS_FMT_RGBX8:
        return GL_RGBA;
    default:
        throw std::runtime_error("Unhandled RS pixel format");
    }
}

GLenum toGlType(RSPixelFormat format)
{
    switch (format)
    {
    case RS_FMT_BGRA8:
    case RS_FMT_BGRX8:
        return GL_UNSIGNED_BYTE;
    case RS_FMT_RGBA32F:
        return GL_FLOAT;
    case RS_FMT_RGBA16:
        return GL_UNSIGNED_SHORT;
    case RS_FMT_RGBA8:
    case RS_FMT_RGBX8:
        return GL_UNSIGNED_BYTE;
    default:
        throw std::runtime_error("Unhandled RS pixel format");
    }
}

int main() {
//    while (!::IsDebuggerPresent())
 //       ::Sleep(100);
    //Setup receiver
    SpoutReceiver sRecv;

    //Setup Opengl

    //Enable experimental extensions
    glewExperimental = GL_TRUE;


    //Initialize glfw window system
    if (!glfwInit()) {

        std::printf("GLFW failed to init\n");
        glfwTerminate();
        return 1;
    }

    //Set Opengl Versions (P.S. If you accidentally put two of these like I did you will get a very strange read access error or something IDK)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    //Makes the window all floaty and see-through.
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);

    //A modern (and possibly messy) window pointer setup.
    //void(*)(GLFWwindow*) is a placeholder (any) type for the last arguments which is what will be called when the pointer needs to be released.
    std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)> window(glfwCreateWindow(1280, 720, "Hello World", nullptr, nullptr), glfwDestroyWindow);

    //Check that the pointer is not a nullptr
    if (!window) {
        std::printf("Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    int bufferWidth, bufferHeight;

    //Get the final draw buffer size.
    glfwGetFramebufferSize(window.get(), &bufferWidth, &bufferHeight);

    //I kind of don't know exactly what this does yet.
    glfwMakeContextCurrent(window.get());

    //Initializes glew.
    if (glewInit() != GLEW_OK) {
        std::printf("Failed to initialize GLEW\n");
        //Old way of handling this.
            //glfwDestroyWindow(window.get());
        //Smart way of handling this.
        window.reset();
        glfwTerminate();
        return 1;
    }

    //Create renderstream object

    RenderStream rs;

    //Initialize the renderstream system.
    //This may produce an exception on designer machines.
    //I also found out it will do this if it is not compiled for x64 :(
    rs.initialise();

    //Get the window contexts from the native platform.
    //Since RS is windows only we can just get the native windows types directly.
    //Alternative is to wrap it in platform agnostic class.
    HGLRC hc = glfwGetWGLContext(window.get());
    HDC dc = GetDC(glfwGetWin32Window(window.get()));

    //Check that theese are not null.
    if (!hc) {
        std::printf("Unable to get WGL Context\n");
        return 1;
    }

    if (!dc) {
        std::printf("Unable to get native window\n");
        return 1;

    }

    //Initialize renderstream with opengl support.
    rs.initialiseGpGpuWithOpenGlContexts(hc, dc);


    //Setup a stream descriptions pointer.
    //This does the same vodo magic as the window smart pointer.
   std::unique_ptr<const StreamDescriptions> header(nullptr);


   //Sets up the rendertarget structs for RS.
   //Once a texture is tied to an fbo, you basically only work on the fbo from that point on.
    struct RenderTarget {
        GLuint texture;
        GLuint frameBuffer;
    };

    //Create the spout specific rendertarget.
    RenderTarget SpoutTarget;


//Prepare texture for spout receiver
    glGenTextures(1, &SpoutTarget.texture);
    if (glGetError() != GL_NO_ERROR)
        throw std::runtime_error("Failed to generate render target texture for stream");

    glBindTexture(GL_TEXTURE_2D, SpoutTarget.texture);
    {
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to bind render target texture for stream");

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to setup render target texture parameters for spout");


        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to create render target texture for spout");
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &SpoutTarget.frameBuffer);
    if (glGetError() != GL_NO_ERROR)
        throw std::runtime_error("Failed to create render target framebuffer for stream");

    glBindFramebuffer(GL_FRAMEBUFFER, SpoutTarget.frameBuffer);
    {
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to bind render target framebuffer for stream");

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, SpoutTarget.texture, 0);
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to attach render target texture for stream");

        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, buffers);
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to set draw buffers for stream");

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Failed fame buffer status check");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //Generate Map for render targets.
    std::unordered_map<StreamHandle, RenderTarget> renderTargets;


    while (!glfwWindowShouldClose(window.get())) {
        glfwPollEvents();

        int SpoutWidth = 0;
        int SpoutHeight = 0;

                if (sRecv.IsUpdated()) {
                    glBindTexture(GL_TEXTURE_2D, SpoutTarget.texture);
                    {
                        if (glGetError() != GL_NO_ERROR)
                            throw std::runtime_error("Failed to bind render target texture for stream");
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sRecv.GetSenderWidth(), sRecv.GetSenderHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                        if (glGetError() != GL_NO_ERROR)
                            throw std::runtime_error("Failed to generate spout texture");
                    }
                    glBindTexture(GL_TEXTURE_2D, 0);
                    SpoutWidth = sRecv.GetSenderWidth();
                    SpoutHeight = sRecv.GetSenderHeight();
                }
                //Need to create a spout specific texture to read into.
                //Putting true in the function fixes the inverted texture display which I'm too much of a n00b to solve.
                if (sRecv.ReceiveTexture(SpoutTarget.texture, GL_TEXTURE_2D, true)) {
                    std::printf("frame received\n");
                }
                if (glGetError() != GL_NO_ERROR)
                    throw std::runtime_error("Failed Receiving frame from spout.");

                //Clears the render window.
                glClearColor(0.f, 0.f, 0.f, 0.f);
                glClear(GL_COLOR_BUFFER_BIT);


                //Copy from one framebuffer to another.

                glBindFramebuffer(GL_READ_FRAMEBUFFER, SpoutTarget.frameBuffer);
                {
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); 
                    {
                        if (glGetError() != GL_NO_ERROR)
                            throw std::runtime_error("Failed to bind render target texture for stream");
                        glBlitFramebuffer(0, 0, sRecv.GetSenderWidth(), sRecv.GetSenderHeight(), 0, 0, 1280, 720,
                            GL_COLOR_BUFFER_BIT, GL_NEAREST);
                        if (glGetError() != GL_NO_ERROR)
                            throw std::runtime_error("Failed to bind render target texture for stream");
                    }
                }
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

                //It works up to here
               

                //Delete this block if it breaks
                //Handle sending the frame to renderstream.
                auto awaitResult = rs.awaitFrameData(5000);
                if (std::holds_alternative<RS_ERROR>(awaitResult))
                {
                    RS_ERROR err = std::get<RS_ERROR>(awaitResult);
                    //Update the streams pointer when a change error occurs. 
                    if (err == RS_ERROR_STREAMS_CHANGED)
                    {
                        header.reset(rs.getStreams());
                        const size_t numStreams = header ? header->nStreams : 0;
                        for (size_t i = 0; i < numStreams; ++i) {
                            const StreamDescription& description = header->streams[i];
                            RenderTarget& target = renderTargets[description.handle];
                            glGenTextures(1, &target.texture);
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to generate render target texture for stream");

                            glBindTexture(GL_TEXTURE_2D, target.texture);
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to bind render target texture for stream");

                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to setup render target texture parameters for stream");

                            glTexImage2D(GL_TEXTURE_2D, 0, toGlInternalFormat(description.format), description.width, description.height, 0, toGlFormat(description.format), toGlType(description.format), nullptr);
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to create render target texture for stream");
                            glBindTexture(GL_TEXTURE_2D, 0);

                            glGenFramebuffers(1, &target.frameBuffer);
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to create render target framebuffer for stream");

                            glBindFramebuffer(GL_FRAMEBUFFER, target.frameBuffer);
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to bind render target framebuffer for stream");

                            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target.texture, 0);
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to attach render target texture for stream");

                            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
                            glDrawBuffers(1, buffers);
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to set draw buffers for stream");

                            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                                throw std::runtime_error("Failed fame buffer status check");

                            glBindFramebuffer(GL_FRAMEBUFFER, 0);

                        }

                        std::printf("Found %d Streams\n", header->nStreams);
                        continue;
                    }
                    else if (err == RS_ERROR_TIMEOUT)
                    {
                        continue;
                    }
                    else if (err != RS_ERROR_SUCCESS)
                    {
                        std::printf("rs_awaitFrameData returned %d", err);
                        break;
                    }
                }

                const FrameData& frameData = std::get<FrameData>(awaitResult);
                const size_t numStreams = header ? header->nStreams : 0;
                for (size_t i = 0; i < numStreams; ++i) {
                    const StreamDescription& description = header->streams[i];

                    CameraResponseData cameraData;
                    cameraData.tTracked = frameData.tTracked;
                    try
                    {
                        cameraData.camera = rs.getFrameCamera(description.handle);
                    }
                    catch (const RenderStreamError& e)
                    {
                        /*
                        if (e.error == RS_ERROR_NOTFOUND)
                            continue;
                        throw;
                        */
                    }

                    {

                        glFinish();

                        glViewport(0, 0, SpoutWidth, SpoutHeight);

                        const RenderTarget& target = renderTargets.at(description.handle);
                        //Set this back to 0
                        glBindFramebuffer(GL_READ_FRAMEBUFFER, SpoutTarget.frameBuffer);
                        {
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to bind 1111 read fbo");
                            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.frameBuffer);
                            {
                                if (glGetError() != GL_NO_ERROR)
                                    throw std::runtime_error("Failed to bind read fbo");
                                glBlitFramebuffer(0, 0, SpoutWidth, SpoutHeight, 0, 0, description.width, description.height,
                                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
                                if (glGetError() != GL_NO_ERROR)
                                    throw std::runtime_error("Failed to blit.");
                            }
                        }
                        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                        if (glGetError() != GL_NO_ERROR)
                            throw std::runtime_error("Failed to bind 000 read fbo");


                        SenderFrameTypeData data;
                        data.gl.texture = target.texture;

                        //Send the frame to renderstream
                        //I would hope this would generate some form of error, but it doesn't.
                        rs.sendFrame(description.handle, RS_FRAMETYPE_OPENGL_TEXTURE, data, &cameraData);

                    }

                }

                //Delete block

                glfwSwapBuffers(window.get());

               
            
    }
	return 0;
}