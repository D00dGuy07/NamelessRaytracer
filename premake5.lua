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

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",

		"%{prj.name}/vendor/glm/glm/**.h",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",

		"%{prj.name}/vendor/tinygltf/stb_image.h",
		"%{prj.name}/vendor/tinygltf/stb_image_write.h",
		"%{prj.name}/vendor/tinygltf/json.hpp",
		"%{prj.name}/vendor/tinygltf/tiny_gltf.h",
		"%{prj.name}/vendor/tinygltf/tiny_gltf.cc"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/glm",
		"%{prj.name}/vendor/tinygltf"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "10.0.19041.0"

	filter "configurations:Debug"
		defines "DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "DIST"
		optimize "On"