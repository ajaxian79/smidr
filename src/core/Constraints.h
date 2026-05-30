#pragma once

#include "core/Math.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace smidr {

using ParamId = uint32_t;
constexpr ParamId kInvalidParam = UINT32_MAX;

enum class ConstraintType {
    Distance,
    Angle,
    Parallel,
    Perpendicular,
    Coincident,
    Horizontal,
    Vertical,
    Equal,
    Fix,
    Tangent,
    Concentric,
    Symmetric,
};

struct Constraint {
    ConstraintType type;
    std::vector<ParamId> params;
    float value = 0.f;
    bool  active = true;
};

class ConstraintSolver {
public:
    ParamId add_parameter(float initial_value);
    float   get(ParamId p) const;
    void    set(ParamId p, float v);

    void fix(ParamId p);
    void unfix(ParamId p);

    void add_constraint(Constraint c);
    void clear();

    float residual() const;
    bool  solve(int max_iter = 100, float tolerance = 1e-6f);

    size_t parameter_count() const { return params_.size(); }
    size_t constraint_count() const { return constraints_.size(); }

private:
    float evaluate_constraint(const Constraint& c) const;
    void  compute_gradient(const Constraint& c, std::vector<float>& grad) const;

    std::vector<float>       params_;
    std::vector<bool>        fixed_;
    std::vector<Constraint>  constraints_;
};

class SketchConstraints {
public:
    ParamId add_point(Vec3 pos);
    Vec3    point(ParamId p) const;
    void    set_point(ParamId p, Vec3 v);

    void add_distance(ParamId p1, ParamId p2, float d);
    void add_horizontal(ParamId p1, ParamId p2);
    void add_vertical(ParamId p1, ParamId p2);
    void add_coincident(ParamId p1, ParamId p2);
    void add_fix(ParamId p);

    bool solve(int max_iter = 100);

    ConstraintSolver& solver() { return solver_; }

private:
    ConstraintSolver solver_;
    std::vector<ParamId> point_x_, point_y_, point_z_;
};

}  // namespace smidr
