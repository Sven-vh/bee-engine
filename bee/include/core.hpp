#pragma once

#include "common.hpp"

#include "core/input.hpp"
#include "core/audio.hpp"
#include "core/Layer.hpp"
#include "core/engine.hpp"
#include "core/device.hpp"
#include "core/fileio.hpp"

#include "resource/resourceManager.hpp"
#include "managers/render_manager.hpp"
#include "managers/grid_manager.hpp"
#include "managers/undo_redo_manager.hpp"

#include "tools/ease.hpp"
#include "tools/shapes.hpp"
#include "tools/gradient.hpp"
#include "tools/profiler.hpp"
#include "tools/raycasting.hpp"
#include "tools/file_dialog.hpp"
#include "tools/imguiHelper.hpp"
#include "tools/cerealHelper.hpp"

#include "input/KeyCode.hpp"
#include "input/MouseCode.hpp"

#include "events/Event.hpp"
#include "events/KeyEvent.hpp"
#include "events/KeyEvent.hpp"
#include "events/MouseEvent.hpp"
#include "events/ApplicationEvent.hpp"

#include "ecs/components.hpp"
#include "ecs/enttHelper.hpp"
#include "ecs/enttCereal.hpp"
#include "ecs/componentInspector.hpp"
#include "ecs/componentInitialize.hpp"
#include "managers/scene_manager.hpp"

#include "xsr/include/xsr.hpp"

#include "rendering/FrameBuffer.hpp"
#include "rendering/RenderingHelper.hpp"

#include "math/math.hpp"

#include "tools/Tweening/tween_system.hpp"

#include "resource/gltfLoader.hpp"

#ifdef BEE_PLATFORM_PC
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <GLFW/glfw3.h>
#endif