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