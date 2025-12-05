-- xmake f -m release; xmake; xmake run
-- xmake f -m debug; xmake; xmake run
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

rule("slangc")
    -- run shader compilation as a pre-build hook instead of overriding C/C++ build
    before_build(function (target)
        import("core.project.depend")
        local projdir = os.projectdir()
        -- compile all .slang files under project/shaders
        local slangdir = path.join(projdir, "shaders")
        if not os.isdir(slangdir) then
            return
        end
        local outdir = path.join(target:targetdir(), "shaders")
        os.mkdir(outdir)

        -- resolve slangc (try VULKAN_SDK/Bin first, fallback to PATH)
        local slangc = "slangc"
        local vk_sdk = os.getenv("VULKAN_SDK")
        if vk_sdk then
            local tryexe = path.join(vk_sdk, "Bin", "slangc.exe")
            if not os.isfile(tryexe) then
                tryexe = path.join(vk_sdk, "bin", "slangc.exe")
            end
            if os.isfile(tryexe) then
                slangc = tryexe
            end
        end

        -- compile each .slang -> .spv (incremental via depend.on_changed)
        for _, f in ipairs(os.files(path.join(slangdir, "*.slang"))) do
            local basename = path.basename(f)
            local out = path.join(outdir, basename .. ".spv")
            local dep = target:dependfile(out)
            depend.on_changed(function ()
                -- slangc: input file directly, no -f
                os.vrunv(slangc, {f, "-o", out})
            end, {files = {f}, dependfile = dep})
        end
    end)

target("VulkanTutorial")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("glfw", "glm", "tinyobjloader", "stb", "tinygltf", "ktx", "vulkansdk")
    -- attach the rule so slang files compile before building the C++ target
    add_rules("slangc")
    -- Enable Vulkan validation layers automatically in Debug builds
    if is_mode("debug") then
        add_defines("ENABLE_VALIDATION_LAYERS")
    end
    -- run from target directory so runtime finds compiled shaders under ./shaders
    set_rundir("$(builddir)/$(plat)/$(arch)/$(mode)")
