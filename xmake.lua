set_project("MyVulkanProject")
set_languages("cxx20")
add_rules("mode.debug", "mode.release")

add_requires("vulkansdk")

add_requires("glfw")
add_requires("glm")
add_requires("tinyobjloader")
add_requires("stb")
add_requires("tinygltf")
add_requires("ktx")

-- Make Vulkan-Hpp structs aggregates so C++20 designated initializers work with MSVC
add_defines("VULKAN_HPP_NO_STRUCT_CONSTRUCTORS")

-- Note: we rely on the Vulkan SDK via add_requires("vulkansdk") and link it per-target below.

-- Enable Vulkan validation layers automatically in Debug builds
if is_mode("debug") then
    add_defines("ENABLE_VALIDATION_LAYERS")
end

target("VulkanTutorial")
    set_kind("binary")
    
    add_files("src/*.cpp")

    add_packages("glfw", "glm", "tinyobjloader", "stb", "tinygltf", "ktx", "vulkansdk")
