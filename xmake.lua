add_rules("mode.release", "mode.debug")
set_languages("c++23")

add_requires("libogg", "libvorbis", "libopus", "libflac", "libsndfile", "openal-soft",
             "imgui", "glfw", "libhv", "python 3.12.x", "pybind11", "node-api-headers")

includes("napi.lua")
add_requires("napi 8.1.0", { configs = { napi_version = 7 } })

target("chatnotifier")
    set_kind("shared")
    add_files("Source/libchatnotifier/glad/*.c")
    add_files("Source/libchatnotifier/imgui/*.cpp")
    add_files("Source/libchatnotifier/*.cppm")
    add_files("Source/libchatnotifier/napi.cppm", {public = true})
    add_files("Source/libchatnotifier/node/*.cc", {public = true})
    add_includedirs("Source/libchatnotifier")
    add_packages("libogg", "libvorbis", "libopus", "libflac", "libsndfile", "openal-soft",
                 "imgui", "glfw", "libhv", "python", "pybind11", "node-api-headers", "napi")
    add_defines("TWITCH_CLIENT_SECRET=\"$(env TWITCH_CLIENT_SECRET)\"")
    add_linkdirs("External/node/lib")
    add_links("node")
    if is_plat("windows") then
        add_links("delayimp")
        add_shflags("/DEF:$(projectdir)/External/node/def/node_api.def /DELAYLOAD:NODE.EXE /SAFESEH:NO")
    end

    after_build(function(target)
        os.cp("$(buildir)/$(host)/$(arch)/$(mode)/chatnotifier.dll", "$(buildir)/$(host)/$(arch)/$(mode)/chatnotifier.node")
    end)
