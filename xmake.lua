set_project("MyVulkanProject")
set_languages("cxx20")
add_rules("mode.debug", "mode.release")


add_requires("glfw")
add_requires("glm")
add_requires("tinyobjloader")
add_requires("stb")
add_requires("tinygltf")
add_requires("ktx")

add_packages("vulkan")

target("VulkanTutorial")
    set_kind("binary")
    
    add_files("src/*.cpp")

    add_packages("glfw", "glm", "tinyobjloader", "stb", "tinygltf", "ktx")
