#pragma once
#include <exprtk/exprtk.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace USTC_CG {
namespace fem_bem {

    // Forward declaration
    template<typename T>
    class Expression;

    // Numerical derivative class that stores the derivative computation
    template<typename T>
    class DerivativeExpression {
       private:
        std::function<T(const std::unordered_map<std::string, T>&)> derivative_func_;
        std::string variable_name_;

       public:
        DerivativeExpression(
            std::function<T(const std::unordered_map<std::string, T>&)> func,
            const std::string& var_name)
            : derivative_func_(std::move(func)), variable_name_(var_name)
        {
        }

        T evaluate_at(const std::unordered_map<std::string, T>& values) const
        {
            return derivative_func_(values);
        }

        std::string get_string() const
        {
            return "";  // Derivative expressions don't have string representation
        }

        bool is_string_based() const
        {
            return false;
        }
    };

    // Numerical derivative using finite differences (5-point stencil)
    template<typename T>
    inline T numerical_derivative(
        const exprtk::expression<T>& expr,
        exprtk::details::variable_node<T>* var,
        const T& h = T(0.00000001))
    {
        const T x_init = var->ref();
        const T _2h = T(2) * h;

        var->ref() = x_init + _2h;
        const T y0 = expr.value();
        var->ref() = x_init + h;
        const T y1 = expr.value();
        var->ref() = x_init - h;
        const T y2 = expr.value();
        var->ref() = x_init - _2h;
        const T y3 = expr.value();
        var->ref() = x_init;

        return (-y0 + T(8) * (y1 - y2) + y3) / (T(12) * h);
    }

    // Create a derivative function for an expression
    template<typename T>
    std::function<T(const std::unordered_map<std::string, T>&)>
    create_derivative_function(
        const std::string& expression_string,
        const std::string& variable_name,
        const T& h = T(0.00000001))
    {
        return [expression_string, variable_name, h](
                   const std::unordered_map<std::string, T>& values) -> T {
            // Create a temporary expression for derivative computation
            exprtk::symbol_table<T> symbol_table;
            exprtk::expression<T> expr;
            exprtk::parser<T> parser;

            // Add all variables to symbol table
            std::unordered_map<std::string, T> temp_values = values;
            
            for (auto& [name, value] : temp_values) {
                symbol_table.add_variable(name, value);
            }

            symbol_table.add_constants();
            expr.register_symbol_table(symbol_table);

            if (!parser.compile(expression_string, expr)) {
                return T(0);  // Return 0 for invalid expressions
            }

            // Find the variable node for differentiation
            auto* var_node = symbol_table.get_variable(variable_name);
            if (!var_node) {
                return T(0);  // Variable not found
            }

            return numerical_derivative(expr, var_node, h);
        };
    }

}  // namespace fem_bem
}  // namespace USTC_CG