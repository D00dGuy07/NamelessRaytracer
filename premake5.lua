workspace "NamelessRaytracer"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.system}-%{cfg.architecture}-%{cfg.buildcfg}"

project "NamelessRaytracer"
	location "NamelessRaytracer"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/")
	objdir ("bin-int/" .. outputdir .. "/")

	defines "_CRT_SECURE_NO_WARNINGS" -- For GLFW

	files
	{
		-- My code
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",

		-- GLM code
		"%{prj.name}/vendor/glm/glm/**.h",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",

		-- TinyGLTF and included stb_image code
		"%{prj.name}/vendor/tinygltf/stb_image.h",
		"%{prj.name}/vendor/tinygltf/stb_image_write.h",
		"%{prj.name}/vendor/tinygltf/json.hpp",
		"%{prj.name}/vendor/tinygltf/tiny_gltf.h",
		"%{prj.name}/vendor/tinygltf/tiny_gltf.cc",

		-- Generated glad OpenGL loader code
		"%{prj.name}/vendor/glad/include/glad/glad.h",
		"%{prj.name}/vendor/glad/src/glad.c",

		-- GLFW generic code
		"%{prj.name}/vendor/glfw/include/GLFW/glfw3.h",
		"%{prj.name}/vendor/glfw/include/GLFW/glfw3native.h",
		"%{prj.name}/vendor/glfw/src/context.c",
		"%{prj.name}/vendor/glfw/src/init.c",
		"%{prj.name}/vendor/glfw/src/input.c",
		"%{prj.name}/vendor/glfw/src/internal.h",
		"%{prj.name}/vendor/glfw/src/mappings.h",
		"%{prj.name}/vendor/glfw/src/monitor.c",
		"%{prj.name}/vendor/glfw/src/platform.c",
		"%{prj.name}/vendor/glfw/src/platform.h",
		"%{prj.name}/vendor/glfw/src/vulkan.c",
		"%{prj.name}/vendor/glfw/src/window.c",

		"%{prj.name}/vendor/glfw/src/null_init.c",
		"%{prj.name}/vendor/glfw/src/null_joystick.c",
		"%{prj.name}/vendor/glfw/src/null_joystick.h",
		"%{prj.name}/vendor/glfw/src/null_monitor.c",
		"%{prj.name}/vendor/glfw/src/null_platform.h",
		"%{prj.name}/vendor/glfw/src/null_window.c",

		-- imgui code
		"%{prj.name}/vendor/imgui/imconfig.h",
		"%{prj.name}/vendor/imgui/imgui.cpp",
		"%{prj.name}/vendor/imgui/imgui.h",
		"%{prj.name}/vendor/imgui/imgui_demo.cpp",
		"%{prj.name}/vendor/imgui/imgui_draw.cpp",
		"%{prj.name}/vendor/imgui/imgui_internal.h",
		"%{prj.name}/vendor/imgui/imgui_tables.cpp",
		"%{prj.name}/vendor/imgui/imgui_widgets.cpp",
		"%{prj.name}/vendor/imgui/imstb_rectpack.h",
		"%{prj.name}/vendor/imgui/imstb_textedit.h",
		"%{prj.name}/vendor/imgui/imstb_truetype.h",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_glfw.cpp",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_glfw.h",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_opengl3.cpp",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_opengl3.h",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_opengl3_loader.h"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/glm",
		"%{prj.name}/vendor/tinygltf",
		"%{prj.name}/vendor/glad/include",
		"%{prj.name}/vendor/glfw/include",
		"%{prj.name}/vendor/imgui"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "10.0.19041.0"

		defines "_GLFW_WIN32"

		files
		{
			-- GLFW win32 implementation
			"%{prj.name}/vendor/glfw/src/egl_context.c",
			"%{prj.name}/vendor/glfw/src/osmesa_context.c",
			"%{prj.name}/vendor/glfw/src/wgl_context.c",
			"%{prj.name}/vendor/glfw/src/win32_init.c",
			"%{prj.name}/vendor/glfw/src/win32_joystick.c",
			"%{prj.name}/vendor/glfw/src/win32_joystick.h",
			"%{prj.name}/vendor/glfw/src/win32_module.c",
			"%{prj.name}/vendor/glfw/src/win32_monitor.c",
			"%{prj.name}/vendor/glfw/src/win32_platform.h",
			"%{prj.name}/vendor/glfw/src/win32_thread.c",
			"%{prj.name}/vendor/glfw/src/win32_thread.h",
			"%{prj.name}/vendor/glfw/src/win32_time.c",
			"%{prj.name}/vendor/glfw/src/win32_time.h",
			"%{prj.name}/vendor/glfw/src/win32_window.c"
		}

	filter "configurations:Debug"
		defines "DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "DIST"
		optimize "On"