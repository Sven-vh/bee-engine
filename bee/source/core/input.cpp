#include "core/input.hpp"
#include "platform/pc/input_pc.hpp"
//used to be a cross platform input class, but now it's only for pc
bee::Input* bee::Input::Create()
{
    return new bee::InputPc();
}


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/