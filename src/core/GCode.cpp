#include "core/GCode.h"

#include <cmath>
#include <fstream>

namespace smidr {

static float polyline_length(const Polyline& pl) {
    float len = 0.f;
    for (size_t i = 0; i + 1 < pl.points.size(); ++i) {
        len += (pl.points[i+1] - pl.points[i]).length();
    }
    if (pl.closed && pl.points.size() > 1) {
        len += (pl.points[0] - pl.points.back()).length();
    }
    return len;
}

bool generate_gcode(const std::filesystem::path& path,
                    const Mesh& mesh,
                    const GCodeSettings& s) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    AABB bb = mesh.bounds();
    float z_min = bb.min_pt.y;
    float z_max = bb.max_pt.y;

    f << "; smidr G-code\n";
    f << "; layer_height=" << s.layer_height << " nozzle=" << s.nozzle_diameter << "\n";
    f << "G21 ; mm\n";
    f << "G90 ; absolute\n";
    f << "M82 ; abs extruder\n";
    f << "M104 S" << static_cast<int>(s.extruder_temp) << "\n";
    f << "M140 S" << static_cast<int>(s.bed_temp) << "\n";
    f << "M109 S" << static_cast<int>(s.extruder_temp) << "\n";
    f << "M190 S" << static_cast<int>(s.bed_temp) << "\n";
    f << "G28 ; home\n";
    f << "G92 E0\n";

    float E = 0.f;
    float filament_area = kPi * (1.75f * 1.75f) * 0.25f;
    float extrusion_area = s.nozzle_diameter * s.layer_height;
    float extrusion_per_mm = extrusion_area / filament_area;

    int layer = 0;
    for (float z = z_min + s.layer_height; z <= z_max; z += s.layer_height) {
        auto slices = slice_mesh(mesh, {0, 1, 0}, z);
        if (slices.empty()) continue;

        f << "; LAYER:" << layer++ << " Z=" << z << "\n";
        f << "G1 Z" << z << " F" << (s.travel_speed * 60.f) << "\n";

        for (auto& pl : slices) {
            if (pl.points.size() < 2) continue;
            f << "G0 X" << pl.points[0].x << " Y" << pl.points[0].z
              << " F" << (s.travel_speed * 60.f) << "\n";
            f << "G1 F" << (s.print_speed * 60.f) << "\n";
            for (size_t i = 1; i < pl.points.size(); ++i) {
                Vec3 d = pl.points[i] - pl.points[i-1];
                float dist = d.length();
                E += dist * extrusion_per_mm;
                f << "G1 X" << pl.points[i].x << " Y" << pl.points[i].z
                  << " E" << E << "\n";
            }
            if (pl.closed && pl.points.size() > 2) {
                Vec3 d = pl.points[0] - pl.points.back();
                float dist = d.length();
                E += dist * extrusion_per_mm;
                f << "G1 X" << pl.points[0].x << " Y" << pl.points[0].z
                  << " E" << E << "\n";
            }
        }
    }

    f << "G1 E" << (E - s.retract) << " F2400 ; retract\n";
    f << "M104 S0\n";
    f << "M140 S0\n";
    f << "G28 X0 Y0\n";
    f << "M84\n";
    return f.good();
}

}  // namespace smidr
