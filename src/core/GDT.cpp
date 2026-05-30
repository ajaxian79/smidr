#include "core/GDT.h"

#include <cstdio>

namespace smidr {

int GDTSpec::add_datum(const Datum& d) {
    datums.push_back(d);
    return static_cast<int>(datums.size()) - 1;
}

int GDTSpec::add_tolerance(const Tolerance& t) {
    tolerances.push_back(t);
    return static_cast<int>(tolerances.size()) - 1;
}

const char* tolerance_kind_name(ToleranceKind k) {
    switch (k) {
        case ToleranceKind::Flatness: return "Flatness";
        case ToleranceKind::Straightness: return "Straightness";
        case ToleranceKind::Roundness: return "Roundness";
        case ToleranceKind::Cylindricity: return "Cylindricity";
        case ToleranceKind::Parallelism: return "Parallelism";
        case ToleranceKind::Perpendicularity: return "Perpendicularity";
        case ToleranceKind::Angularity: return "Angularity";
        case ToleranceKind::Position: return "Position";
        case ToleranceKind::Concentricity: return "Concentricity";
        case ToleranceKind::Symmetry: return "Symmetry";
        case ToleranceKind::Profile_Line: return "Profile_Line";
        case ToleranceKind::Profile_Surface: return "Profile_Surface";
        case ToleranceKind::TotalRunout: return "TotalRunout";
        case ToleranceKind::CircularRunout: return "CircularRunout";
    }
    return "?";
}

const char* tolerance_kind_symbol(ToleranceKind k) {
    switch (k) {
        case ToleranceKind::Flatness: return "FL";
        case ToleranceKind::Straightness: return "ST";
        case ToleranceKind::Roundness: return "RD";
        case ToleranceKind::Cylindricity: return "CY";
        case ToleranceKind::Parallelism: return "||";
        case ToleranceKind::Perpendicularity: return "PP";
        case ToleranceKind::Angularity: return "AN";
        case ToleranceKind::Position: return "PS";
        case ToleranceKind::Concentricity: return "CO";
        case ToleranceKind::Symmetry: return "SY";
        case ToleranceKind::Profile_Line: return "PL";
        case ToleranceKind::Profile_Surface: return "PS_S";
        case ToleranceKind::TotalRunout: return "TR";
        case ToleranceKind::CircularRunout: return "CR";
    }
    return "?";
}

std::string GDTSpec::format_call_out(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(tolerances.size())) return "";
    auto& t = tolerances[static_cast<size_t>(idx)];
    char buf[256];
    const char* mod = "";
    switch (t.modifier) {
        case Modifier::MMC: mod = " M"; break;
        case Modifier::LMC: mod = " L"; break;
        case Modifier::RFS: mod = " S"; break;
        default: break;
    }
    std::string refs;
    for (auto& d : t.datum_refs) { refs += " "; refs += d; }
    std::snprintf(buf, sizeof buf, "[%s] %.4f%s |%s", tolerance_kind_symbol(t.kind),
                  t.value, mod, refs.c_str());
    return buf;
}

}  // namespace smidr
