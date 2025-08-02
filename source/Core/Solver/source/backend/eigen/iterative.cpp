#include <Eigen/IterativeLinearSolvers>
#include <RZSolver/Solver.hpp>
#include <iostream>

USTC_CG_NAMESPACE_OPEN_SCOPE

namespace Solver {

template<typename EigenSolver>
class EigenIterativeSolver : public LinearSolver {
   private:
    std::string solver_name;

   public:
    EigenIterativeSolver(const std::string& name) : solver_name(name)
    {
    }

    std::string getName() const override
    {
        return solver_name;
    }
    bool isIterative() const override
    {
        return true;
    }
    bool requiresGPU() const override
    {
        return false;
    }

    SolverResult solve(
        const Eigen::SparseMatrix<float>& A,
        const Eigen::VectorXf& b,
        Eigen::VectorXf& x,
        const SolverConfig& config = SolverConfig{}) override
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        SolverResult result;

        try {
            EigenSolver solver;
            solver.setTolerance(config.tolerance);
            solver.setMaxIterations(config.max_iterations);

            auto setup_end_time = std::chrono::high_resolution_clock::now();
            result.setup_time =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    setup_end_time - start_time);

            auto solve_start_time = std::chrono::high_resolution_clock::now();

            solver.compute(A);
            if (solver.info() != Eigen::Success) {
                result.error_message = "Matrix decomposition failed";
                return result;
            }

            x = solver.solve(b);

            auto solve_end_time = std::chrono::high_resolution_clock::now();
            result.solve_time =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    solve_end_time - solve_start_time);

            result.converged = (solver.info() == Eigen::Success);
            result.iterations = solver.iterations();

            // Check for NaN results first (common in BiCGSTAB breakdown)
            if (!x.allFinite()) {
                result.converged = false;
                result.error_message =
                    "Solver produced NaN/infinite values - numerical breakdown";
                result.final_residual = std::numeric_limits<float>::quiet_NaN();
                return result;
            }

            // Compute actual residual for verification
            Eigen::VectorXf residual = A * x - b;
            float b_norm = b.norm();
            result.final_residual =
                (b_norm > 0) ? residual.norm() / b_norm : residual.norm();

            // Additional check: if residual is too large, mark as failed
            if (result.final_residual > 0.1f) {  // 10% error threshold
                result.converged = false;
                result.error_message =
                    "Solver produced poor quality solution (residual > 10%)";
            }

            if (config.verbose) {
                std::cout << solver_name << ": " << result.iterations
                          << " iterations, residual: " << result.final_residual
                          << std::endl;
            }
        }
        catch (const std::exception& e) {
            result.error_message = e.what();
            result.converged = false;
        }

        return result;
    }
};

// Specific solver implementations
class EigenCGSolver
    : public EigenIterativeSolver<
          Eigen::ConjugateGradient<Eigen::SparseMatrix<float>>> {
   public:
    EigenCGSolver() : EigenIterativeSolver("Eigen Conjugate Gradient")
    {
    }
};

class EigenBiCGStabSolver
    : public EigenIterativeSolver<Eigen::BiCGSTAB<Eigen::SparseMatrix<float>>> {
   public:
    EigenBiCGStabSolver() : EigenIterativeSolver("Eigen BiCGSTAB")
    {
    }

    // Override solve method for BiCGSTAB-specific handling
    SolverResult solve(
        const Eigen::SparseMatrix<float>& A,
        const Eigen::VectorXf& b,
        Eigen::VectorXf& x,
        const SolverConfig& config = SolverConfig{}) override
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        SolverResult result;

        try {
            // For SPD matrices, BiCGSTAB is not recommended - fall back to CG
            if (isSPDMatrix(A) && A.rows() > 500) {
                result.converged = false;
                result.error_message =
                    "BiCGSTAB not suitable for large SPD matrices - use CG "
                    "instead";
                result.final_residual = std::numeric_limits<float>::quiet_NaN();
                return result;
            }

            // Try BiCGSTAB with restart mechanism
            const int max_restarts = 3;
            int restart_count = 0;

            while (restart_count < max_restarts) {
                Eigen::BiCGSTAB<Eigen::SparseMatrix<float>> solver;
                solver.setTolerance(config.tolerance);
                solver.setMaxIterations(std::min(
                    config.max_iterations, 1000));  // Limit per restart

                auto setup_end_time = std::chrono::high_resolution_clock::now();
                result.setup_time =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        setup_end_time - start_time);

                auto solve_start_time =
                    std::chrono::high_resolution_clock::now();

                solver.compute(A);
                if (solver.info() != Eigen::Success) {
                    result.error_message = "Matrix decomposition failed";
                    return result;
                }

                x = solver.solve(b);

                auto solve_end_time = std::chrono::high_resolution_clock::now();
                result.solve_time =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        solve_end_time - solve_start_time);

                // Check for numerical issues
                if (!x.allFinite()) {
                    restart_count++;
                    if (restart_count < max_restarts) {
                        // Restart with random initial guess
                        x = Eigen::VectorXf::Random(A.rows()) * 0.1f;
                        continue;
                    }
                    else {
                        result.converged = false;
                        result.error_message =
                            "BiCGSTAB numerical breakdown after " +
                            std::to_string(max_restarts) + " restarts";
                        result.final_residual =
                            std::numeric_limits<float>::quiet_NaN();
                        return result;
                    }
                }

                result.converged = (solver.info() == Eigen::Success);
                result.iterations = solver.iterations() + restart_count * 1000;

                // Compute actual residual
                Eigen::VectorXf residual = A * x - b;
                float b_norm = b.norm();
                result.final_residual =
                    (b_norm > 0) ? residual.norm() / b_norm : residual.norm();

                // Check if solution is acceptable
                if (result.converged &&
                    result.final_residual < config.tolerance * 10) {
                    break;
                }
                else if (result.final_residual > 0.1f) {
                    restart_count++;
                    if (restart_count < max_restarts) {
                        x = Eigen::VectorXf::Random(A.rows()) * 0.1f;
                        continue;
                    }
                    else {
                        result.converged = false;
                        result.error_message =
                            "BiCGSTAB failed to achieve acceptable accuracy";
                    }
                }
                break;
            }

            if (config.verbose) {
                std::cout << "Eigen BiCGSTAB" << ": " << result.iterations
                          << " iterations (with " << restart_count
                          << " restarts), residual: " << result.final_residual
                          << std::endl;
            }
        }
        catch (const std::exception& e) {
            result.error_message = e.what();
            result.converged = false;
        }

        return result;
    }

   private:
    // Improved heuristic to detect SPD matrices
    bool isSPDMatrix(const Eigen::SparseMatrix<float>& A)
    {
        if (A.rows() != A.cols())
            return false;

        // Check if matrix is symmetric (approximately)
        int sample_size = std::min(100, (int)A.rows());
        int asymmetric_count = 0;
        int total_checks = 0;

        for (int i = 0; i < sample_size; ++i) {
            for (int j = i + 1; j < sample_size; ++j) {
                float aij = A.coeff(i, j);
                float aji = A.coeff(j, i);

                // Only check if at least one is non-zero
                if (abs(aij) > 1e-10f || abs(aji) > 1e-10f) {
                    total_checks++;
                    float max_val = std::max(abs(aij), abs(aji));
                    if (max_val > 1e-10f && abs(aij - aji) > 1e-6f * max_val) {
                        asymmetric_count++;
                    }
                }
            }
        }

        // If more than 10% of checked entries are asymmetric, consider it
        // non-SPD
        if (total_checks > 0 && (float)asymmetric_count / total_checks > 0.1f) {
            return false;
        }

        // Check diagonal dominance and positivity (common in SPD matrices)
        for (int i = 0; i < sample_size; ++i) {
            if (A.coeff(i, i) <= 0)
                return false;
        }

        return true;
    }
};

// Factory functions
std::unique_ptr<LinearSolver> createEigenCGSolver()
{
    return std::make_unique<EigenCGSolver>();
}

std::unique_ptr<LinearSolver> createEigenBiCGStabSolver()
{
    return std::make_unique<EigenBiCGStabSolver>();
}

}  // namespace Solver

USTC_CG_NAMESPACE_CLOSE_SCOPE
