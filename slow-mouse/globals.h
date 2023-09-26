#pragma once
#include <cstdint>
#include <cstddef>
#include "hacks.h"

namespace globals
{
	inline std::uintptr_t client_dll = 0;
	inline std::uintptr_t engine_dll = 0;

	inline bool glow = false;
	inline float glowRGBA[] = { 1.f, 1.f, 1.f, 1.f };
	inline bool glowHealthColor = false;

	inline bool aimbot = false;
	inline float aimbot_fov = 2.f;
	inline float smooth = 4.f;
	inline int hitbox = 8;

	inline bool bunnyhop = false;
	inline bool skinchanger = false;
	inline bool friendly = false;
	inline int fov = 90.f;
	inline bool aimthroughwalls = false;
}