#include "core/device.hpp"
#include "platform/opengl/device_gl.hpp"

//used to be a cross platform device class, but now it's only for opengl
bee::Device* bee::Device::Create()
{
    return new bee::OpenGLDevice();
}