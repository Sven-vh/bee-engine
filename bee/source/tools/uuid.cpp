#include "tools/uuid.hpp"

#include <random>

namespace bee
{
static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

//bee::UUID::UUID() : m_UUID(s_UniformDistribution(s_Engine)) {}
//
//UUID::UUID(uint64_t uuid) : m_UUID(uuid) {}

}  // namespace bee