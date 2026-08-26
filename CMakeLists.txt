cmake_minimum_required(VERSION 3.24) 
project(CS_450_535_Fall_2026)

set(CMAKE_INSTALL_PREFIX .)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#####################################
# Require C++23
#####################################

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#####################################
# Find necessary libraries
#####################################

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

#####################################
# GLFW
#####################################

find_package(glfw3 3.4.0 QUIET)

if(NOT glfw3_FOUND)
    message("GLFW not installed...downloading from source...")

    FetchContent_Declare(
        glfw3
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG 3.4
        GIT_SHALLOW 1
    )

    set("BUILD_SHARED_LIBS" OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw3)
    set(glfw3_VERSION "3.4.0")
else()
    message("GLFW already installed on system...")
endif()

message("GLFW version: ${glfw3_VERSION}")

#####################################
# GLM
#####################################

find_package(glm 1.0.3 QUIET)

if(NOT glm_FOUND)
    message("GLM not installed...downloading from source...")

    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 1.0.3  
        GIT_SHALLOW 1
    )    
    FetchContent_MakeAvailable(glm)
    set(glm_VERSION "1.0.3")
else()
    message("GLM already installed on system...")
endif()

message("GLM version: ${glm_VERSION}")

#####################################
# Assimp
#####################################

find_package(assimp 6.0.2 QUIET)

if(NOT assimp_FOUND)
    message("assimp not installed...downloading from source...")
    set(CMAKE_POLICY_DEFAULT_CMP0175 OLD)

    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp
        GIT_TAG v6.0.2
        GIT_SHALLOW 1
    )

    set("ASSIMP_INSTALL_PDB" OFF CACHE BOOL "" FORCE)
    set("BUILD_SHARED_LIBS" OFF CACHE BOOL "" FORCE)
    set("ASSIMP_BUILD_TESTS" OFF CACHE BOOL "" FORCE)
    set("ASSIMP_BUILD_SAMPLES" OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(assimp)
    set(assimp_VERSION "6.0.2")
else()
    message("assimp already installed on system...")
endif()

message("assimp version: ${assimp_VERSION}")

#####################################
# Vulkan and Threads
#####################################

find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

#####################################
# VMA-HPP
#####################################

find_package(VulkanMemoryAllocator-Hpp QUIET) # CONFIG REQUIRED)

if(NOT VulkanMemoryAllocator-Hpp_FOUND)
    message("VulkanMemoryAllocator-Hpp not installed...downloading from source...")

    FetchContent_Declare(
        vmahpp
        URL      https://github.com/YaaZ/VulkanMemoryAllocator-Hpp/releases/download/v3.3.0%2B3/VulkanMemoryAllocator-Hpp-3.3.0.tar.gz
        URL_HASH SHA256=8c2a0573babe1f86f3241d12b4c9df40bb944649b7f4239c1c611797f8af6cd5 # Copy from the asset list on the release page
    )
    FetchContent_MakeAvailable(vmahpp)
else()
    message("VulkanMemoryAllocator-Hpp already installed on system...")
endif()

#####################################
# stb
#####################################

find_package(stb QUIET)

if(NOT stb_FOUND)
    message("stb not installed...downloading from source...")

    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb
        GIT_TAG master
        GIT_SHALLOW 1
    )

    FetchContent_MakeAvailable(stb)    

    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

else()
    message("stb already installed on system...")
endif()

#####################################
# Get general sources
#####################################

include_directories("src/include")

file(GLOB GENERAL_SOURCES    
    "src/lib/*.cpp"    
)

#####################################
# SPIR-V COMPILATION
#####################################

# Adapted from: https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396
macro(COMPILE_VULKAN_SHADERS target)
    # Reset list of binary files
    set(SPIRV_BINARY_FILES)
    
    # Graphics pipelines use vertex/fragment shaders. Ray-tracing pipelines
    # additionally use ray-generation, miss, and closest-hit shader stages.
    file(GLOB SHADER_SOURCES
        "vulkanshaders/${target}/*.vert"
        "vulkanshaders/${target}/*.frag"
        "vulkanshaders/${target}/*.rgen"
        "vulkanshaders/${target}/*.rmiss"
        "vulkanshaders/${target}/*.rchit"
        "vulkanshaders/${target}/*.rint"
    )

    foreach(GLSL ${SHADER_SOURCES})
        #message(${GLSL})
        cmake_path(GET GLSL FILENAME filename)        
        #message(${filename}) 
        set(SPIRV "${PROJECT_BINARY_DIR}/compiledshaders/${target}/${filename}.spv")
        #message(${SPIRV})
        #message("${PROJECT_BINARY_DIR}/compiledshaders/${target}/")
        #message(Vulkan::glslc)
        	  
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND cd
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/compiledshaders/${target}/"
            # GL_EXT_ray_tracing requires a modern SPIR-V environment.
            # Vulkan 1.3 matches the minimum API requested by the raytracer.
            COMMAND Vulkan::glslc --target-env=vulkan1.3 ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach(GLSL)

    add_custom_target(
	    Shaders_${target}
	    DEPENDS ${SPIRV_BINARY_FILES}
	)

	add_dependencies(${target} Shaders_${target})

endmacro()

#####################################
# Create executables 
# and install targets
# (also setup testing)
#####################################

set(ALL_LIBRARIES 
    Vulkan::Vulkan    
    glfw 
    glm::glm  
    assimp::assimp
    stb    
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp   
    Threads::Threads)

add_library(PrometheusThirdPartyImpl STATIC 
    "src/lib/pro/stb_impl.cpp"
    "src/lib/pro/vma_impl.cpp")
target_link_libraries(PrometheusThirdPartyImpl PRIVATE ${ALL_LIBRARIES})

add_library(PrometheusBase STATIC "src/lib/pro/ProBase.cpp")
target_precompile_headers(PrometheusBase PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/include/pro/ProBase.hpp")
target_link_libraries(PrometheusBase PRIVATE ${ALL_LIBRARIES})

macro(CREATE_VULKAN_EXECUTABLE target)
    add_executable(${target} ${GENERAL_SOURCES} "./src/app/${target}.cpp")
    target_precompile_headers(${target} REUSE_FROM PrometheusBase)
    target_link_libraries(${target} PRIVATE PrometheusThirdPartyImpl ${ALL_LIBRARIES})    
    install(TARGETS ${target} RUNTIME DESTINATION bin/${target})
    COMPILE_VULKAN_SHADERS(${target})
    install(DIRECTORY ${PROJECT_BINARY_DIR}/compiledshaders/${target} DESTINATION bin/${target}/build/compiledshaders)
endmacro()

set(TEST_FOLDER "tests")
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_FOLDER}")
    message(STATUS "Tests folder found")
    include(CTest)
    enable_testing()
    add_subdirectory(${TEST_FOLDER})
endif()

CREATE_VULKAN_EXECUTABLE(BasicSquare)
CREATE_VULKAN_EXECUTABLE(SoftwareRenderer)
