#pragma once

#include "core.hpp"
#include "managers/particle_manager.hpp"
#include "core/engine.hpp"

// using std::exception to write a custom exception
#define GAME_EXCEPTION(msg) throw std::runtime_error(msg)
#define GAME_EXCEPTION_IF(condition, msg) \
    if (condition) GAME_EXCEPTION(("(" + m_name + ") ") + msg)