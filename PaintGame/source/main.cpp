#include "core/engine.hpp"
#include "Gameplay.hpp"
#include "Physics.hpp"
#include "components.hpp"

int main()
{
    EngineSettings settings;
    settings.projectPath = "PaintGame";
    bee::Engine.Initialize(settings);
    bee::Engine.PushAppLayer(new Physics());
    bee::Engine.PushAppLayer(new Gameplay());
    bee::Engine.Run();
    bee::Engine.Shutdown();
    return 0;
}