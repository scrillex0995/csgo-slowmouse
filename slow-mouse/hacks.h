#pragma once
#include "memory.h"
#include "sdk/offsets.h"

namespace hacks
{
	void VisualThread(const Memory& mem) noexcept;
	void AimbotThread(const Memory& mem) noexcept;
	void MiscThread(const Memory& mem) noexcept;
}