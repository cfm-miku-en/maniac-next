#include "window.h"
#include "config.h"
#include <maniac/maniac.h>
#include <unordered_map>
#include <cmath>

static void help_marker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void section_gap() {
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
}

static bool toggle_switch(const char* label, bool* v) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight();
    float width  = height * 1.75f;
    float radius = height * 0.50f;

    ImGui::InvisibleButton(label, ImVec2(width, height));
    bool changed = false;
    if (ImGui::IsItemClicked()) { *v = !*v; changed = true; }

    static std::unordered_map<std::string, float> anim_map;
    float& t = anim_map[label];
    float target = *v ? 1.0f : 0.0f;
    float dt = ImGui::GetIO().DeltaTime;
    t += (target - t) * (1.0f - expf(-12.0f * dt));
    if (fabsf(t - target) < 0.005f) t = target;

    ImVec4 acc = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
    ImVec4 off = ImVec4(0.31f, 0.31f, 0.31f, 0.78f);
    ImVec4 blended = ImVec4(
        off.x + (acc.x - off.x) * t,
        off.y + (acc.y - off.y) * t,
        off.z + (acc.z - off.z) * t,
        off.w + (acc.w - off.w) * t
    );

    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height),
        ImGui::ColorConvertFloat4ToU32(blended), height * 0.5f);
    dl->AddCircleFilled(
        ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius),
        radius - 1.5f, IM_COL32(255, 255, 255, 255));

    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (height - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::Text("%s", label);

    return changed;
}

static bool color_swatch(const char* id, float r, float g, float b, float* accent) {
    bool active = (fabsf(accent[0]-r) < 0.01f &&
                   fabsf(accent[1]-g) < 0.01f &&
                   fabsf(accent[2]-b) < 0.01f);

    if (active) ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(r, g, b, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r*0.8f, g*0.8f, b*0.8f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(r*0.6f, g*0.6f, b*0.6f, 1.f));

    bool clicked = ImGui::Button(id, ImVec2(22, 22));
    ImGui::PopStyleColor(3);
    if (active) ImGui::PopStyleVar();

    if (clicked) { accent[0] = r; accent[1] = g; accent[2] = b; }
    return clicked;
}

struct SliderAnim {
    float display = 0.0f;
    bool  initialized = false;
};

static bool animated_slider_int(const char* label, int* v, int v_min, int v_max,
                                 SliderAnim& anim, float speed = 8.0f) {
    if (!anim.initialized) {
        anim.display     = (float)*v;
        anim.initialized = true;
    }

    float target = (float)*v;
    float dt     = ImGui::GetIO().DeltaTime;
    anim.display += (target - anim.display) * (1.0f - expf(-speed * dt));
    if (fabsf(anim.display - target) < 0.5f) anim.display = target;

    float fraction  = (anim.display - (float)v_min) / (float)(v_max - v_min);
    fraction        = fraction < 0.0f ? 0.0f : fraction > 1.0f ? 1.0f : fraction;
    float bar_width = ImGui::CalcItemWidth();
    float bar_h     = ImGui::GetFrameHeight();
    ImVec2 bar_pos  = ImGui::GetCursorScreenPos();

    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImGuiStyle& st  = ImGui::GetStyle();
    float rounding  = st.FrameRounding;

    ImVec4 bg_col   = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 fill_col = ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab);
    fill_col.w      = 0.35f;

    dl->AddRectFilled(bar_pos,
        ImVec2(bar_pos.x + bar_width, bar_pos.y + bar_h),
        ImGui::ColorConvertFloat4ToU32(bg_col), rounding);

    float fill_w = bar_width * fraction;
    if (fill_w > 0.0f) {
        dl->AddRectFilled(bar_pos,
            ImVec2(bar_pos.x + fill_w, bar_pos.y + bar_h),
            ImGui::ColorConvertFloat4ToU32(fill_col), rounding);
    }

    std::string hidden_label = std::string("##asi_") + label;
    bool changed = ImGui::SliderInt(hidden_label.c_str(), v, v_min, v_max, "");

    char val_buf[32];
    snprintf(val_buf, sizeof(val_buf), "%d", *v);

    ImFont* font     = ImGui::GetFont();
    float font_size  = ImGui::GetFontSize();
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, val_buf);
    ImVec2 text_pos  = ImVec2(
        bar_pos.x + (bar_width - text_size.x) * 0.5f,
        bar_pos.y + (bar_h     - text_size.y) * 0.5f
    );

    float grab_size  = ImGui::GetStyle().GrabMinSize;
    float grab_x     = bar_pos.x + fraction * (bar_width - grab_size);
    float grab_left  = grab_x;
    float grab_right = grab_x + grab_size;

    dl->PushClipRect(ImVec2(grab_left, bar_pos.y), ImVec2(grab_right, bar_pos.y + bar_h), true);
    dl->AddText(font, font_size, text_pos, IM_COL32(0, 0, 0, 220), val_buf);
    dl->PopClipRect();

    dl->PushClipRect(bar_pos, ImVec2(grab_left, bar_pos.y + bar_h), true);
    dl->AddText(font, font_size, text_pos, IM_COL32(255, 255, 255, 200), val_buf);
    dl->PopClipRect();

    dl->PushClipRect(ImVec2(grab_right, bar_pos.y), ImVec2(bar_pos.x + bar_width, bar_pos.y + bar_h), true);
    dl->AddText(font, font_size, text_pos, IM_COL32(255, 255, 255, 200), val_buf);
    dl->PopClipRect();

    return changed;
}

static void set_priority_class(int priority) {
    const auto proc = GetCurrentProcess();
    const auto old  = GetPriorityClass(proc);
    SetPriorityClass(proc, priority);
    debug("changed priority class from 0x%lx to 0x%lx", old, GetPriorityClass(proc));
}

int main(int, char**) {
    std::string message;

    config::read_from_file(maniac::config);

    auto run = [&message](osu::Osu& osu) {
        maniac::osu = &osu;
        message = "waiting for beatmap...";
        maniac::block_until_playing();
        message = "found beatmap";

        std::vector<osu::HitObject> hit_objects;
        for (int i = 0; i < 10; i++) {
            try {
                hit_objects = osu.get_hit_objects();
                break;
            } catch (std::exception& err) {
                debug("get hit objects attempt %d failed: %s", i + 1, err.what());
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }

        if (hit_objects.empty())
            throw std::runtime_error("failed getting hit objects");

        set_priority_class(HIGH_PRIORITY_CLASS);
        maniac::randomize(hit_objects, maniac::config.randomization_mean, maniac::config.randomization_stddev);

        if (maniac::config.humanization_type == maniac::config::STATIC_HUMANIZATION)
            maniac::humanize_static(hit_objects, maniac::config.humanization_modifier);
        if (maniac::config.humanization_type == maniac::config::DYNAMIC_HUMANIZATION)
            maniac::humanize_dynamic(hit_objects, maniac::config.humanization_modifier);

        auto actions = maniac::to_actions(hit_objects, osu.get_game_time());
        message = "playing";
        maniac::play(actions);

        set_priority_class(NORMAL_PRIORITY_CLASS);
    };

    auto thread = std::jthread([&message, &run](const std::stop_token& token) {
        while (!token.stop_requested()) {
            try {
                auto osu = osu::Osu();
                while (!token.stop_requested()) run(osu);
            } catch (std::exception& err) {
                (message = err.what()).append(" (retrying in 2 seconds)");
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    });

    static SliderAnim anim_hmod, anim_mean, anim_stddev, anim_comp, anim_tap;

    window::start([&message] {
        ImGui::Begin("maniac-next", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

        bool og_theme = (maniac::config.theme_index == 2);

        ImGui::Dummy(ImVec2(0, 2));
        ImVec4 status_col = (message == "playing")
            ? ImVec4(0.4f, 1.0f, 0.5f, 1.0f)
            : ImVec4(0.8f, 0.8f, 0.8f, 0.7f);
        ImGui::PushStyleColor(ImGuiCol_Text, status_col);
        ImGui::Text("  %s", message.c_str());
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 2));

        section_gap();

        if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Dummy(ImVec2(0, 3));

            ImGui::Text("Theme");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(180);
            ImGui::Combo("##theme", &maniac::config.theme_index, "Cherry\0Moonlight\0OG\0\0");

            if (!og_theme) {
                ImGui::Dummy(ImVec2(0, 4));
                ImGui::Text("Accent");
                ImGui::SameLine();
                color_swatch("##pink",   1.0f,  0.40f, 0.70f, maniac::config.accent_color); ImGui::SameLine();
                color_swatch("##blue",   0.40f, 0.60f, 1.0f,  maniac::config.accent_color); ImGui::SameLine();
                color_swatch("##green",  0.30f, 0.90f, 0.55f, maniac::config.accent_color); ImGui::SameLine();
                color_swatch("##purple", 0.70f, 0.40f, 1.0f,  maniac::config.accent_color); ImGui::SameLine();
                color_swatch("##orange", 1.0f,  0.55f, 0.20f, maniac::config.accent_color); ImGui::SameLine();
                ImGui::ColorEdit3("##accent_custom", maniac::config.accent_color,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                ImGui::SameLine();
                ImGui::TextDisabled("custom");
            }

            ImGui::Dummy(ImVec2(0, 3));
        }

        section_gap();

        if (ImGui::CollapsingHeader("Humanization", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Dummy(ImVec2(0, 3));

            if (og_theme) {
                ImGui::Combo("Humanization Type##og_htype", &maniac::config.humanization_type, "Static\0Dynamic (new)\0\0");
                ImGui::SameLine();
                help_marker("Static: density per 1s chunk. Dynamic: density 1s ahead of each hit, applied individually.");
                ImGui::InputInt("Humanization##og_hmod", &maniac::config.humanization_modifier, 1, 10);
                ImGui::SameLine();
                help_marker("Density-based hit-time offset.");
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::Text("Adds a random hit-time offset generated using a normal\ndistribution with given mean and standard deviation.");
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::InputInt("Randomization Mean##og_rmean", &maniac::config.randomization_mean);
                ImGui::InputInt("Randomization Stddev##og_rstddev", &maniac::config.randomization_stddev);
            } else {
                ImGui::SetNextItemWidth(180);
                ImGui::Combo("Type##htype", &maniac::config.humanization_type, "Static\0Dynamic\0\0");
                ImGui::SameLine();
                help_marker("Static: density per 1s chunk. Dynamic: density 1s ahead of each hit, applied individually.");

                ImGui::Dummy(ImVec2(0, 2));

                ImGui::SetNextItemWidth(180);
                animated_slider_int("Modifier##hmod", &maniac::config.humanization_modifier, 0, 500, anim_hmod);
                ImGui::SameLine();
                help_marker("Density-based hit-time offset. Higher = more human variation.");

                ImGui::Dummy(ImVec2(0, 4));
                ImGui::TextDisabled("Gaussian randomization");
                ImGui::Dummy(ImVec2(0, 2));

                ImGui::SetNextItemWidth(180);
                animated_slider_int("Mean##rmean", &maniac::config.randomization_mean, -50, 50, anim_mean);
                ImGui::SameLine();
                help_marker("Mean of the normal distribution used for random hit-time offset.");

                ImGui::SetNextItemWidth(180);
                animated_slider_int("Std Dev##rstddev", &maniac::config.randomization_stddev, 0, 100, anim_stddev);
                ImGui::SameLine();
                help_marker("Standard deviation. Higher = more spread.");
            }

            ImGui::Dummy(ImVec2(0, 3));
        }

        section_gap();

        if (ImGui::CollapsingHeader("Gameplay", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Dummy(ImVec2(0, 3));

            if (og_theme) {
                ImGui::InputInt("Compensation##og_comp", &maniac::config.compensation_offset);
                ImGui::SameLine();
                help_marker("Adds constant value to all hit-times to compensate for input latency.");
                ImGui::Checkbox("Mirror Mod##og_mirror", &maniac::config.mirror_mod);
                ImGui::InputInt("Tap time##og_tap", &maniac::config.tap_time);
                ImGui::SameLine();
                help_marker("How long a key is held down for a single keypress, in milliseconds.");
            } else {
                ImGui::SetNextItemWidth(180);
                animated_slider_int("Compensation (ms)##comp", &maniac::config.compensation_offset, -100, 100, anim_comp);
                ImGui::SameLine();
                help_marker("Constant offset added to all hit-times. Compensates for input latency.");

                ImGui::Dummy(ImVec2(0, 2));

                ImGui::SetNextItemWidth(180);
                animated_slider_int("Tap Time (ms)##tap", &maniac::config.tap_time, 1, 100, anim_tap);
                ImGui::SameLine();
                help_marker("How long a key is held down per keypress.");

                ImGui::Dummy(ImVec2(0, 4));
                toggle_switch("Mirror Mod", &maniac::config.mirror_mod);
            }

            ImGui::Dummy(ImVec2(0, 3));
        }

        section_gap();

        ImGui::Dummy(ImVec2(0, 2));
        ImGui::TextDisabled("maniac-next by miku (original by fs-c)");

        ImGui::End();
    });

    config::write_to_file(maniac::config);
    thread.request_stop();
    return EXIT_SUCCESS;
}
