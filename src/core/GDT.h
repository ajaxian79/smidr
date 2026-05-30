#pragma once

#include "core/Math.h"

#include <string>
#include <vector>

namespace smidr {

enum class ToleranceKind {
    Flatness, Straightness, Roundness, Cylindricity,
    Parallelism, Perpendicularity, Angularity,
    Position, Concentricity, Symmetry,
    Profile_Line, Profile_Surface,
    TotalRunout, CircularRunout
};

enum class Modifier { None, MMC, LMC, RFS };

struct Datum {
    std::string label = "A";
    Vec3 point{0, 0, 0};
    Vec3 normal{0, 0, 1};
};

struct Tolerance {
    ToleranceKind kind = ToleranceKind::Flatness;
    float value = 0.1f;
    Modifier modifier = Modifier::None;
    std::vector<std::string> datum_refs;
    std::string feature_id;
    std::string label;
};

class GDTSpec {
public:
    std::vector<Datum>     datums;
    std::vector<Tolerance> tolerances;

    int add_datum(const Datum& d);
    int add_tolerance(const Tolerance& t);
    std::string format_call_out(int tol_idx) const;
};

const char* tolerance_kind_name(ToleranceKind k);
const char* tolerance_kind_symbol(ToleranceKind k);

}  // namespace smidr
