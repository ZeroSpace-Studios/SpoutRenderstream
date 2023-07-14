#include <gl/glew.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#define NOMINMAX

#include <GLFW/glfw3native.h>
#include <cstdio>
#include <memory>
#include <vector>

#include "../SpoutGL/SpoutReceiver.h"
#include "../SpoutGL/SpoutSender.h"
#include "../includes/renderstream.hpp"
#include "argparse.hpp"

// TODO
// Add cli args for stream name and window size
// Add a way to get the stream name from the sender
// Add a scoped schema for renderstream

typedef struct RenderTarget
{
    GLuint texture;
    GLuint frameBuffer;
} RenderTarget;


struct free_delete
{
    void operator()(void* x) { free(x); }
};

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



float randomFloat()
{
    return (float)(rand()) / (float)(RAND_MAX);
}

std::vector<std::string> getSpoutSenders(SpoutReceiver& sRecv) {
    int c = sRecv.GetSenderCount();
    std::vector<std::string> senders;
    for (int i = 0; i < c; i++) {
        char name[256];
        sRecv.GetSender(i, name);
        senders.push_back(std::string(name));
      //  delete name;
    }
    return senders;
}


void generateSchema(std::vector<std::string> &senders, ScopedSchema& schema, bool genInputs, bool genOutputs = true) {
   
    //Change the below line to use a smart pointer

    schema.schema.engineName = _strdup("SpoutRenderStream");
    schema.schema.engineVersion = _strdup("1.8");
    schema.schema.info = _strdup("");
    schema.schema.pluginVersion = _strdup("2.0");

    // std::vector<const char*> channels;
    // channels.push_back("Default");

	// schema.schema.channels.nChannels = channels.size();
    // schema.schema.channels.channels = channels.data();
    
    if (!genOutputs) {
        schema.schema.scenes.nScenes = 1;
        schema.schema.scenes.scenes = static_cast<RemoteParameters*>(
            malloc(
                schema.schema.scenes.nScenes * sizeof(RemoteParameters)
            )
            );
        RemoteParameters rp = {
            "Default",
            1,
            nullptr,
        };
        schema.schema.scenes.scenes[0] = rp;
        schema.schema.scenes.scenes[0].parameters = static_cast<RemoteParameter*>(
            malloc(
                schema.schema.scenes.scenes[0].nParameters * sizeof(RemoteParameter)
            )
        );
        RemoteParameterTypeDefaults defaults;
        RemoteParameter par;
        par.group = "Input";
        par.displayName = "SpoutSource";
        par.key = "spout_input";
        par.type = RS_PARAMETER_IMAGE;
        par.nOptions = 0;
        par.options = nullptr;
        par.dmxOffset = -1;
        par.dmxType = RS_DMX_16_BE;
        par.flags = REMOTEPARAMETER_NO_FLAGS;
        schema.schema.scenes.scenes[0].parameters[0] = par;
        return;
    }

    schema.schema.scenes.nScenes = senders.size();
    schema.schema.scenes.scenes = static_cast<RemoteParameters*>(
        malloc(
            schema.schema.scenes.nScenes * sizeof(RemoteParameters)
        )
    );

    for (int i = 0; i < schema.schema.scenes.nScenes; i++) {
        RemoteParameters rp = {
            senders[i].c_str(),
            0,
            nullptr,
        };
        schema.schema.scenes.scenes[i] = rp;

        if (genInputs)
        {
            schema.schema.scenes.scenes[i].nParameters = 1;
            schema.schema.scenes.scenes[i].parameters = static_cast<RemoteParameter*>(
                malloc(
                    schema.schema.scenes.scenes[i].nParameters * sizeof(RemoteParameter)
                )
            );

            //std::shared_ptr<RemoteParameter> param (static_cast<RemoteParameter*>(malloc(schema.schema.scenes.scenes[i].nParameters * sizeof(RemoteParameter))), free_delete());

           // schema.schema.scenes.scenes[i].parameters = std::move(param.get());

            RemoteParameterTypeDefaults defaults;
            RemoteParameter par;
            par.group = "Input";
            par.displayName = "SpoutSource";
            par.key = "spout_input";
            par.type = RS_PARAMETER_IMAGE;
            par.nOptions = 0;
            par.options = nullptr;
            par.dmxOffset = -1;
            par.dmxType = RS_DMX_16_BE;
            par.flags = REMOTEPARAMETER_NO_FLAGS;
            schema.schema.scenes.scenes[i].parameters[0] = par;
        }
    };
}


void generateGlTexture(RenderTarget& target, const int width, const int height, RSPixelFormat format) {
    //Generate opengl texture and frame buffer
    glGenTextures(1, &target.texture);
    if (glGetError() != GL_NO_ERROR)
        throw std::runtime_error("Failed to generate render target texture for stream");

    glBindTexture(GL_TEXTURE_2D, target.texture);
    {
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to bind render target texture for stream");

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        
        //May no longer be needed.

        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to setup render target texture parameters");

        glTexImage2D(GL_TEXTURE_2D, 0, toGlInternalFormat(format), width, height, 0, toGlFormat(format), toGlType(format), nullptr);
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("Failed to create render target texture for stream");
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &target.frameBuffer);
    if (glGetError() != GL_NO_ERROR)
        throw std::runtime_error("Failed to create render target framebuffer for stream");

    glBindFramebuffer(GL_FRAMEBUFFER, target.frameBuffer);
    {
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
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}


int main(int argc, char* argv[])
{
   // while (!::IsDebuggerPresent())
   //     ::Sleep(100);


  // Setup argpraeser
    argparse::ArgumentParser program("ZeroSpace SpoutBridge");


    program.add_argument("--windowed", "-w").help("Render Visible Window")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--remove_sender_names", "-r").help("Sets the default behavior to remove sender names from the list of available senders.")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--inputs", "-i").help("Presents a texture input from disguise as a RenderStream Spout source.")
		.default_value(false)
		.implicit_value(true);

    program.add_argument("--disable_outputs").help("Disables outputs, for use when wanting to just send an output.")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        //PNL(err.what());
        std::printf("Error: %s\n", err.what());
        std::exit(1);
    }

    // Setup receiver
    SpoutReceiver sRecv;

    //Setup Sender
    SpoutSender sSend;
    sSend.SetSenderName("RenderStream");

    // Setup Opengl

    // Enable experimental extensions
    glewExperimental = GL_TRUE;

    // Initialize glfw window system
    if (!glfwInit())
    {

        std::printf("GLFW failed to init\n");
        glfwTerminate();
        return 1;
    }

    // Set Opengl Versions (P.S. If you accidentally put two of these like I did you will get a very strange read access error or something IDK)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Makes the window all floaty and see-through.
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);

    //Control window visibility
    bool isWindowed = false;
    if (program["--windowed"] == true) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        isWindowed = true;
    }
    else {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    }

    bool isRemoveSenders = false;
    if (program["--remove_sender_names"] == true) {
        isRemoveSenders = true;
    }

	bool isInput = false;
    if (program["--inputs"] == true) {
		isInput = true;
    }

    bool isOutputDisabled = false;
    if (program["--disable_outputs"] == true) {
        isOutputDisabled = true;
    }


    // A modern (and possibly messy) window pointer setup.
    // void(*)(GLFWwindow*) is a placeholder (any) type for the last arguments which is what will be called when the pointer needs to be released.
    std::unique_ptr<GLFWwindow, void (*)(GLFWwindow*)> window(glfwCreateWindow(1280, 720, "ZeroSpace SpoutBridge (Non-Commerical)", nullptr, nullptr), glfwDestroyWindow);

    // Check that the pointer is not a nullptr
    if (!window)
    {
        std::printf("Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    int bufferWidth, bufferHeight;

    // Get the final draw buffer size.
    glfwGetFramebufferSize(window.get(), &bufferWidth, &bufferHeight);

    // I kind of don't know exactly what this does yet.
    glfwMakeContextCurrent(window.get());

    // Initializes glew.
    if (glewInit() != GLEW_OK)
    {
        std::printf("Failed to initialize GLEW\n");
        // Old way of handling this.
        // glfwDestroyWindow(window.get());
        // Smart way of handling this.
        window.reset();
        glfwTerminate();
        return 1;
    }

    // Create renderstream object

    RenderStream rs;

    // Initialize the renderstream system.
    // This may produce an exception on designer machines.
    // I also found out it will do this if it is not compiled for x64 :(
    rs.initialise();

    // Get the window contexts from the native platform.
    // Since RS is windows only we can just get the native windows types directly.
    // Alternative is to wrap it in platform agnostic class.
    HGLRC hc = glfwGetWGLContext(window.get());
    HDC dc = GetDC(glfwGetWin32Window(window.get()));

    // Check that theese are not null.
    if (!hc)
    {
        std::printf("Unable to get WGL Context\n");
        //d3 logging rs_logToD3();
        return 1;
    }

    if (!dc)
    {
        std::printf("Unable to get native window\n");
        return 1;
    }

    // Initialize renderstream with opengl support.
    rs.initialiseGpGpuWithOpenGlContexts(hc, dc);

    // Setup a stream descriptions pointer.
    // This does the same vodo magic as the window smart pointer.
    std::unique_ptr<const StreamDescriptions> header(nullptr);

    // Sets up the rendertarget structs for RS.
    // Once a texture is tied to an fbo, you basically only work on the fbo from that point on.

    // Create the spout specific rendertarget.
    RenderTarget SpoutTarget;

    // Set Height and Width For Receivers
    int SpoutWidth = 0;
    int SpoutHeight = 0;

    //Cleaner Implmentation
    //Must pass non 0 values
    generateGlTexture(SpoutTarget, 1280, 720, RS_FMT_RGBA32F);

    // Generate Map for render targets.
    std::unordered_map<StreamHandle, RenderTarget> renderTargets;

    // Spout Incoming texture target
    RenderTarget SpoutIncomingTarget;

    // Set Height and Width For Incoming target;
    int SpoutIncomingWidth = 0;
    int SpoutIncomingHeight = 0;


    std::vector<std::string> SenderNames;
    ScopedSchema schema;

    if (isOutputDisabled) {
        generateSchema(SenderNames, schema, isInput, false);
        rs.setSchema(&schema.schema);
        rs.saveSchema(argv[0], &schema.schema);
    } else
    //Setup loading the schema
    {
        // Loading a schema from disk is useful if some parts of it cannot be generated during runtime (ie. it is exported from an editor) 
        // or if you want it to be user-editable

        //Commenting this fixes the crash in RS2.0, I'm unsure why at the moment.
        
        const Schema* importedSchema = rs.loadSchema(argv[0]);
        if (importedSchema && importedSchema->scenes.nScenes > 0)
            std::printf("A schema existed on disk");
        
    }


    while (!glfwWindowShouldClose(window.get()))
    {
        // Get the sender names.

        glfwPollEvents();

        
        if (!isOutputDisabled)
        {
            std::vector<std::string> Senders = getSpoutSenders(sRecv);

            if (SenderNames.size() != Senders.size()) {
                //Handles adding new senders without breaking order;
                for (auto x : Senders) {
                    if (std::find(SenderNames.begin(), SenderNames.end(), x) == SenderNames.end()) {
                        SenderNames.push_back(x);
                    }
                    else {
                        if (isRemoveSenders) {
                            for (auto y : SenderNames) {
                                if (std::find(Senders.begin(), Senders.end(), y) == Senders.end()) {
                                    SenderNames.erase(std::remove(SenderNames.begin(), SenderNames.end(), y), SenderNames.end());
                                }
                            }

                                }
                            }
                    // SenderNames = Senders;
                    generateSchema(SenderNames, schema, isInput);
                    rs.setSchema(&schema.schema);
                    rs.saveSchema(argv[0], &schema.schema);
                        }
                    }

            if (sRecv.IsUpdated())
            {
                glBindTexture(GL_TEXTURE_2D, SpoutTarget.texture);
                {
                    if (glGetError() != GL_NO_ERROR)
                        throw std::runtime_error("Failed to bind render target texture for stream");
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sRecv.GetSenderWidth(), sRecv.GetSenderHeight(), 0, GL_RGBA, GL_FLOAT, nullptr);
                    if (glGetError() != GL_NO_ERROR)
                        throw std::runtime_error("Failed to generate spout texture");
                }
                glBindTexture(GL_TEXTURE_2D, 0);
                SpoutWidth = sRecv.GetSenderWidth();
                SpoutHeight = sRecv.GetSenderHeight();
            }
            // Need to create a spout specific texture to read into.
            // Putting true in the function fixes the inverted texture display which I'm too much of a n00b to solve.
            if (sRecv.ReceiveTexture(SpoutTarget.texture, GL_TEXTURE_2D, false))
            {
#ifdef DEBUG
                std::printf("frame received\n");
                PNL("frame received")
#else

#endif
            }
            if (glGetError() != GL_NO_ERROR)
                throw std::runtime_error("Failed Receiving frame from spout.");
        }
            // Clears the render window.
           // glClearColor(0.f, 0.f, 0.f, 0.f);
          //  glClear(GL_COLOR_BUFFER_BIT);

            // Copy from spout framebuffer to the main window FBO.

            if (isWindowed) {

                glBindFramebuffer(GL_READ_FRAMEBUFFER, SpoutTarget.frameBuffer);
                {
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                    {
                        if (glGetError() != GL_NO_ERROR)
                            throw std::runtime_error("Failed to bind render target texture for stream");

                        glViewport(0, 0, SpoutWidth, SpoutHeight);

                        glBlitFramebuffer(0, 0, SpoutWidth, SpoutHeight, 0, 720, 1280, 0,
                            GL_COLOR_BUFFER_BIT, GL_NEAREST);
                        if (glGetError() != GL_NO_ERROR)
                            throw std::runtime_error("Failed to bind render target texture for stream");
                    }
                }
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

            }


            // Handle sending the frame to renderstream.
            auto awaitResult = rs.awaitFrameData(5000);
            if (std::holds_alternative<RS_ERROR>(awaitResult))
            {
                RS_ERROR err = std::get<RS_ERROR>(awaitResult);
                // Update the streams pointer when a change error occurs.
                if (err == RS_ERROR_STREAMS_CHANGED)
                {
                    header.reset(rs.getStreams());
                    const size_t numStreams = header ? header->nStreams : 0;
                    for (size_t i = 0; i < numStreams; ++i)
                    {
                        const StreamDescription& description = header->streams[i];
                        RenderTarget& target = renderTargets[description.handle];
						generateGlTexture(target, description.width, description.height, description.format);
                    }

                    std::printf("Found %d Streams\n", header->nStreams);
                    // PNL(fmt::sprintf("Found %d Streams\n", header->nStreams))
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
            if (frameData.scene >= schema.schema.scenes.nScenes)
            {
                std::printf("Scene out of bounds\n");
                //   PNL("Scene out of bounds");
                continue;
            }

            if (!isOutputDisabled) {
                sRecv.SetReceiverName(SenderNames[frameData.scene].c_str());
            }
            //Handle receiving texture

            if (isInput) {
                //Should move this to another thread.

                const auto& scene = schema.schema.scenes.scenes[frameData.scene];
                ParameterValues values = rs.getFrameParameters(scene);

                ImageFrameData image = values.get<ImageFrameData>("spout_input");
                if (SpoutIncomingWidth != image.width || SpoutIncomingHeight != image.height)
                {
                    generateGlTexture(SpoutIncomingTarget, image.width, image.height, image.format);
                    SpoutIncomingWidth = image.width;
                    SpoutIncomingHeight = image.height;
                }
                SenderFrame Idata;
                Idata.type = RS_FRAMETYPE_OPENGL_TEXTURE;

                Idata.gl.texture = SpoutIncomingTarget.texture;

                rs.getFrameImage(image.imageId, Idata);

                //Ideally thise should operate in a separate thread.

                sSend.SendTexture(
                    Idata.gl.texture,
                    GL_TEXTURE_2D,
                    SpoutIncomingWidth,
                    SpoutIncomingHeight,
                    false
                );
            }
            if (!isOutputDisabled) {


                const size_t numStreams = header ? header->nStreams : 0;
                for (size_t i = 0; i < numStreams; ++i)
                {
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

                        const RenderTarget& target = renderTargets.at(description.handle);

                        glBindFramebuffer(GL_FRAMEBUFFER, target.frameBuffer);
                        {
                            if (glGetError() != GL_NO_ERROR)
                                throw std::runtime_error("Failed to bind xxx read fbo");
                            //  glClear(GL_COLOR_BUFFER_BIT);
                        }

                        glClearColor(0.f, 0.f, 0.f, 0.f);
                        glClear(GL_COLOR_BUFFER_BIT);
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);

                        //   glViewport(0, 0, SpoutWidth, SpoutHeight);

                           // Set this back to 0
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
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                        if (glGetError() != GL_NO_ERROR)
                            throw std::runtime_error("Failed to bind 000 read fbo");

                        //glFinish();

                        SenderFrame data;
                        data.type = RS_FRAMETYPE_OPENGL_TEXTURE;
                        data.gl.texture = target.texture;

                        FrameResponseData response = {};
                        response.cameraData = &cameraData;

                        // Send the frame to renderstream
                        // I would hope this would generate some form of error, but it doesn't.
                        rs.sendFrame(description.handle, data, response);
                    }
                   
                }
            }

            glfwSwapBuffers(window.get());
        }
        
    return 0;
}