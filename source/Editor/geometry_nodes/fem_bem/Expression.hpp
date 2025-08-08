#pragma once
#include <exprtk/exprtk.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "derivative.hpp"
#include "integrate.hpp"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3d.h"

namespace USTC_CG {
namespace fem_bem {

    // Forward declarations
    template<typename T>
    class DerivativeExpression;

    template<typename T = double>
    class Expression {
       public:
        using value_type = T;
        using expression_type = exprtk::expression<T>;
        using symbol_table_type = exprtk::symbol_table<T>;
        using parser_type = exprtk::parser<T>;

        // Constructors
        Expression() = default;
        explicit Expression(const std::string& expr_str)
            : expression_string_(expr_str)
        {
        }

        Expression(
            const std::string& expr_str,
            const std::vector<std::string>& variable_names)
            : expression_string_(expr_str),
              variable_names_(variable_names)
        {
        }

        // Copy constructor and assignment
        Expression(const Expression& other)
            : expression_string_(other.expression_string_),
              variable_names_(other.variable_names_),
              is_parsed_(false)  // Force re-parsing with new symbol table
        {
        }

        Expression& operator=(const Expression& other)
        {
            if (this != &other) {
                expression_string_ = other.expression_string_;
                variable_names_ = other.variable_names_;
                is_parsed_ = false;  // Force re-parsing
                compiled_expression_.reset();
                symbol_table_.reset();
            }
            return *this;
        }

        // Move constructor and assignment
        Expression(Expression&& other) noexcept = default;
        Expression& operator=(Expression&& other) noexcept = default;

        // Factory methods
        static Expression from_string(const std::string& expr_str)
        {
            return Expression(expr_str);
        }

        static Expression constant(T value)
        {
            return Expression(std::to_string(value));
        }

        static Expression zero()
        {
            return Expression("0");
        }

        static Expression one()
        {
            return Expression("1");
        }

        // Basic properties
        const std::string& get_string() const
        {
            return expression_string_;
        }

        // Method to check if this is a string-based expression
        bool is_string_based() const
        {
            return true;  // Regular expressions are always string-based
        }

        // Evaluation
        T evaluate() const
        {
            ensure_parsed();
            if (!compiled_expression_) {
                throw std::runtime_error(
                    "Expression not properly parsed: " + expression_string_);
            }
            return compiled_expression_->value();
        }

        T evaluate_at(
            const std::unordered_map<std::string, T>& variable_values) const
        {
            // Lazy parsing with variable discovery
            if (!is_parsed_ || !compiled_expression_) {
                // If no variables specified, discover them from the values
                // provided
                if (variable_names_.empty()) {
                    for (const auto& [name, value] : variable_values) {
                        variable_names_.push_back(name);
                    }
                }
                parse_expression();
            }

            if (!compiled_expression_) {
                throw std::runtime_error(
                    "Expression not properly parsed: " + expression_string_);
            }

            // Store original values
            std::unordered_map<std::string, T> original_values;
            const exprtk::symbol_table<T>& sym_table =
                compiled_expression_->get_symbol_table();

            for (const auto& [name, value] : variable_values) {
                auto* var_node = sym_table.get_variable(name);
                if (var_node) {
                    original_values[name] = var_node->ref();
                    var_node->ref() = value;
                }
            }

            T result = compiled_expression_->value();

            // Restore original values
            for (const auto& [name, value] : original_values) {
                auto* var_node = sym_table.get_variable(name);
                if (var_node) {
                    var_node->ref() = value;
                }
            }

            return result;
        }

        // Arithmetic operations
        Expression operator+(const Expression& other) const
        {
            return Expression(
                "(" + expression_string_ + ") + (" + other.expression_string_ +
                ")");
        }

        Expression operator-(const Expression& other) const
        {
            return Expression(
                "(" + expression_string_ + ") - (" + other.expression_string_ +
                ")");
        }

        Expression operator*(const Expression& other) const
        {
            return Expression(
                "(" + expression_string_ + ") * (" + other.expression_string_ +
                ")");
        }

        Expression operator/(const Expression& other) const
        {
            return Expression(
                "(" + expression_string_ + ") / (" + other.expression_string_ +
                ")");
        }

        Expression operator*(T scalar) const
        {
            return Expression(
                std::to_string(scalar) + " * (" + expression_string_ + ")");
        }

        Expression operator-() const
        {
            return Expression("-(" + expression_string_ + ")");
        }

        // Integration methods
        template<typename MappingFunc = std::nullptr_t>
        T integrate_over_simplex(
            const std::vector<std::string>& barycentric_names,
            MappingFunc mapping = nullptr,
            std::size_t intervals = 100) const
        {
            ensure_parsed();
            if constexpr (std::is_same_v<MappingFunc, std::nullptr_t>) {
                return integrate_simplex(
                    *compiled_expression_, barycentric_names, intervals);
            }
            else {
                // For mapping integration, we need to handle coordinate
                // variables
                return integrate_simplex_with_mapping(
                    *compiled_expression_,
                    mapping,
                    barycentric_names,
                    intervals);
            }
        }

        template<typename MappingFunc = std::nullptr_t>
        T integrate_product_with(
            const Expression& other,
            const std::vector<std::string>& barycentric_names,
            MappingFunc mapping = nullptr,
            std::size_t intervals = 100) const
        {
            ensure_parsed();
            other.ensure_parsed();

            if constexpr (std::is_same_v<MappingFunc, std::nullptr_t>) {
                return integrate_product_simplex(
                    *compiled_expression_,
                    *other.compiled_expression_,
                    barycentric_names,
                    intervals);
            }
            else {
                return integrate_product_simplex_with_mapping(
                    *compiled_expression_,
                    *other.compiled_expression_,
                    mapping,
                    barycentric_names,
                    intervals);
            }
        }

        // Derivative methods - now properly implemented
        DerivativeExpression<T> derivative(
            const std::string& variable_name) const
        {
            auto derivative_func = create_derivative_function<T>(
                expression_string_, variable_name);
            return DerivativeExpression<T>(derivative_func, variable_name);
        }

        std::vector<DerivativeExpression<T>> gradient(
            const std::vector<std::string>& variable_names) const
        {
            std::vector<DerivativeExpression<T>> grad;
            grad.reserve(variable_names.size());
            for (const auto& var : variable_names) {
                grad.push_back(derivative(var));
            }
            return grad;
        }

        // Access to underlying exprtk objects (for advanced use)
        const expression_type* get_compiled_expression() const
        {
            ensure_parsed();
            return compiled_expression_.get();
        }

        const symbol_table_type* get_symbol_table() const
        {
            ensure_parsed();
            return symbol_table_.get();
        }

       private:
        std::string expression_string_;
        mutable std::vector<std::string> variable_names_;

        // Parsed expression components
        mutable std::unique_ptr<symbol_table_type> symbol_table_;
        mutable std::unique_ptr<expression_type> compiled_expression_;
        mutable bool is_parsed_ = false;

        // Storage for variables
        mutable std::unordered_map<std::string, T> temp_variables_;

        void ensure_parsed() const
        {
            if (!is_parsed_ || !compiled_expression_) {
                parse_expression();
            }
        }

        void parse_expression() const
        {
            symbol_table_ = std::make_unique<symbol_table_type>();
            compiled_expression_ = std::make_unique<expression_type>();

            // Enable constants and standard functions
            symbol_table_->add_constants();

            // Register specified variables only
            if (!variable_names_.empty()) {
                for (const auto& var_name : variable_names_) {
                    auto& temp_var = temp_variables_[var_name];
                    temp_var = T(0);
                    symbol_table_->add_variable(var_name, temp_var);
                }
            }

            // Register symbol table with expression
            compiled_expression_->register_symbol_table(*symbol_table_);

            parser_type parser;
            if (!parser.compile(expression_string_, *compiled_expression_)) {
                throw std::runtime_error(
                    "Failed to parse expression: " + expression_string_);
            }

            is_parsed_ = true;
        }

        template<typename MappingFunc>
        T integrate_simplex_with_mapping(
            const expression_type& expr,
            MappingFunc mapping,
            const std::vector<std::string>& barycentric_names,
            std::size_t intervals) const
        {
            // This is a simpler version that just integrates a single
            // expression with coordinate mapping We need to create a dummy
            // constant expression to use the product integration
            Expression<T> constant_one("1");

            constant_one.ensure_parsed();

            return integrate_product_simplex_with_mapping(
                *constant_one.compiled_expression_,
                expr,
                mapping,
                barycentric_names,
                intervals);
        }
    };

    // Type aliases
    using ExpressionD = Expression<double>;
    using ExpressionF = Expression<float>;

    // Free function operators for scalar * Expression
    template<typename T>
    Expression<T> operator*(T scalar, const Expression<T>& expr)
    {
        return expr * scalar;
    }

    // Utility functions
    template<typename T>
    Expression<T> make_expression(const std::string& expr_str)
    {
        return Expression<T>::from_string(expr_str);
    }

    inline ExpressionD make_expression_d(const std::string& expr_str)
    {
        return ExpressionD::from_string(expr_str);
    }

    inline ExpressionF make_expression_f(const std::string& expr_str)
    {
        return ExpressionF::from_string(expr_str);
    }

}  // namespace fem_bem
}  // namespace USTC_CG
