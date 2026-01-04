#pragma once
#include <rzsim/api.h>

#include <Eigen/Eigen>
#include <vector>


RUZINO_NAMESPACE_OPEN_SCOPE class Geometry;

struct ReducedOrderedBasis {
    ReducedOrderedBasis(const Geometry& g);

    std::vector<Eigen::VectorXf> basis;
};

RUZINO_NAMESPACE_CLOSE_SCOPE