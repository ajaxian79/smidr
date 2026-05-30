#pragma once

#include "core/Slicer.h"

#include <filesystem>
#include <string>
#include <vector>

namespace smidr {

struct GCodeSettings {
    float layer_height = 0.2f;
    float nozzle_diameter = 0.4f;
    float print_speed = 50.f;
    float travel_speed = 120.f;
    float retract = 1.f;
    int   perimeters = 2;
    float infill_density = 0.2f;
    float infill_angle = 45.f;
    float extruder_temp = 200.f;
    float bed_temp = 60.f;
};

bool generate_gcode(const std::filesystem::path& path,
                    const Mesh& mesh,
                    const GCodeSettings& settings);

}  // namespace smidr
