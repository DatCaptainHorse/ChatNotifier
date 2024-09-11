if (NOT NON_NIX_BUILD)
  message(WARNING "Fetching dependencies without `NON_NIX_BUILD` set.")
endif ()

# Fetch libogg
message(STATUS "Fetching libogg")
xrepo_package("libogg")

# Fetch libopus
message(STATUS "Fetching libopus")
xrepo_package("libopus")

# Fetch libvorbis
message(STATUS "Fetching libvorbis")
xrepo_package("libvorbis")

# Fetch libflac
message(STATUS "Fetching libflac")
xrepo_package("libflac")

# Fetch libsndfile
message(STATUS "Fetching libsndfile")
xrepo_package("libsndfile")

# Fetch openal-soft
message(STATUS "Fetching openal-soft")
xrepo_package("openal-soft")

# Fetch imgui
message(STATUS "Fetching imgui")
xrepo_package("imgui")

# Fetch glfw
message(STATUS "Fetching GLFW")
xrepo_package("glfw")

# Fetch libhv
message(STATUS "Fetching libhv")
xrepo_package("libhv")

# Fetch python
message(STATUS "Fetching Python")
xrepo_package("python 3.12.x")

# Fetch pybind11
message(STATUS "Fetching pybind11")
xrepo_package("pybind11")
