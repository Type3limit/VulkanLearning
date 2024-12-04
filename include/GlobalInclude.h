#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include <unordered_map>
#include <span>
#include <memory>
#include <functional>
#include <concepts>
#include <format>
#include <chrono>
#include <numeric>
#include <numbers>

//GLM
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//����������������ϵ���ڴ˶���GLM_FORCE_LEFT_HANDED
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

//stb_image.h
#include <stb_image.h>

//Vulkan
#ifdef _WIN32                        //����ƽ̨��Windows������������н������ƽ̨�ϵĲ��죩
#define VK_USE_PLATFORM_WIN32_KHR    //�ڰ���vulkan.hǰ����ú꣬��һ������vulkan_win32.h��windows.h
#define NOMINMAX                     //����ú�ɱ���windows.h�е�min��max���������׼���еĺ�������ͻ
#pragma comment(lib, "vulkan-1.lib") //���ӱ�������ľ�̬�����
#endif
#include "helper.h"