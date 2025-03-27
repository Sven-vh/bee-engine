#include "bee.hpp"
#include "gameplay.hpp"

int main()
{
    EngineSettings settings;
    settings.projectPath = "MazeGame";
    bee::Engine.Initialize(settings);
    bee::Engine.PushAppLayer(new Gameplay());
    bee::Engine.Run();
    bee::Engine.Shutdown();
}


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/