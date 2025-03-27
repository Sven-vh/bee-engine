#pragma once

// Standard Library headers
#include <map>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <numeric>
#include <stdint.h>
#include <iostream>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <unordered_map>
#include <utility>

// External Library headers
#include <cereal/cereal.hpp>
//#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal_optional_nvp.h>

#include "entt/entt.hpp"
#include "entt/meta/meta.hpp"
#include "entt/meta/factory.hpp"
#include "entt/meta/resolve.hpp"
#include "entt/meta/pointer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glmCereal.h>

#include <imgui/imgui.h>
#include <imgui/implot.h>
#include <imgui/ImGuizmo.h>
#define NOMINMAX
#include <imgui/imfilebrowser.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "editor/IconsFontAwesome6.hpp"

#include "defines.hpp"

// Bee headers
#include "tools/log.hpp"
#include "tools/tools.hpp"
#include "tools/warnings.hpp"
#include "tools/uuid.hpp"



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/