# Expression System Architecture

## Overview

The refactored expression system separates concerns by creating a dedicated `Expression` class that encapsulates all expression-related operations, while the `ElementBasis` class focuses on managing shape functions and integration operations.

## Key Components

### 1. Expression Class (`Expression.hpp`)

The `Expression<T>` class is the core abstraction for mathematical expressions:

```cpp
// Create expressions
auto expr = ExpressionD::from_string("x + y*z");

// Variable management
double x = 1.0, y = 2.0, z = 3.0;
expr.add_variable("x", x);
expr.add_variable("y", y);
expr.add_variable("z", z);

// Evaluation
double result = expr.evaluate();

// Arithmetic operations
auto sum = expr1 + expr2;
auto product = expr1 * expr2;
auto scaled = expr1 * 2.5;

// Integration over simplices
double integral = expr.integrate_over_simplex(barycentric_names, mapping, intervals);

// Product integration (for shape function integration)
double product_integral = expr1.integrate_product_with(expr2, barycentric_names, mapping);
```

#### Key Features:
- **String-based creation**: Parse mathematical expressions from strings
- **Variable management**: Add and manage variables by reference
- **Lazy parsing**: Expressions are parsed on first use and cached
- **Arithmetic operations**: Support for +, -, *, / operations
- **Integration capabilities**: Built-in integration over simplices with optional coordinate mapping
- **Future-ready**: Framework for derivative operations (placeholder implementation)

### 2. ElementBasis Class (`ElementBasis.hpp`)

The `ElementBasis` class manages collections of shape functions and provides integration services:

```cpp
// Create element basis
auto fem2d = make_fem_2d();

// Add shape functions as expressions
fem2d->add_vertex_expression("u1");
fem2d->add_vertex_expression("u2"); 
fem2d->add_vertex_expression("1-u1-u2");

// Integration without coordinate mapping
auto results = fem2d->integrate_vertex_against_str("x*x + y*y");

// Integration with coordinate mapping
std::vector<pxr::GfVec2d> triangle = {...};
auto mapped_results = fem2d->integrate_vertex_against_with_mapping("x + y", triangle);
```

#### Key Improvements:
- **Type safety**: Template-based design with compile-time dimension checking
- **Clean API**: Separate methods for different shape function types (vertex, edge, face, volume)
- **Mapping support**: Coordinate transformation for physical domain integration
- **Expression integration**: Direct integration with the new Expression class

### 3. Integration System (`integrate.hpp`)

The integration system provides numerical integration over simplices:

- **Simpson's rule**: For 1D integration
- **Trapezoidal rule**: For 2D and 3D integration with boundary corrections
- **Coordinate mapping**: Support for pullback operations from physical to reference domains
- **Product integration**: Specialized for integrating products of expressions

### 4. Derivative Framework (`derivative.hpp`)

Placeholder framework for future numerical and symbolic differentiation:

```cpp
// Future usage (when implemented)
auto grad_expr = expr.gradient({"x", "y", "z"});
auto partial_x = expr.derivative("x");
```

## Architecture Benefits

### 1. **Separation of Concerns**
- `Expression`: Handles parsing, evaluation, arithmetic, integration
- `ElementBasis`: Manages shape functions and element-specific operations
- Integration functions: Numerical algorithms

### 2. **Type Safety**
- Template-based design ensures compile-time checking
- Dimension-aware operations prevent runtime errors

### 3. **Extensibility**
- Easy to add new expression operations
- Framework ready for derivative implementations
- Support for different element types and dimensions

### 4. **Performance**
- Lazy parsing: expressions parsed only when needed
- Cached compilation: parsed expressions reused
- Reference-based variables: no copying of data

### 5. **User-Friendly API**
- Intuitive string-based expression creation
- Arithmetic operators work naturally
- Factory functions for common cases

## Usage Patterns

### Basic Expression Usage
```cpp
auto expr = make_expression_d("sin(x) + cos(y)");
expr.add_variable("x", x_value);
expr.add_variable("y", y_value);
double result = expr.evaluate();
```

### Element Integration
```cpp
auto fem2d = make_fem_2d();
fem2d->set_vertex_expressions({"u1", "u2", "1-u1-u2"});

// Integrate shape functions against load function
auto load_vector = fem2d->integrate_vertex_against_str("sin(x)*cos(y)");
```

### Coordinate Mapping
```cpp
std::vector<pxr::GfVec2d> physical_triangle = {
    {0.0, 0.0}, {2.0, 0.0}, {1.0, 1.5}
};

auto results = fem2d->integrate_vertex_against_with_mapping(
    "x*x + y*y", physical_triangle);
```

## Future Extensions

1. **Numerical Derivatives**: Implementation of finite difference schemes
2. **Symbolic Derivatives**: Basic symbolic differentiation for common functions  
3. **Higher-Order Integration**: Gauss quadrature for improved accuracy
4. **Caching System**: Cache integration results for repeated computations
5. **Parallel Integration**: OpenMP support for large-scale computations

## Migration Guide

The refactored system is mostly backward compatible at the API level:

- Replace direct `exprtk::expression` usage with `Expression<T>`
- Use factory functions (`make_fem_2d()`) instead of direct construction
- Expression strings remain the same format
- Integration method signatures are preserved

The main benefits are improved type safety, better encapsulation, and a foundation for future enhancements like automatic differentiation.