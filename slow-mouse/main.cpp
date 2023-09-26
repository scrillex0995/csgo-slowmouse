#include "gui.h"
#include "hacks.h"
#include "globals.h"

#include <thread>

int __stdcall wWinMain(
	HINSTANCE instance,
	HINSTANCE previousInstance,
	PWSTR arguments,
	int commandShow)
{
	Memory mem{ "csgo.exe" };

	globals::client_dll = mem.GetModuleAddress("client.dll");
	globals::engine_dll = mem.GetModuleAddress("engine.dll");

	std::thread(hacks::VisualThread, mem).detach(); //can be compiler error :(
	std::thread(hacks::AimbotThread, mem).detach();
	std::thread(hacks::MiscThread, mem).detach();

	// create gui
	gui::CreateHWindow("svchost.exe");
	gui::CreateDevice();
	gui::CreateImGui();

	Beep(300, 500);

	while (gui::isRunning)
	{
		gui::BeginRender();
		gui::Render();
		gui::EndRender();

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	// destroy gui
	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return EXIT_SUCCESS;
}
