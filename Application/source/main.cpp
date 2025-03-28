#include "core/engine.hpp"
#include "ApplicationLayer.h"
#include "CarGame.h"
//#include <iostream>

int main()
{
    EngineSettings settings;
    settings.projectPath = "Application";
    bee::Engine.Initialize(settings);
    bee::Engine.PushAppLayer(new CarGame());
    bee::Engine.Run();
    bee::Engine.Shutdown();
    return 0;
}


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/