#include "hacks.h"
#include "gui.h"
#include "globals.h"
#include "vector.h"

#include <thread>
#include <iostream>
#include <array>

using namespace hazedumper;
using namespace netvars;
using namespace signatures;

struct Color
{
	float r, g, b, a;
};

Color glow_color = {1.f, 1.f, 1.f, 1.f};

constexpr const void setGlowColor(float r, float g, float b, float a)
{
	glow_color.r = r;
	glow_color.g = g;
	glow_color.b = b;
	glow_color.a = a;
}

constexpr const Color healthGlowColor(float player_health)
{
	if (player_health > 60)
	{
		return Color(0.0f, 1.0f, 0.0f, globals::glowRGBA[3]);
	}
	else if (player_health > 35 && player_health < 60)
	{
		return Color(1.0f, 0.5f, 0.0f, globals::glowRGBA[3]);
	}
	else
	{
		return Color(1.0f, 0.0f, 0.0f, globals::glowRGBA[3]);
	}
}

void hacks::VisualThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		const auto local_player = mem.read<std::uintptr_t>(globals::client_dll + dwLocalPlayer);
		const auto lp_team = mem.read<std::uintptr_t>(local_player + m_iTeamNum);

		const auto glow_object_manager = mem.read<DWORD>(globals::client_dll + dwGlowObjectManager);
		const auto client_state = mem.read<DWORD>(globals::client_dll + dwClientState);

		if (!globals::glow)
			continue;

		if (!local_player)
			continue;

		for (int i = 0; i < 64; i++)
		{
			const auto entity = mem.read<DWORD>(globals::client_dll + dwEntityList + i * 0x10);

			const auto entity_team = mem.read<std::uintptr_t>(entity + m_iTeamNum);
			const auto entity_health = mem.read<std::uintptr_t>(entity + m_iHealth);

			const auto entity_glowIndex = mem.read<std::int32_t>(entity + m_iGlowIndex);

			if (!entity)
				continue;

			if (entity_team == lp_team && !globals::friendly)
				continue;

			if (globals::glowHealthColor)
			{
				glow_color = healthGlowColor(entity_health);
			}
			else
			{
				setGlowColor(
					globals::glowRGBA[0],
					globals::glowRGBA[1],
					globals::glowRGBA[2],
					globals::glowRGBA[3]
				);
			}
			mem.write<Color>(glow_object_manager + (entity_glowIndex * 0x38) + 0x8, glow_color);

			mem.write<bool>(glow_object_manager + (entity_glowIndex * 0x38) + 0x27, true);
			mem.write<bool>(glow_object_manager + (entity_glowIndex * 0x38) + 0x28, true);
		}
	}
}

constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles) noexcept
{
	return((enemyPosition - localPosition).ToAngle() - viewAngles);
}

void hacks::AimbotThread(const Memory& mem) noexcept
{
	while (gui::isRunning) 
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2));

		const auto local_player = mem.read<std::uintptr_t>(globals::client_dll + dwLocalPlayer);

		if (!local_player)
			continue;

		const auto lp_team = mem.read<std::uintptr_t>(local_player + m_iTeamNum);

		const auto client_state = mem.read<DWORD>(globals::engine_dll + dwClientState);

		auto bestFov = globals::aimbot_fov;
		auto bestAngle = Vector3{ };

		for (int i = 0; i < 64; i++)
		{

			//angles

			const auto origin = mem.read<Vector3>(local_player + m_vecOrigin);
			const auto viewOffset = mem.read<Vector3>(local_player + m_vecViewOffset);
			const auto viewAngles = mem.read<Vector3>(client_state + dwClientState_ViewAngles);
			const auto punchAngle = mem.read<Vector3>(local_player + m_aimPunchAngle) * 2;

			//mem.write<Vector3>(client_state + dwClientState_ViewAngles, Vector3(0.f, 0.f, 0.f));

			//entity

			const auto entity = mem.read<DWORD>(globals::client_dll + dwEntityList + i * 0x10);

			const auto entity_team = mem.read<std::uintptr_t>(entity + m_iTeamNum);
			const auto dormant = mem.read<bool>(entity + m_bDormant);
			const auto lifeState = mem.read<int32_t>(entity + m_lifeState);
			const auto spottedByMask = mem.read<int32_t>(entity + m_bSpottedByMask);
			const auto boneMatrix = mem.read<std::uintptr_t>(entity + m_dwBoneMatrix);

			if (!GetAsyncKeyState(VK_LBUTTON))
				continue;

			if (!globals::aimbot)
				continue;

			if (!entity)
				continue;

			if (entity_team == lp_team && !globals::friendly)
				continue;

			if (lifeState)
				continue;

			if (dormant)
				continue;

			if (!spottedByMask && !globals::aimthroughwalls)
				continue;

			// 8 is head index

			const auto localhealth = mem.read<std::uintptr_t>(local_player + m_iHealth);

			const auto entityHitboxPosition = Vector3(
				mem.read<float>(boneMatrix + 0x30 * globals::hitbox + 0x0C),
				mem.read<float>(boneMatrix + 0x30 * globals::hitbox + 0x1C),
				mem.read<float>(boneMatrix + 0x30 * globals::hitbox + 0x2C)
			);

			const auto local_eye_position = origin + viewOffset;

			const auto angle = CalculateAngle(
				local_eye_position,
				entityHitboxPosition,
				viewAngles + punchAngle
			);

			const auto fov = std::hypot(angle.x, angle.y);

			if (fov < bestFov)
			{
				bestFov = fov;
				bestAngle = angle;
			}

			Vector3 aimbotAngle = viewAngles + bestAngle / globals::smooth;

			if (aimbotAngle.x > 89.f)
				aimbotAngle.x = 88.f;

			if (aimbotAngle.x < -89.f)
				aimbotAngle.x = -88.f;

			if (aimbotAngle.y > 180.f)
				aimbotAngle.y = 179.f;

			if (aimbotAngle.y < -180.f)
				aimbotAngle.y = -179.f;

			aimbotAngle.z = 0.f;

			if (!bestAngle.IsZero())
			{
				mem.write<Vector3>(client_state + dwClientState_ViewAngles, aimbotAngle);
			}
		}
	}
}

constexpr const int GetWeaponPaint(const short& itemDefinition)
{
	switch (itemDefinition)
	{
	case 1: return 764; //Deagle
	case 4: return 957; //Glock
	case 7: return 724; //AK47
	case 9: return 344; //awp	
	case 40: return 996; //SSG08
	case 60: return 946; //m41s
	case 61: return 1040; //USP-S
	default: return 0;
	}
}

void hacks::MiscThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2));

		const auto local_player = mem.read<std::uintptr_t>(globals::client_dll + dwLocalPlayer);

		if (!local_player)
			continue;

		const auto lp_flags = mem.read<bool>(local_player + m_fFlags);
		const auto lp_weapons = mem.read<std::array<unsigned long, 8>>(local_player + m_hMyWeapons);
		const auto view_model = mem.read<int>(local_player + m_hViewModel) & 0xFFF;
	
		const auto client_state = mem.read<DWORD>(globals::engine_dll + dwClientState);

		//bunnyhop

		if (globals::bunnyhop && GetAsyncKeyState(VK_SPACE) && lp_flags & (1 << 0))
		{
			mem.write<BYTE>(globals::client_dll + dwForceJump, 6);
		}

		//skinchanger

		if (globals::skinchanger && local_player)
		{
			for (const auto handle : lp_weapons)
			{
				const auto lp_weapon = mem.read<DWORD>((globals::client_dll + dwEntityList + (handle & 0xFFF) * 0x10) - 0x10);
				const auto weapon_id = mem.read<short>(lp_weapon + m_iItemDefinitionIndex);
				const auto active_weapon = mem.read<DWORD>(globals::client_dll + dwEntityList + m_hActiveWeapon);
				
				if (!lp_weapon)
					continue;
				/*
				if (weapon_id == 42)
				{
					mem.write<short>(lp_weapon + m_iEntityQuality, 3);
					mem.write<short>(lp_weapon + m_iItemDefinitionIndex, 512);
					mem.write<int>(active_weapon + m_nModelIndex, 712);
				}
				*/
				if (const auto paint = GetWeaponPaint(weapon_id))
				{
					const bool shouldUpdate = mem.read<std::int32_t>(lp_weapon + m_nFallbackPaintKit) != paint;

					mem.write<std::int32_t>(lp_weapon + m_iItemIDHigh, -1);

					mem.write<std::int32_t>(lp_weapon + m_nFallbackPaintKit, paint);
					mem.write<float>(lp_weapon + m_flFallbackWear, 0.1f);

					if (shouldUpdate)
						mem.write<std::int32_t>(client_state + 0x174, -1);
				}
			}
		}

		if (globals::fov != 90 && local_player)
		{
			mem.write<int>(local_player + m_iDefaultFOV, globals::fov);
		}
	}
}
