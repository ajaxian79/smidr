#pragma once

#include "core/NURBS.h"
#include "core/BREPIntersect.h"

namespace smidr {

BREPSolid brep_union(const BREPSolid& a, const BREPSolid& b);
BREPSolid brep_intersect(const BREPSolid& a, const BREPSolid& b);
BREPSolid brep_subtract(const BREPSolid& a, const BREPSolid& b);

bool point_inside_brep(Vec3 p, const BREPSolid& solid, int tessellation = 16);

}  // namespace smidr
