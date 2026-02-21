#include "window.h"
#include "config.h"
#include <maniac/maniac.h>


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

    float t = *v ? 1.0f : 0.0f;

    ImU32 col_bg = *v
        ? ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive))
        : IM_COL32(80, 80, 80, 200);

    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
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

    ImVec4 col(r, g, b, 1.0f);
    if (active) ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r*0.8f, g*0.8f, b*0.8f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(r*0.6f, g*0.6f, b*0.6f, 1.f));

    bool clicked = ImGui::Button(id, ImVec2(22, 22));
    ImGui::PopStyleColor(3);
    if (active) ImGui::PopStyleVar();

    if (clicked) { accent[0] = r; accent[1] = g; accent[2] = b; }
    return clicked;
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

    window::start([&message] {
        ImGui::Begin("maniac-next", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

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
            ImGui::Combo("##theme", &maniac::config.theme_index, "Cherry\0Moonlight\0\0");

            ImGui::Dummy(ImVec2(0, 4));

            ImGui::Text("Accent");
            ImGui::SameLine();
            color_swatch("##pink",   1.0f, 0.40f, 0.70f, maniac::config.accent_color); ImGui::SameLine();
            color_swatch("##blue",   0.40f, 0.60f, 1.0f, maniac::config.accent_color); ImGui::SameLine();
            color_swatch("##green",  0.30f, 0.90f, 0.55f, maniac::config.accent_color); ImGui::SameLine();
            color_swatch("##purple", 0.70f, 0.40f, 1.0f, maniac::config.accent_color); ImGui::SameLine();
            color_swatch("##orange", 1.0f, 0.55f, 0.20f, maniac::config.accent_color); ImGui::SameLine();
            ImGui::ColorEdit3("##accent_custom", maniac::config.accent_color,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();
            ImGui::TextDisabled("custom");

            ImGui::Dummy(ImVec2(0, 3));
        }

        section_gap();

        if (ImGui::CollapsingHeader("Humanization", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Dummy(ImVec2(0, 3));

            ImGui::SetNextItemWidth(180);
            ImGui::Combo("Type##htype", &maniac::config.humanization_type, "Static\0Dynamic\0\0");
            ImGui::SameLine();
            help_marker("Static: density per 1s chunk. Dynamic: density 1s ahead of each hit, applied individually.");

            ImGui::Dummy(ImVec2(0, 2));

            ImGui::SetNextItemWidth(180);
            ImGui::SliderInt("Modifier##hmod", &maniac::config.humanization_modifier, 0, 500);
            ImGui::SameLine();
            help_marker("Density-based hit-time offset. Higher = more human variation.");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextDisabled("Gaussian randomization");
            ImGui::Dummy(ImVec2(0, 2));

            ImGui::SetNextItemWidth(180);
            ImGui::SliderInt("Mean##rmean", &maniac::config.randomization_mean, -50, 50);
            ImGui::SameLine();
            help_marker("Mean of the normal distribution used for random hit-time offset.");

            ImGui::SetNextItemWidth(180);
            ImGui::SliderInt("Std Dev##rstddev", &maniac::config.randomization_stddev, 0, 100);
            ImGui::SameLine();
            help_marker("Standard deviation. Higher = more spread.");

            ImGui::Dummy(ImVec2(0, 3));
        }

        section_gap();

        if (ImGui::CollapsingHeader("Gameplay", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Dummy(ImVec2(0, 3));

            ImGui::SetNextItemWidth(180);
            ImGui::SliderInt("Compensation (ms)##comp", &maniac::config.compensation_offset, -100, 100);
            ImGui::SameLine();
            help_marker("Constant offset added to all hit-times. Compensates for input latency.");

            ImGui::Dummy(ImVec2(0, 2));

            ImGui::SetNextItemWidth(180);
            ImGui::SliderInt("Tap Time (ms)##tap", &maniac::config.tap_time, 1, 100);
            ImGui::SameLine();
            help_marker("How long a key is held down per keypress.");

            ImGui::Dummy(ImVec2(0, 4));

            toggle_switch("Mirror Mod", &maniac::config.mirror_mod);

            ImGui::Dummy(ImVec2(0, 3));
        }

        section_gap();

        ImGui::Dummy(ImVec2(0, 2));
        ImGui::TextDisabled("  maniac-next");
        ImGui::SameLine();
        ImGui::TextDisabled("by miku (original by fs-c)");

        ImGui::End();
    });

    config::write_to_file(maniac::config);
    thread.request_stop();
    return EXIT_SUCCESS;
}
