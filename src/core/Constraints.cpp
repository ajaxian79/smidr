#include "core/Constraints.h"

#include <algorithm>
#include <cmath>

namespace smidr {

ParamId ConstraintSolver::add_parameter(float v) {
    params_.push_back(v);
    fixed_.push_back(false);
    return static_cast<ParamId>(params_.size() - 1);
}

float ConstraintSolver::get(ParamId p) const {
    return p < params_.size() ? params_[p] : 0.f;
}

void ConstraintSolver::set(ParamId p, float v) {
    if (p < params_.size()) params_[p] = v;
}

void ConstraintSolver::fix(ParamId p) {
    if (p < fixed_.size()) fixed_[p] = true;
}

void ConstraintSolver::unfix(ParamId p) {
    if (p < fixed_.size()) fixed_[p] = false;
}

void ConstraintSolver::add_constraint(Constraint c) {
    constraints_.push_back(std::move(c));
}

void ConstraintSolver::clear() {
    params_.clear();
    fixed_.clear();
    constraints_.clear();
}

float ConstraintSolver::evaluate_constraint(const Constraint& c) const {
    auto P = [&](size_t i) -> float {
        return c.params[i] < params_.size() ? params_[c.params[i]] : 0.f;
    };

    switch (c.type) {
        case ConstraintType::Distance: {
            if (c.params.size() < 6) return 0.f;
            float dx = P(0) - P(3), dy = P(1) - P(4), dz = P(2) - P(5);
            float d = std::sqrt(dx*dx + dy*dy + dz*dz);
            return d - c.value;
        }
        case ConstraintType::Horizontal: {
            if (c.params.size() < 4) return 0.f;
            return P(1) - P(3);
        }
        case ConstraintType::Vertical: {
            if (c.params.size() < 4) return 0.f;
            return P(0) - P(2);
        }
        case ConstraintType::Coincident: {
            if (c.params.size() < 6) return 0.f;
            float dx = P(0)-P(3), dy = P(1)-P(4), dz = P(2)-P(5);
            return std::sqrt(dx*dx + dy*dy + dz*dz);
        }
        case ConstraintType::Equal: {
            if (c.params.size() < 2) return 0.f;
            return P(0) - P(1);
        }
        case ConstraintType::Fix: {
            if (c.params.empty()) return 0.f;
            return P(0) - c.value;
        }
        case ConstraintType::Parallel: {
            if (c.params.size() < 12) return 0.f;
            Vec3 d1{P(3)-P(0), P(4)-P(1), P(5)-P(2)};
            Vec3 d2{P(9)-P(6), P(10)-P(7), P(11)-P(8)};
            Vec3 cr = d1.cross(d2);
            return cr.length();
        }
        case ConstraintType::Perpendicular: {
            if (c.params.size() < 12) return 0.f;
            Vec3 d1{P(3)-P(0), P(4)-P(1), P(5)-P(2)};
            Vec3 d2{P(9)-P(6), P(10)-P(7), P(11)-P(8)};
            return d1.dot(d2);
        }
        case ConstraintType::Concentric: {
            if (c.params.size() < 6) return 0.f;
            float dx = P(0)-P(3), dy = P(1)-P(4), dz = P(2)-P(5);
            return dx*dx + dy*dy + dz*dz;
        }
        case ConstraintType::Angle: {
            if (c.params.size() < 12) return 0.f;
            Vec3 d1{P(3)-P(0), P(4)-P(1), P(5)-P(2)};
            Vec3 d2{P(9)-P(6), P(10)-P(7), P(11)-P(8)};
            float dot = d1.normalized().dot(d2.normalized());
            dot = std::max(-1.f, std::min(1.f, dot));
            float angle = std::acos(dot);
            return angle - c.value;
        }
        case ConstraintType::Tangent:
        case ConstraintType::Symmetric:
            return 0.f;
    }
    return 0.f;
}

void ConstraintSolver::compute_gradient(const Constraint& c, std::vector<float>& grad) const {
    constexpr float eps = 1e-5f;
    auto& mutable_params = const_cast<std::vector<float>&>(params_);
    for (auto pid : c.params) {
        if (pid >= params_.size()) { grad.push_back(0.f); continue; }
        float orig = mutable_params[pid];
        mutable_params[pid] = orig + eps;
        float fp = evaluate_constraint(c);
        mutable_params[pid] = orig - eps;
        float fm = evaluate_constraint(c);
        mutable_params[pid] = orig;
        grad.push_back((fp - fm) / (2.f * eps));
    }
}

float ConstraintSolver::residual() const {
    float r = 0.f;
    for (auto& c : constraints_) {
        if (!c.active) continue;
        float v = evaluate_constraint(c);
        r += v * v;
    }
    return std::sqrt(r);
}

bool ConstraintSolver::solve(int max_iter, float tolerance) {
    for (int iter = 0; iter < max_iter; ++iter) {
        if (residual() < tolerance) return true;

        std::vector<float> delta(params_.size(), 0.f);
        for (auto& c : constraints_) {
            if (!c.active) continue;
            float r = evaluate_constraint(c);
            std::vector<float> grad;
            compute_gradient(c, grad);

            float gn2 = 0.f;
            for (float g : grad) gn2 += g * g;
            if (gn2 < 1e-12f) continue;

            float step = -r / gn2;
            for (size_t i = 0; i < c.params.size(); ++i) {
                if (c.params[i] >= params_.size() || fixed_[c.params[i]]) continue;
                delta[c.params[i]] += step * grad[i];
            }
        }

        float damping = 0.5f;
        for (size_t i = 0; i < params_.size(); ++i) {
            if (!fixed_[i]) params_[i] += delta[i] * damping;
        }
    }
    return residual() < tolerance;
}

ParamId SketchConstraints::add_point(Vec3 pos) {
    ParamId x = solver_.add_parameter(pos.x);
    solver_.add_parameter(pos.y);
    solver_.add_parameter(pos.z);
    point_x_.push_back(x);
    point_y_.push_back(x + 1);
    point_z_.push_back(x + 2);
    return static_cast<ParamId>(point_x_.size() - 1);
}

Vec3 SketchConstraints::point(ParamId p) const {
    if (p >= point_x_.size()) return {};
    return {solver_.get(point_x_[p]), solver_.get(point_y_[p]), solver_.get(point_z_[p])};
}

void SketchConstraints::set_point(ParamId p, Vec3 v) {
    if (p >= point_x_.size()) return;
    solver_.set(point_x_[p], v.x);
    solver_.set(point_y_[p], v.y);
    solver_.set(point_z_[p], v.z);
}

void SketchConstraints::add_distance(ParamId p1, ParamId p2, float d) {
    Constraint c;
    c.type = ConstraintType::Distance;
    c.params = {point_x_[p1], point_y_[p1], point_z_[p1],
                point_x_[p2], point_y_[p2], point_z_[p2]};
    c.value = d;
    solver_.add_constraint(c);
}

void SketchConstraints::add_horizontal(ParamId p1, ParamId p2) {
    Constraint c;
    c.type = ConstraintType::Horizontal;
    c.params = {point_x_[p1], point_y_[p1], point_x_[p2], point_y_[p2]};
    solver_.add_constraint(c);
}

void SketchConstraints::add_vertical(ParamId p1, ParamId p2) {
    Constraint c;
    c.type = ConstraintType::Vertical;
    c.params = {point_x_[p1], point_y_[p1], point_x_[p2], point_y_[p2]};
    solver_.add_constraint(c);
}

void SketchConstraints::add_coincident(ParamId p1, ParamId p2) {
    Constraint c;
    c.type = ConstraintType::Coincident;
    c.params = {point_x_[p1], point_y_[p1], point_z_[p1],
                point_x_[p2], point_y_[p2], point_z_[p2]};
    solver_.add_constraint(c);
}

void SketchConstraints::add_fix(ParamId p) {
    if (p < point_x_.size()) {
        solver_.fix(point_x_[p]);
        solver_.fix(point_y_[p]);
        solver_.fix(point_z_[p]);
    }
}

bool SketchConstraints::solve(int max_iter) {
    return solver_.solve(max_iter);
}

}  // namespace smidr
