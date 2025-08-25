#include <Eigen/Eigen>
#include <autodiff/forward/dual.hpp>
#include <autodiff/forward/dual/eigen.hpp>
#include <autodiff/forward/real.hpp>
#include <autodiff/forward/real/eigen.hpp>
#include <autodiff/reverse/var.hpp>
#include <autodiff/reverse/var/eigen.hpp>

#include "nodes/core/def/node_def.hpp"

using namespace autodiff;

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_FUNCTION(function_add_backward)
{
    b.add_input<std::function<var(const ArrayXvar&)>>("Function_1");
    b.add_input<std::function<var(const ArrayXvar&)>>("Function_2");
    b.add_output<std::function<var(const ArrayXvar&)>>("Function_result");
}

NODE_EXECUTION_FUNCTION(function_add_backward)
{
    auto f1 =
        params.get_input<std::function<var(const ArrayXvar&)>>("Function_1");
    auto f2 =
        params.get_input<std::function<var(const ArrayXvar&)>>("Function_2");
    auto f = [f1, f2](const ArrayXvar& x) {
        var y = f1(x) + f2(x);
        return y;
    };
    params.set_output<std::function<var(const ArrayXvar&)>>(
        "Function_result", std::move(f));
    return true;
}

NODE_DECLARATION_FUNCTION(function_add_forward_dual)
{
    b.add_input<std::function<dual(const ArrayXdual&)>>("Function_1");
    b.add_input<std::function<dual(const ArrayXdual&)>>("Function_2");
    b.add_output<std::function<dual(const ArrayXdual&)>>("Function_result");
}

NODE_EXECUTION_FUNCTION(function_add_forward_dual)
{
    auto f1 =
        params.get_input<std::function<dual(const ArrayXdual&)>>("Function_1");
    auto f2 =
        params.get_input<std::function<dual(const ArrayXdual&)>>("Function_2");
    auto f = [f1, f2](const ArrayXdual& x) {
        dual y = f1(x) + f2(x);
        return y;
    };
    params.set_output<std::function<dual(const ArrayXdual&)>>(
        "Function_result", std::move(f));
    return true;
}

NODE_DECLARATION_FUNCTION(function_add_forward_real)
{
    b.add_input<std::function<real(const ArrayXreal&)>>("Function_1");
    b.add_input<std::function<real(const ArrayXreal&)>>("Function_2");
    b.add_output<std::function<real(const ArrayXreal&)>>("Function_result");
}

NODE_EXECUTION_FUNCTION(function_add_forward_real)
{
    auto f1 =
        params.get_input<std::function<real(const ArrayXreal&)>>("Function_1");
    auto f2 =
        params.get_input<std::function<real(const ArrayXreal&)>>("Function_2");
    auto f = [f1, f2](const ArrayXreal& x) {
        real y = f1(x) + f2(x);
        return y;
    };
    params.set_output<std::function<real(const ArrayXreal&)>>(
        "Function_result", std::move(f));
    return true;
}

NODE_DECLARATION_FUNCTION(function_composition_backward)
{
    b.add_input<std::function<var(const ArrayXvar&)>>("Function_1");
    b.add_input<std::function<var(var)>>("Function_2");
    b.add_output<std::function<var(const ArrayXvar&)>>("Function_result");
}

NODE_EXECUTION_FUNCTION(function_composition_backward)
{
    auto f1 =
        params.get_input<std::function<var(const ArrayXvar&)>>("Function_1");
    auto f2 = params.get_input<std::function<var(var)>>("Function_2");
    auto f = [f1, f2](const ArrayXvar& x) {
        var y = f2(f1(x));
        return y;
    };
    params.set_output<std::function<var(const ArrayXvar&)>>(
        "Function_result", std::move(f));
    return true;
}

NODE_DECLARATION_FUNCTION(function_composition_forward_dual)
{
    b.add_input<std::function<dual(const ArrayXdual&)>>("Function_1");
    b.add_input<std::function<dual(dual)>>("Function_2");
    b.add_output<std::function<dual(const ArrayXdual&)>>("Function_result");
}

NODE_EXECUTION_FUNCTION(function_composition_forward_dual)
{
    auto f1 =
        params.get_input<std::function<dual(const ArrayXdual&)>>("Function_1");
    auto f2 = params.get_input<std::function<dual(dual)>>("Function_2");
    auto f = [f1, f2](const ArrayXdual& x) {
        dual y = f2(f1(x));
        return y;
    };
    params.set_output<std::function<dual(const ArrayXdual&)>>(
        "Function_result", std::move(f));
    return true;
}

NODE_DECLARATION_FUNCTION(function_composition_forward_real)
{
    b.add_input<std::function<real(const ArrayXreal&)>>("Function_1");
    b.add_input<std::function<real(real)>>("Function_2");
    b.add_output<std::function<real(const ArrayXreal&)>>("Function_result");
}

NODE_EXECUTION_FUNCTION(function_composition_forward_real)
{
    auto f1 =
        params.get_input<std::function<real(const ArrayXreal&)>>("Function_1");
    auto f2 = params.get_input<std::function<real(real)>>("Function_2");
    auto f = [f1, f2](const ArrayXreal& x) {
        real y = f2(f1(x));
        return y;
    };
    params.set_output<std::function<real(const ArrayXreal&)>>(
        "Function_result", std::move(f));
    return true;
}

NODE_DECLARATION_FUNCTION(function_multiply_backward)
{
    b.add_input<std::function<var(const ArrayXvar&)>>("Function_1");
    b.add_input<std::function<var(const ArrayXvar&)>>("Function_2");
    b.add_output<std::function<var(const ArrayXvar&)>>("Function_result");
}

NODE_EXECUTION_FUNCTION(function_multiply_backward)
{
    auto f1 =
        params.get_input<std::function<var(const ArrayXvar&)>>("Function_1");
    auto f2 =
        params.get_input<std::function<var(const ArrayXvar&)>>("Function_2");
    auto f = [f1, f2](const ArrayXvar& x) {
        var y = f1(x) * f2(x);
        return y;
    };
    params.set_output<std::function<var(const ArrayXvar&)>>(
        "Function_result", std::move(f));
    return true;
}

NODE_DECLARATION_FUNCTION(function_multiply_forward_dual)
{
    b.add_input<std::function<dual(const ArrayXdual&)>>("Function_1");
    b.add_input<std::function<dual(const ArrayXdual&)>>("Function_2");
    b.add_output<std::function<dual(const ArrayXdual&)>>("Function_result");
}

NODE_EXECUTION_FUNCTION(function_multiply_forward_dual)
{
    auto f1 =
        params.get_input<std::function<dual(const ArrayXdual&)>>("Function_1");
    auto f2 =
        params.get_input<std::function<dual(const ArrayXdual&)>>("Function_2");
    auto f = [f1, f2](const ArrayXdual& x) {
        dual y = f1(x) * f2(x);
        return y;
    };
    params.set_output<std::function<dual(const ArrayXdual&)>>(
        "Function_result", std::move(f));
    return true;
}

NODE_DECLARATION_FUNCTION(function_multiply_forward_real)
{
    b.add_input<std::function<real(const ArrayXreal&)>>("Function_1");
    b.add_input<std::function<real(const ArrayXreal&)>>("Function_2");
    b.add_output<std::function<real(const ArrayXreal&)>>("Function_result");
}

NODE_EXECUTION_FUNCTION(function_multiply_forward_real)
{
    auto f1 =
        params.get_input<std::function<real(const ArrayXreal&)>>("Function_1");
    auto f2 =
        params.get_input<std::function<real(const ArrayXreal&)>>("Function_2");
    auto f = [f1, f2](const ArrayXreal& x) {
        real y = f1(x) * f2(x);
        return y;
    };
    params.set_output<std::function<real(const ArrayXreal&)>>(
        "Function_result", std::move(f));
    return true;
}

NODE_DECLARATION_UI(function_add_backward);
NODE_DECLARATION_UI(function_add_forward_dual);
NODE_DECLARATION_UI(function_add_forward_real);
NODE_DECLARATION_UI(function_composition_backward);
NODE_DECLARATION_UI(function_composition_forward_dual);
NODE_DECLARATION_UI(function_composition_forward_real);
NODE_DECLARATION_UI(function_multiply_backward);
NODE_DECLARATION_UI(function_multiply_forward_dual);
NODE_DECLARATION_UI(function_multiply_forward_real);
NODE_DEF_CLOSE_SCOPE