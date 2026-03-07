#include <maniac/common.h>
#include <maniac/maniac.h>

void maniac::randomize(std::vector<osu::HitObject> &hit_objects, int mean, int stddev) {
    if (stddev <= 0) {
        return;
    }

	std::random_device rd;
	std::mt19937 gen{rd()};

	std::normal_distribution<> distr{static_cast<double>(mean), static_cast<double>(stddev)};

	for (auto &hit_object : hit_objects) {
        hit_object.start_time += std::round(distr(gen));
        hit_object.end_time += std::round(distr(gen));
	}

	debug("randomized %d hit objects with offsets along a normal distribution with mean %d and stddev %d",
        hit_objects.size(), mean, stddev);
}

void maniac::humanize_static(std::vector<osu::HitObject> &hit_objects, int modifier) {
	if (!modifier) {
        return;
    }

	const auto actual_modifier = static_cast<double>(modifier) / 100.0;

	constexpr auto slice_size = 1000;

    const auto latest_hit = std::max_element(hit_objects.begin(), hit_objects.end(), [](auto a, auto b) {
        return a.end_time < b.end_time;
    })->end_time;

    auto slices = std::vector<int>{};
    slices.resize((latest_hit / slice_size) + 1);

    for (const auto &hit_object : hit_objects) {
        slices.at(hit_object.start_time / slice_size)++;

        if (hit_object.is_slider) {
            slices.at(hit_object.end_time / slice_size)++;
        }
    }

    for (auto &hit_object : hit_objects) {
        const auto start_offset = static_cast<int>(slices.at(hit_object.start_time / slice_size) * actual_modifier);

        hit_object.start_time += start_offset;

        if (hit_object.is_slider) {
            const auto end_offset = static_cast<int>(slices.at(hit_object.end_time / slice_size) * actual_modifier);

            hit_object.end_time += end_offset;
        }
    }

	debug("statically humanized %d hit objects (%d slices of %dms) with modifier %d", hit_objects.size(), slices.size(), slice_size, modifier);
}

void maniac::humanize_dynamic(std::vector<osu::HitObject> &hit_objects, int modifier) {
    const auto actual_modifier = static_cast<double>(modifier) / 100.0;

    constexpr auto max_delta = 1000;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> distr(0, 1);

    for (int i = 0; i < hit_objects.size(); i++) {
        auto &cur = hit_objects.at(i);

        int start_density = 0;

        for (int j = i - 1; j >= 0; j--) {
            const auto pre = hit_objects.at(j);

            if ((pre.start_time + max_delta > cur.start_time) || (pre.is_slider && pre.end_time + max_delta > cur.start_time)) {
                start_density++;
            }
        }

        const auto offset = static_cast<int>(start_density * actual_modifier);
        cur.start_time += distr(gen) == 0 ? -1 * offset : offset;

        if (cur.is_slider) {
            int end_density = 0;

            for (int j = i - 1; j >= 0; j--) {
                const auto pre = hit_objects.at(j);

                if ((pre.start_time + max_delta > cur.end_time) || (pre.is_slider && pre.end_time + max_delta > cur.end_time)) {
                    end_density++;
                }
            }

            cur.end_time += static_cast<int>(end_density * actual_modifier);
        }
    }

    debug("dynamically humanized %d hit objects with modifier %d", hit_objects.size(), modifier);
}

void maniac::humanize_ur(std::vector<osu::HitObject> &hit_objects, int ur_target_x10) {
    if (hit_objects.empty() || ur_target_x10 <= 0)
        return;

    const double stddev_ms = static_cast<double>(ur_target_x10) / 10.0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> distr(0.0, stddev_ms);

    for (auto &hit_object : hit_objects) {
        int offset = static_cast<int>(std::round(distr(gen)));
        hit_object.start_time += offset;
        if (hit_object.is_slider) {
            int end_offset = static_cast<int>(std::round(distr(gen)));
            hit_object.end_time += end_offset;
        }
    }

    debug("UR-humanized %d hit objects targeting UR %.1f (stddev %.2fms)",
          hit_objects.size(), ur_target_x10 / 10.0, stddev_ms);
}

void maniac::humanize_ur_static(std::vector<osu::HitObject> &hit_objects, int ur_target_x10) {
    if (hit_objects.empty() || ur_target_x10 <= 0)
        return;

    const double stddev_ms = static_cast<double>(ur_target_x10) / 10.0;

    uint32_t seed = 0;
    for (const auto &h : hit_objects)
        seed ^= static_cast<uint32_t>(h.start_time) * 2654435761u;

    std::mt19937 gen(seed);
    std::normal_distribution<> distr(0.0, stddev_ms);

    for (auto &hit_object : hit_objects) {
        int offset = static_cast<int>(std::round(distr(gen)));
        hit_object.start_time += offset;
        if (hit_object.is_slider) {
            int end_offset = static_cast<int>(std::round(distr(gen)));
            hit_object.end_time += end_offset;
        }
    }

    debug("UR-humanized (static) %d hit objects targeting UR %.1f (stddev %.2fms)",
          hit_objects.size(), ur_target_x10 / 10.0, stddev_ms);
}
