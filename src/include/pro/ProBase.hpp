#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstddef>
#include <functional>
#include <thread>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <ranges>
#include <array>
#include <bit>
#include <type_traits>

using namespace std;

#define VULKAN_HPP_USE_REFLECT
#define VULKAN_HPP_NO_NODISCARD_WARNINGS
//#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan_raii.hpp>
//#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>

#define GLM_FORCE_CTOR_INIT
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "vk_mem_alloc_raii.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

namespace pro {
    ///////////////////////////////////////////////////////////////////////////
    // Constants
    ///////////////////////////////////////////////////////////////////////////

    constexpr int DEFAULT_MAX_LINE_WIDTH = 80;

    ///////////////////////////////////////////////////////////////////////////
    // Helper functions
    ///////////////////////////////////////////////////////////////////////////

    inline string get_full_error_string(string origin, string errorType, string msg) {
        return "[" + origin + "][" + errorType + "] " + msg;   
    };

    inline void print_error(string origin, string msg) {
        string full_error_msg = get_full_error_string(origin, "ERROR", msg);
        cerr << full_error_msg << endl;        
    };

    inline void print_warning(string origin, string msg) {
        string full_error_msg = get_full_error_string(origin, "WARNING", msg);
        cerr << full_error_msg << endl;        
    };

    inline void print_failure(string origin, string msg) {
        string full_error_msg = get_full_error_string(origin, "FAILURE", msg);
        cerr << full_error_msg << endl;        
    };

    inline void print_and_throw_error(string origin, string msg) {
        string full_error_msg = get_full_error_string(origin, "ERROR", msg);
        cerr << full_error_msg << endl;
        throw runtime_error(full_error_msg);
    };

    inline vector<const char*> to_const_char_list(vector<string> &names) {
        vector<const char*> name_chars {};
        name_chars.reserve(names.size());
        for(int i = 0; i < names.size(); i++) {
            name_chars.push_back(names[i].c_str());
        }
        return name_chars;
    };

    inline void print_string_list(string header, vector<string> &list) {
        cout << header + ":" << endl;
        for(auto e : list) {
            cout << "* " << e << endl;
        }
    };

    inline void print_header_line(  char boundary = '*', 
                                    string text = "", 
                                    int maxCharCnt = DEFAULT_MAX_LINE_WIDTH,
                                    std::ostream& os = std::cout) {        
        os << boundary << boundary;
        int extraCharCnt = 2;
        
        if(text.size() > 0) {
            os << " " << text.c_str() << " ";
            extraCharCnt += 2;
        }

        int leftOver = maxCharCnt - extraCharCnt - text.size();
        for(int i = 0; i < leftOver; i++) {
            os << boundary;
        }
        os << endl;        
    };

    inline string to_upper(string text) {
        transform(
            text.begin(),
            text.end(),
            text.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });

        return text;
    };
}
