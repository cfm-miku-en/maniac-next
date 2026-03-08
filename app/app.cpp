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
    bool editing  = ImGui::IsItemActive() && ImGui::GetIO().KeyCtrl;

    if (!editing) {
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
        float grab_left  = bar_pos.x + fraction * (bar_width - grab_size);
        float grab_right = grab_left + grab_size;

        dl->PushClipRect(ImVec2(grab_left, bar_pos.y), ImVec2(grab_right, bar_pos.y + bar_h), true);
        dl->AddText(font, font_size, text_pos, IM_COL32(0, 0, 0, 220), val_buf);
        dl->PopClipRect();

        dl->PushClipRect(bar_pos, ImVec2(grab_left, bar_pos.y + bar_h), true);
        dl->AddText(font, font_size, text_pos, IM_COL32(255, 255, 255, 200), val_buf);
        dl->PopClipRect();

        dl->PushClipRect(ImVec2(grab_right, bar_pos.y), ImVec2(bar_pos.x + bar_width, bar_pos.y + bar_h), true);
        dl->AddText(font, font_size, text_pos, IM_COL32(255, 255, 255, 200), val_buf);
        dl->PopClipRect();
    }

    const char* hash = strstr(label, "##");
    const char* display_label = hash ? label : label;
    char label_buf[128];
    if (hash) {
        size_t len = (size_t)(hash - label);
        strncpy(label_buf, label, len);
        label_buf[len] = '\0';
        display_label = label_buf;
    }
    ImGui::SameLine();
    ImGui::Text("%s", display_label);

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

        while (osu.is_playing())
            std::this_thread::sleep_for(std::chrono::milliseconds(250));

        maniac::block_until_playing();
        message = "found beatmap";

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

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
        if (maniac::config.humanization_type == maniac::config::UR_HUMANIZATION)
            maniac::humanize_ur(hit_objects, maniac::config.ur_target, maniac::config.ur_target_max);
        if (maniac::config.humanization_type == maniac::config::UR_STATIC_HUMANIZATION)
            maniac::humanize_ur_static(hit_objects, maniac::config.ur_target);

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

    static SliderAnim anim_hmod, anim_mean, anim_stddev, anim_comp, anim_tap, anim_ur;

    static bool show_settings  = false;
    static bool show_tutorial  = false;

    static bool tutorial_init = false;
    if (!tutorial_init) {
        tutorial_init = true;
        if (maniac::config.show_tutorial) {
            show_tutorial = true;
            maniac::config.show_tutorial = false;
            config::write_to_file(maniac::config);
        }
    }

    window::start([&message] {
        ImGui::Begin("maniac-next", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

        bool og_theme = (maniac::config.theme_index == 2);

        if (show_tutorial) {
            ImGui::OpenPopup("##tutorial_popup");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Always);
        if (ImGui::BeginPopupModal("##tutorial_popup", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
            float cw = ImGui::GetContentRegionAvail().x;
            ImVec2 title_sz = ImGui::CalcTextSize("Welcome to maniac-next!");
            ImGui::SetCursorPosX((cw - title_sz.x) * 0.5f + ImGui::GetStyle().WindowPadding.x);
            ImGui::Text("Welcome to maniac-next!");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 6));

            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 380);
            ImGui::TextDisabled("Here is how to get started:");
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::BulletText("Open osu!stable.");
            ImGui::BulletText("Set it to windowed with a resolution\n lower than your monitor.");
            ImGui::BulletText("Configure the settings of maniac.");
            ImGui::BulletText("Play a map!");
            ImGui::Dummy(ImVec2(0, 6));
            ImGui::TextDisabled("To see this tutorial again, go to the\nsettings (top right) and click \"Tutorial\".");
            ImGui::PopTextWrapPos();

            ImGui::Dummy(ImVec2(0, 8));
            float btn_w = 120;
            ImGui::SetCursorPosX((cw - btn_w) * 0.5f + ImGui::GetStyle().WindowPadding.x);
            if (ImGui::Button("Got it!", ImVec2(btn_w, 0))) {
                show_tutorial = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::EndPopup();
        }

        if (show_settings) {
            ImGui::OpenPopup("##settings_popup");
            show_settings = false;
        }

        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_Always);
        if (ImGui::BeginPopupModal("##settings_popup", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
            float cw2 = ImGui::GetContentRegionAvail().x;
            ImVec2 stitle = ImGui::CalcTextSize("Settings");
            ImGui::SetCursorPosX((cw2 - stitle.x) * 0.5f + ImGui::GetStyle().WindowPadding.x);
            ImGui::Text("Settings");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 6));

            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 6));

            ImGui::TextDisabled("Appearance");
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::Text("Theme");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(160);
            ImGui::Combo("##s_theme", &maniac::config.theme_index, "Cherry\0Moonlight\0OG\0\0");

            if (maniac::config.theme_index != 2) {
                ImGui::Dummy(ImVec2(0, 4));
                ImGui::Text("Accent");
                ImGui::SameLine();
                color_swatch("##s_pink",   1.0f,  0.40f, 0.70f, maniac::config.accent_color); ImGui::SameLine();
                color_swatch("##s_blue",   0.40f, 0.60f, 1.0f,  maniac::config.accent_color); ImGui::SameLine();
                color_swatch("##s_green",  0.30f, 0.90f, 0.55f, maniac::config.accent_color); ImGui::SameLine();
                color_swatch("##s_purple", 0.70f, 0.40f, 1.0f,  maniac::config.accent_color); ImGui::SameLine();
                color_swatch("##s_orange", 1.0f,  0.55f, 0.20f, maniac::config.accent_color); ImGui::SameLine();
                ImGui::ColorEdit3("##s_accent_custom", maniac::config.accent_color,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                ImGui::SameLine();
                ImGui::TextDisabled("custom");
            }

            ImGui::Dummy(ImVec2(0, 8));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 6));

            ImGui::TextDisabled("Help");
            ImGui::Dummy(ImVec2(0, 4));
            if (ImGui::Button("Tutorial", ImVec2(100, 0))) {
                show_tutorial = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("Show the getting started guide.");

            ImGui::Dummy(ImVec2(0, 10));

            float close_w = 100;
            ImGui::SetCursorPosX((cw2 - close_w) * 0.5f + ImGui::GetStyle().WindowPadding.x);
            if (ImGui::Button("Close", ImVec2(close_w, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::EndPopup();
        }

        ImGui::Dummy(ImVec2(0, 2));
        ImVec4 status_col = (message == "playing")
            ? ImVec4(0.4f, 1.0f, 0.5f, 1.0f)
            : ImVec4(0.8f, 0.8f, 0.8f, 0.7f);
        ImGui::PushStyleColor(ImGuiCol_Text, status_col);
        ImGui::Text("  %s", message.c_str());
        ImGui::PopStyleColor();

        float avail = ImGui::GetContentRegionAvail().x;
        float btn_sz = ImGui::GetFrameHeight();
        ImGui::SameLine(avail - btn_sz + ImGui::GetStyle().WindowPadding.x);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeightWithSpacing() + 2);
        if (ImGui::Button("##gear", ImVec2(btn_sz, btn_sz))) {
            show_settings = true;
        }
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 bp      = ImGui::GetItemRectMin();
            ImVec2 sz      = ImGui::GetItemRectSize();
            ImVec2 tc      = ImVec2(bp.x + sz.x * 0.5f, bp.y + sz.y * 0.5f);
            float r        = sz.x * 0.28f;
            float ri       = r * 0.55f;
            int teeth      = 6;
            float tooth_h  = r * 0.35f;
            ImU32 col      = IM_COL32(210, 210, 210, 230);
            for (int i = 0; i < teeth; i++) {
                float a0 = ((float)i / teeth) * 6.2832f;
                float a1 = a0 + 0.38f;
                float a2 = a1 + 0.25f;
                float a3 = a2 + 0.38f;
                auto pt = [&](float ang, float rad) {
                    return ImVec2(tc.x + cosf(ang) * rad, tc.y + sinf(ang) * rad);
                };
                dl->AddQuadFilled(pt(a0, r), pt(a1, r + tooth_h), pt(a2, r + tooth_h), pt(a3, r), col);
            }
            dl->AddCircleFilled(tc, r,        col,      32);
            dl->AddCircleFilled(tc, ri, IM_COL32(0,0,0,0), 32);
            ImVec4 bg4 = ImGui::GetStyleColorVec4(ImGuiCol_Button);
            ImU32 bg_col2 = ImGui::ColorConvertFloat4ToU32(bg4);
            dl->AddCircleFilled(tc, ri, bg_col2, 32);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Settings");

        ImGui::Dummy(ImVec2(0, 2));

        section_gap();

        if (ImGui::CollapsingHeader("Humanization", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Dummy(ImVec2(0, 3));

            if (og_theme) {
                ImGui::Combo("Humanization Type##og_htype", &maniac::config.humanization_type, "Static\0Dynamic (new)\0UR (Varies)\0UR (Closer)\0\0");
                ImGui::SameLine();
                help_marker("Static: density per 1s chunk. Dynamic: density 1s ahead of each hit. UR Varies: random UR in range each run. UR Closer: same offsets every run.");
                if (maniac::config.humanization_type != maniac::config::UR_HUMANIZATION &&
                    maniac::config.humanization_type != maniac::config::UR_STATIC_HUMANIZATION) {
                    ImGui::InputInt("Humanization##og_hmod", &maniac::config.humanization_modifier, 1, 10);
                    ImGui::SameLine();
                    help_marker("Density-based hit-time offset.");
                } else if (maniac::config.humanization_type == maniac::config::UR_HUMANIZATION) {
                    ImGui::InputInt("Min UR##og_ur_min", &maniac::config.ur_target, 1, 10);
                    ImGui::InputInt("Max UR##og_ur_max", &maniac::config.ur_target_max, 1, 10);
                    ImGui::SameLine();
                    help_marker("Random UR picked between Min and Max each run.");
                } else {
                    ImGui::InputInt("Target UR##og_ur", &maniac::config.ur_target, 1, 10);
                    ImGui::SameLine();
                    help_marker("Fixed UR target. Same offsets every run on the same map.");
                }
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::Text("Adds a random hit-time offset generated using a normal\ndistribution with given mean and standard deviation.");
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::InputInt("Randomization Mean##og_rmean", &maniac::config.randomization_mean);
                ImGui::InputInt("Randomization Stddev##og_rstddev", &maniac::config.randomization_stddev);
            } else {
                ImGui::SetNextItemWidth(180);
                ImGui::Combo("Type##htype", &maniac::config.humanization_type, "Static\0Dynamic\0UR (Varies)\0UR (Closer)\0\0");
                ImGui::SameLine();
                help_marker("Static: density per 1s chunk. Dynamic: density 1s ahead of each hit. UR Varies: random UR in range each run. UR Closer: same offsets every run on the same map.");

                ImGui::Dummy(ImVec2(0, 2));

                if (maniac::config.humanization_type == maniac::config::UR_HUMANIZATION) {
                    static SliderAnim anim_ur_max;
                    ImGui::SetNextItemWidth(180);
                    animated_slider_int("Min UR##ur_min", &maniac::config.ur_target, 10, 500, anim_ur);
                    ImGui::SameLine();
                    help_marker("Minimum UR for this run (picked randomly between Min and Max).");
                    ImGui::SetNextItemWidth(180);
                    animated_slider_int("Max UR##ur_max", &maniac::config.ur_target_max, 10, 500, anim_ur_max);
                    ImGui::SameLine();
                    help_marker("Maximum UR for this run.");
                } else if (maniac::config.humanization_type == maniac::config::UR_STATIC_HUMANIZATION) {
                    ImGui::SetNextItemWidth(180);
                    animated_slider_int("Target UR##ur", &maniac::config.ur_target, 10, 500, anim_ur);
                    ImGui::SameLine();
                    help_marker("Target UR x10 (e.g. 72 = 7.2 UR). Same offsets every run on the same map.");
                } else {
                    ImGui::SetNextItemWidth(180);
                    animated_slider_int("Modifier##hmod", &maniac::config.humanization_modifier, 0, 500, anim_hmod);
                    ImGui::SameLine();
                    help_marker("Density-based hit-time offset. Higher = more human variation.");
                }

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
