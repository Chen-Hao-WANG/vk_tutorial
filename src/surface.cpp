#include "tutorial.hpp"

void HelloTriangleApplication::createSurface() {
    VkSurfaceKHR raw_surface;
    if (!SDL_Vulkan_CreateSurface(window.get(), **instance, nullptr, &raw_surface)) {
        throw SDLException("Failed to create Vulkan surface");
    }
    surface = vk::raii::SurfaceKHR(instance, raw_surface);
}
