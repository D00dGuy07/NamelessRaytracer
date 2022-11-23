#include <iostream>
#include <random>
#include <fstream>

#include "glm/glm.hpp"
#include "tiny_gltf.h"
#include "stb_image_write.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad/glad.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Triangle intersection code

struct Ray
{
	glm::dvec3 Origin; // P
	glm::dvec3 Direction; // d
};

struct Triangle
{
	glm::dvec3 A;
	glm::dvec3 B;
	glm::dvec3 C;
};

glm::dvec3 RayEquation(Ray ray, double t)
{
	return ray.Origin + ray.Direction;
}

double RayPlaneIntersection(Ray ray, glm::dvec3 n, double d, bool* hit)
{
	double nd = glm::dot(n, ray.Direction);
	if (nd == 0.0f)
	{
		*hit = false;
		return 0.0;
	}
	*hit = true;

	double np = glm::dot(n, ray.Origin);

	return (d - np) / nd;
}

struct IntersectionResult
{
	bool IsHit;
	glm::dvec3 Normal;
	glm::dvec3 Position;
	glm::dvec3 Barycentric;
};

IntersectionResult RayTriangleIntersection(Ray ray, glm::dvec3 A, glm::dvec3 B, glm::dvec3 C)
{
	// Calculate plane normal and coefficient
	glm::dvec3 baca = glm::cross(B - A, C - A);
	glm::dvec3 normal = glm::normalize(baca);
	
	const double EPSILON = 0.0000001;
	glm::dvec3 edge1, edge2, h, s, q;
	double a, f, u, v;
	edge1 = B - A;
	edge2 = C - A;
	h = glm::cross(ray.Direction, edge2);
	a = glm::dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return { false, normal, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} };    // This ray is parallel to this triangle.
	f = 1.0 / a;
	s = ray.Origin - A;
	u = f * glm::dot(s, h);
	if (u < 0.0 || u > 1.0)
		return { false, normal, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} };
	q = glm::cross(s, edge1);
	v = f * glm::dot(ray.Direction, q);
	if (v < 0.0 || u + v > 1.0)
		return { false, normal, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} };
	// At this stage we can compute t to find out where the intersection point is on the line.
	double t = f * glm::dot(edge2, q);
	if (t > EPSILON) // ray intersection
	{
		glm::dvec3 Q = ray.Origin + ray.Direction * t;

		glm::dvec3 ba = glm::cross(B - A, Q - A);
		glm::dvec3 cb = glm::cross(C - B, Q - B);
		glm::dvec3 ac = glm::cross(A - C, Q - C);

		// Calculate barycentric coordinates
		double denominator = glm::dot(baca, normal);
		glm::dvec3 barycentric = {
			glm::dot(cb, normal) / denominator,
			glm::dot(ac, normal) / denominator,
			glm::dot(ba, normal) / denominator
		};

		return { true, normal, Q, barycentric };
	}
	else // This means that there is a line intersection but not a ray intersection.
		return { false, normal, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} };
}

// Camera code

double RandomDouble() {
	static std::uniform_real_distribution<double> distribution(0.0, 1.0);
	static std::mt19937 generator;
	return distribution(generator);
}

double RandomDouble(double min, double max) {
	// Returns a random real in [min,max).
	return min + (max - min) * RandomDouble();
}

// Entire camera structure adapted from raytracing in one weekend book
struct Camera
{
	glm::dvec3 Origin;
	glm::dvec3 UpperLeftCorner;
	glm::dvec3 Horizontal;
	glm::dvec3 Vertical;
	glm::dvec3 U, V, W;
	double LensRadius;

	Camera(
		glm::dvec3 lookFrom,
		glm::dvec3 lookAt,
		glm::dvec3 vup,
		double vfov, // vertical field-of-view in degrees
		double aspectRatio,
		double aperture,
		double focusDist)
	{
		double theta = glm::radians(vfov);
		double h = std::tan(theta / 2.0);
		double viewportHeight = 2.0 * h;
		double viewportWidth = aspectRatio * viewportHeight;

		W = glm::normalize(lookFrom - lookAt);
		U = glm::normalize(glm::cross(vup, W));
		V = glm::cross(W, U);

		Origin = lookFrom;
		Horizontal = focusDist * viewportWidth * U;
		Vertical = focusDist * viewportHeight * V;
		UpperLeftCorner = Origin - Horizontal / 2.0 + Vertical / 2.0 - focusDist * W;

		LensRadius = aperture / 2;
	}

	Ray GetRay(double s, double t) const
	{
		// Random in unit disk function flattened into here
		glm::dvec3 p;
		while (true) {
			p = { RandomDouble(-1.0, 1.0), RandomDouble(-1.0, 1.0), 0.0 } ;
			if ((p.x * p.x + p.y * p.y + p.z * p.z) >= 1.0) continue;
			break;
		}

		glm::dvec3 rd = LensRadius * p;
		glm::dvec3 offset = U * rd.x + V * rd.y;

		return {
			Origin + offset,
			UpperLeftCorner + s * Horizontal - t * Vertical - Origin - offset
		};
	}
};

// Image helper class

class PNGImage
{
public:

	PNGImage(int32_t width, int32_t height)
		: Width(width), Height(height)
	{
		m_Buffer = new uint8_t[width * height * 3];
	}

	~PNGImage() { delete[] m_Buffer; }

	glm::vec3 GetPixel(int32_t x, int32_t y)
	{
		if (x >= 0 && x < Width && y >= 0 && y < Height)
		{
			int32_t i = (x + y * Width) * 3;
			return {
				m_Buffer[i] / 255.0f,
				m_Buffer[i + 1] / 255.0f,
				m_Buffer[i + 2] / 255.0f,
			};
		}
		return { 0.0f, 0.0f, 0.0f };
	}

	void SetPixel(int32_t x, int32_t y, glm::vec3 color)
	{
		if (x >= 0 && x < Width && y >= 0 && y < Height)
		{
			int32_t i = (x + y * Width) * 3;
			m_Buffer[i] = static_cast<uint8_t>(std::clamp(color.x, 0.0f, 0.999f) * 255.0f);
			m_Buffer[i + 1] = static_cast<uint8_t>(std::clamp(color.y, 0.0f, 0.999f) * 255.0f);
			m_Buffer[i + 2] = static_cast<uint8_t>(std::clamp(color.z, 0.0f, 0.999f) * 255.0f);
		}
	}

	void WriteImage(const std::string& path)
	{
		stbi_write_png(path.c_str(), Width, Height, 3, m_Buffer, Width * 3);
	}

private:
	uint8_t* m_Buffer;

	int32_t Width, Height;
};

class PPMImage
{
public:

	PPMImage(int32_t width, int32_t height)
		: Width(width), Height(height)
	{
		m_Buffer = new glm::vec3[width * height];
	}

	~PPMImage() { delete[] m_Buffer; }

	glm::vec3 GetPixel(int32_t x, int32_t y)
	{
		if (x >= 0 && x < Width && y >= 0 && y < Height)
			return m_Buffer[x + y * Width];
		return { 0.0f, 0.0f, 0.0f };
	}

	void SetPixel(int32_t x, int32_t y, glm::vec3 color)
	{
		if (x >= 0 && x < Width && y >= 0 && y < Height)
			m_Buffer[x + y * Width] = color;
	}

	void WriteImage(const std::string& path)
	{
		std::ofstream outFile(path, std::ios::trunc);
		if (outFile.is_open())
		{
			// Write the file header
			outFile << "P3\n" << Width << " " << Height << "\n255\n";

			// Image needs to be damn flipped
			for (int32_t y = 0; y < Height; y++)
			{
				for (int32_t x = Width - 1; x >= 0; x--)
				{
					glm::vec3 color = m_Buffer[x + y * Width];
					// Convert the color from 0-1 to 0-255
					outFile << static_cast<int>(256 * std::clamp(color.x, 0.0f, 0.999f)) << ' '
						<< static_cast<int>(256 * std::clamp(color.y, 0.0f, 0.999f)) << ' '
						<< static_cast<int>(256 * std::clamp(color.z, 0.0f, 0.999f)) << '\n';
				}
			}
			
			outFile.close();
		}
	}

private:
	glm::vec3* m_Buffer;

	int32_t Width, Height;
};

// Model loading

struct TriangleRegistry
{
	// Vertex Data all in one contiguous buffer for cache locality
	// I hope that helps
	float* Buffer = nullptr;
	glm::vec3* Positions = nullptr;
	glm::vec3* Normals = nullptr;
	glm::vec4* Colors = nullptr;

	size_t VertexCount = 0;

	std::vector<glm::uvec3> Triangles;

	void Allocate(size_t vertexCount)
	{
		Buffer = new float[vertexCount * 10]; // 3 for position and normal, 4 for color
	}

	void Deallocate()
	{
		delete[] Buffer;
	}
};

const char* GLTFTypeName(int32_t gltfType)
{
	switch (gltfType)
	{
	case TINYGLTF_TYPE_SCALAR:
		return "SCALAR";
		break;
	case TINYGLTF_TYPE_VEC2:
		return "VEC2";
		break;
	case TINYGLTF_TYPE_VEC3:
		return "VEC3";
		break;
	case TINYGLTF_TYPE_VEC4:
		return "VEC4";
		break;
	case TINYGLTF_TYPE_MAT2:
		return "MAT2";
		break;
	case TINYGLTF_TYPE_MAT3:
		return "MAT3";
		break;
	case TINYGLTF_TYPE_MAT4:
		return "MAT4";
		break;
	default:
		return "UNKNOWN";
	}
}

const char* GLTFComponentTypeName(int32_t gltfComponentType)
{
	switch (gltfComponentType)
	{
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		return "BYTE";
		break;
	case TINYGLTF_COMPONENT_TYPE_DOUBLE:
		return "DOUBLE";
		break;
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		return "FLOAT";
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		return "INT";
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		return "SHORT";
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		return "UNSIGNED_BYTE";
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		return "UNSIGNED_INT";
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return "UNSIGNED_SHORT";
		break;
	default:
		return "UNKNOWN";
	}
}

const char* GLTFModeName(int32_t gltfMode)
{
	switch (gltfMode)
	{
	case TINYGLTF_MODE_POINTS:
		return "POINTS";
		break;
	case TINYGLTF_MODE_LINE:
		return "LINES";
		break;
	case TINYGLTF_MODE_LINE_LOOP:
		return "LINE_LOOP";
		break;
	case TINYGLTF_MODE_LINE_STRIP:
		return "LINE_STRIP";
		break;
	case TINYGLTF_MODE_TRIANGLES:
		return "TRIANGLES";
		break;
	case TINYGLTF_MODE_TRIANGLE_STRIP:
		return "TRIANGLE_STRIP";
		break;
	case TINYGLTF_MODE_TRIANGLE_FAN:
		return "TRIANGLE_FAN";
		break;
	default:
		return "UNKNOWN";
	}
}

template<typename T>
T* GetBufferLocation(tinygltf::Model& model, int32_t accessorIndex)
{
	auto& accessor = model.accessors[accessorIndex];
	auto& bufferView = model.bufferViews[accessor.bufferView];
	auto& buffer = model.buffers[bufferView.buffer];
	return reinterpret_cast<T*>(buffer.data.data() + bufferView.byteOffset);
}

// This function takes in a lot of data because it needs to print error messages with useful information
bool VerifyPrimitiveAttribute(tinygltf::Model& model, tinygltf::Primitive& primitive,
	const char* attributeName, int32_t requiredType, int32_t requiredComponentType, 
	const std::string& meshName, const char* attributeDescription)
{
	bool isValid = true;
	
	auto attribute = primitive.attributes.find(attributeName);
	if (attribute != primitive.attributes.end())
	{
		auto& accessor = model.accessors[attribute->second];
		if (accessor.type != requiredType)
		{
			std::cout << "ERROR: [" << meshName << "] Primitive found with " << attributeDescription << " type of '" <<
				GLTFTypeName(accessor.type) << "' instead of '" << GLTFTypeName(requiredType) << "'!\n";
			isValid = false;
		}

		if (accessor.componentType != requiredComponentType)
		{
			std::cout << "ERROR: [" << meshName << "] Primitive found with " << attributeDescription << " component type of '" <<
				GLTFComponentTypeName(accessor.componentType) << "' instead of '" << 
				GLTFComponentTypeName(requiredComponentType) << "'!\n";
			isValid = false;
		}
	}
	else
	{
		std::cout << "ERROR: [" << meshName << "] Primitive found without " << attributeDescription << " data!\n";
		isValid = false;
	}

	return isValid;
}

bool VerifyPrimitive(tinygltf::Model& model, tinygltf::Mesh& mesh, tinygltf::Primitive& primitive)
{
	bool isValid = true;

	// Mode needs to be TRIANGLES
	if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
	{
		std::cout << "ERROR: [" << mesh.name << "] Primitive found with mode '" 
			<< GLTFModeName(primitive.mode) << "' which is not supported! (Only 'TRIANGLES' is supported.)\n";
		isValid = false;
	}

	// Materials aren't supported but that just means it will be ignored
	if (primitive.material != -1)
	{
		std::cout << "WARNING: [" << mesh.name <<
			"] Primitive found with material specified. Materials will be ignored because they are not supported.\n";
	}

	// Vertex positions need to be VEC3 and of type FLOAT
	isValid &= VerifyPrimitiveAttribute(model, primitive,
		"POSITION", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT,
		mesh.name, "vertex position");

	// Vertex normals need to be VEC3 and of type FLOAT
	isValid &= VerifyPrimitiveAttribute(model, primitive,
		"NORMAL", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT,
		mesh.name, "vertex normal");

	// Vertex colors need to be VEC4 and of type UNSIGNED_SHORT
	isValid &= VerifyPrimitiveAttribute(model, primitive,
		"COLOR_0", TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
		mesh.name, "vertex color");

	// Indices need to be present. I'm gonna assume that they're okay if they're here
	if (primitive.indices == -1)
	{
		std::cout << "Error: [" << mesh.name << "] Primitive found with no indices!\n";
		isValid = false;
	}

	return isValid; // I sure hope this is enough error checking
}

TriangleRegistry LoadModel(const std::string& path)
{
	TriangleRegistry registry{};

	// Load the gltf with tinygltf
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	tinygltf::Model model;
	bool res = loader.LoadBinaryFromFile(&model, &err, &warn, path);

	if (!warn.empty())
		std::cout << "WARN: " << warn << std::endl;
	if (!err.empty())
		std::cout << "ERR: " << err << std::endl;

	if (res)
	{
		// Verify the primitives and count how many vertices there needs to be space for
		bool isValid = true;
		size_t vertexCount = 0;
		for (auto& mesh : model.meshes)
		{
			//std::cout << mesh.name << "\n";
			for (auto& primitive : mesh.primitives)
			{
				isValid &= VerifyPrimitive(model, mesh, primitive);
				if (isValid) // POSITION is definitely there if isValid is true and if not then the count doesn't matter anyway
					vertexCount += model.accessors[primitive.attributes["POSITION"]].count;
			}
		}

		// If the data is valid then copy the important stuff into the triangle registry
		if (isValid)
		{
			registry.Allocate(vertexCount);
			registry.VertexCount = vertexCount;

			// Copy all of the vertex positions
			{
				registry.Positions = reinterpret_cast<glm::vec3*>(registry.Buffer);

				size_t bytesCopied = 0;
				for (auto& mesh : model.meshes)
				{
					for (auto& primitive : mesh.primitives)
					{
						auto& accessor = model.accessors[primitive.attributes["POSITION"]];
						auto& bufferView = model.bufferViews[accessor.bufferView];
						auto& buffer = model.buffers[bufferView.buffer];

						uint8_t* destination = reinterpret_cast<uint8_t*>(registry.Positions) + bytesCopied;
						std::memcpy(destination, buffer.data.data() + bufferView.byteOffset, bufferView.byteLength);
						bytesCopied += bufferView.byteLength;
					}
				}

				registry.Normals = reinterpret_cast<glm::vec3*>(reinterpret_cast<uint8_t*>(registry.Positions) + bytesCopied);
			}

			// Copy all of the vertex normals
			{
				size_t bytesCopied = 0;
				for (auto& mesh : model.meshes)
				{
					for (auto& primitive : mesh.primitives)
					{
						auto& accessor = model.accessors[primitive.attributes["NORMAL"]];
						auto& bufferView = model.bufferViews[accessor.bufferView];
						auto& buffer = model.buffers[bufferView.buffer];

						uint8_t* destination = reinterpret_cast<uint8_t*>(registry.Normals) + bytesCopied;
						std::memcpy(destination, buffer.data.data() + bufferView.byteOffset, bufferView.byteLength);
						bytesCopied += bufferView.byteLength;
					}
				}

				registry.Colors = reinterpret_cast<glm::vec4*>(reinterpret_cast<uint8_t*>(registry.Normals) + bytesCopied);
			}

			// Copy all of the vertex colors
			{
				size_t colorsCopied = 0;
				for (auto& mesh : model.meshes)
				{
					for (auto& primitive : mesh.primitives)
					{
						auto& accessor = model.accessors[primitive.attributes["COLOR_0"]];
						auto& bufferView = model.bufferViews[accessor.bufferView];
						auto& buffer = model.buffers[bufferView.buffer];

						glm::vec4* destination = registry.Colors + colorsCopied;
						uint16_t* source = reinterpret_cast<uint16_t*>(buffer.data.data() + bufferView.byteOffset);
						
						// All of the colors get converted to floats while copying them
						for (int32_t i = 0; i < accessor.count; i++)
						{
							int32_t index = i * 4;
							destination[i] = {
								source[index] / 65535.0f,
								source[index + 1] / 65535.0f,
								source[index + 2] / 65535.0f,
								source[index + 3] / 65535.0f
							};
						}

						colorsCopied += accessor.count;
					}
				}
			}
			
			{
				uint32_t vertexOffset = 0; // Keep track of the vertex offset so that triangle relations are preserved
				for (auto& mesh : model.meshes)
				{
					for (auto& primitive : mesh.primitives)
					{
						auto& accessor = model.accessors[primitive.indices];
						auto& bufferView = model.bufferViews[accessor.bufferView];
						auto& buffer = model.buffers[bufferView.buffer];

						uint16_t* indices = reinterpret_cast<uint16_t*>(buffer.data.data() + bufferView.byteOffset);
						for (int32_t i = 0; i < accessor.count / 3; i++)
						{
							int32_t index = i * 3;
							registry.Triangles.emplace_back(
								indices[index] + vertexOffset,
								indices[index + 1] + vertexOffset,
								indices[index + 2] + vertexOffset
							);
						}

						vertexOffset += static_cast<uint32_t>(model.accessors[primitive.attributes["POSITION"]].count);
					}
				}
			}
		}
	}

	return registry;
}

int main()
{
	if (!glfwInit())
		return -1;

	GLFWwindow* window = glfwCreateWindow(1280, 720, "I am not putting hello world here again", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	if (!status)
	{
		glfwTerminate();
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	bool showDemoWindow = true;
	ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (showDemoWindow)
			ImGui::ShowDemoWindow(&showDemoWindow);

		ImGui::Render();
		int32_t width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	/*
	TriangleRegistry registry = LoadModel("amongus.glb");

	glm::dvec3 light(2.0, 4.0, -4.0);
	double lightStrength = 10.0;

	glm::dvec3 lookFrom(10.0, 2.0, 3.0);
	glm::dvec3 lookAt(0.0, 0.0, 1.0);
	glm::dvec3 vup(0.0, 1.0, 0.0);
	double focalDist = glm::length(lookFrom);
	double aperture = 0.1;

	int32_t samplesPerPixel = 10;

	Camera cam(lookFrom, lookAt, vup, 60.0, 16.0 / 9.0, aperture, focalDist);

	int32_t width = 1280, height = 720;
	PNGImage image(width, height);
	
	for (int32_t y = 0; y < height; y++)
	{
		for (int32_t x = 0; x < width; x++)
		{
			glm::vec3 pixelColor(0.0f);
			for (uint32_t s = 0; s < static_cast<uint32_t>(samplesPerPixel); s++) {
				double u = (static_cast<double>(x) + RandomDouble()) / static_cast<double>(width - 1);
				double v = (static_cast<double>(y) + RandomDouble()) / static_cast<double>(height - 1);
				Ray r = cam.GetRay(u, v);
				
				bool hasHit = false;
				double closestDistance = 0.0;
				IntersectionResult closestHit{};
				glm::uvec3 closestIndices{};

				for (glm::uvec3& indices : registry.Triangles)
				{
					IntersectionResult result = RayTriangleIntersection(
						r, 
						registry.Positions[indices.x],
						registry.Positions[indices.y],
						registry.Positions[indices.z]
					);

					if (result.IsHit)
					{
						glm::dvec3 diff = result.Position - cam.Origin;
						double distExp = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
						if (!hasHit || distExp < closestDistance)
						{
							closestDistance = distExp;
							closestHit = result;
							closestIndices = indices;
							hasHit = true;
						}
					}
				}

				if (hasHit)
				{
					glm::vec3 albedo = glm::vec3(
						registry.Colors[closestIndices.x] * static_cast<float>(closestHit.Barycentric.x) +
						registry.Colors[closestIndices.y] * static_cast<float>(closestHit.Barycentric.y) +
						registry.Colors[closestIndices.z] * static_cast<float>(closestHit.Barycentric.z)
					);

					glm::dvec3 normal = glm::normalize(
						registry.Normals[closestIndices.x] * static_cast<float>(closestHit.Barycentric.x) +
						registry.Normals[closestIndices.y] * static_cast<float>(closestHit.Barycentric.y) +
						registry.Normals[closestIndices.z] * static_cast<float>(closestHit.Barycentric.z)
					);

					float lightFactor = static_cast<float>(glm::dot(glm::normalize(light - closestHit.Position), normal)) / 2.0f + 0.5f;

					pixelColor += albedo * lightFactor;
				}
				else
					pixelColor += glm::vec3(0.0f);
				
			}

			image.SetPixel(x, y, pixelColor / static_cast<float>(samplesPerPixel));
		}
	}

	image.WriteImage("image.png");
	*/
}