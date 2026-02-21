#pragma once

#include <maniac/common.h>

#include <utility>
#include <fstream>

namespace maniac {
    struct config {
        static constexpr int VERSION = 2;

        static constexpr auto STATIC_HUMANIZATION = 0;
        static constexpr auto DYNAMIC_HUMANIZATION = 1;

        int tap_time = 20;
        bool mirror_mod = false;
        int compensation_offset = -5;
        int humanization_modifier = 0;
        int randomization_mean = 0;
        int randomization_stddev = 0;
        int humanization_type = DYNAMIC_HUMANIZATION;

        std::string keys = "asdfjkl;";

        int theme_index = 1;
        float accent_color[3] = {0.40f, 0.60f, 1.0f};

        bool dark_mode = true;
        float bg_color[3] = {0.1f, 0.1f, 0.1f};
    };
}
