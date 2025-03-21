#include "core/input.hpp"
#include "platform/pc/input_pc.hpp"
//used to be a cross platform input class, but now it's only for pc
bee::Input* bee::Input::Create()
{
    return new bee::InputPc();
}