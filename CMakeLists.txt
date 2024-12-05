# CMakeList.txt: VulkanLearning 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)
# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(BUILD_TYPE "Release")
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(BUILD_TYPE "Debug")
endif()

if(WIN32)
    set(PLATFORM "windows")
elseif(APPLE)
    set(PLATFORM "macos")
elseif(UNIX)
    set(PLATFORM "linux")
endif()

set(PROJECT_SOURCES 
include/glfwgeneral.h 
include/globalinclude.h 
include/stb_image.h
include/vkbase.h
src/glfwgeneral.cpp
src/vkbase.cpp
include/helper.h 
include/synchronization.h 
include/command.h 
"include/rpwf.h" "include/shader.h" "include/pipeline.h" "include/deviceMemory.h" "include/vkbaseplus.h" "include/image.h")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin/${BUILD_TYPE})

project ("VulkanLearning")

include_directories(${CMAKE_CURRENT_LIST_DIR}/include
${CMAKE_CURRENT_LIST_DIR}/../glm/glm
${CMAKE_CURRENT_LIST_DIR}/../glfw/include
${CMKAE_CURRENT_LIST_DIR/../singleton/include}
)

include(FetchContent)
set(GLM_DIR "${CMAKE_CURRENT_LIST_DIR}/../glm")
set(GLM_URL "https://github.com/g-truc/glm")
set(GLFW_DIR "${CMAKE_CURRENT_LIST_DIR}/../glfw")
set(GLFW_URL "https://github.com/glfw/glfw")
set(SINGLETON_DIR "${CMAKE_CURRENT_LIST_DIR}/../singleton")
set(SINGLETON_URL "https://github.com/jimmy-park/singleton")

FetchContent_Declare(
        glm
        GIT_REPOSITORY ${GLM_URL}
        GIT_TAG "master"
        SOURCE_DIR ${GLM_DIR}
  )

FetchContent_Declare(
        glfw
        GIT_REPOSITORY ${GLFW_URL}
        GIT_TAG "master"
        SOURCE_DIR ${GLFW_DIR}
)

set(SINGLETON_INJECT_ABSTRACT_CLASS ON)
set(SINGLETON_INSTALL ON)

FetchContent_Declare(
    singleton
    GIT_REPOSITORY ${SINGLETON_URL}
    GIT_TAG "main"
    SOURCE_DIR ${SINGLETON_DIR}
)

FetchContent_MakeAvailable(glm glfw singleton) 

find_package(Vulkan REQUIRED)

# 将源代码添加到此项目的可执行文件。
add_executable (${CMAKE_PROJECT_NAME} ${PROJECT_SOURCES} main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glm)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE singleton::singleton)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/shader/Triangle.frag.spv DESTINATION 
 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader)
file(COPY ${CMAKE_CURRENT_LIST_DIR}/shader/Triangle.vert.spv DESTINATION 
 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader)
 file(COPY ${CMAKE_CURRENT_LIST_DIR}/shader/Vertex.frag.spv DESTINATION 
 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader)
 file(COPY ${CMAKE_CURRENT_LIST_DIR}/shader/Vertex.vert.spv DESTINATION 
 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader)

# TODO: 如有需要，请添加测试并安装目标。
