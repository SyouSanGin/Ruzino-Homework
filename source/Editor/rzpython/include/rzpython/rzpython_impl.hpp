#pragma once
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "api.h"

RUZINO_NAMESPACE_OPEN_SCOPE

namespace python {

namespace nb = nanobind;

// Forward declarations for external variables
RZPYTHON_EXTERN RZPYTHON_API PyObject* main_dict;
RZPYTHON_EXTERN RZPYTHON_API bool initialized;
RZPYTHON_EXTERN RZPYTHON_API std::unordered_map<std::string, nb::object>
    bound_objects;

// Forward declare functions that are implemented in the .cpp file
RZPYTHON_API PyObject* call_raw(const std::string& code);
RZPYTHON_API bool is_boost_python_module(const std::string& module_name);
RZPYTHON_API void safe_import(const std::string& module_name);

// Helper to determine if we should use eval or file input mode
template<typename T>
constexpr bool is_void_type()
{
    return std::is_void_v<T>;
}

// Generic template implementation using nanobind casting
template<typename T>
T call(const std::string& code)
{
    static_assert(!std::is_void_v<T>, "Use call<void>() for void return type");

    PyObject* py_result = call_raw(code);
    if (!py_result) {
        throw std::runtime_error(
            "Failed to get result from Python code: " + code);
    }

    try {
        // Use direct nanobind casting - don't steal ownership yet
        T result = nb::cast<T>(nb::handle(py_result));
        Py_DECREF(py_result);  // Now we can safely decref
        return result;
    }
    catch (const nb::cast_error& e) {
        Py_DECREF(py_result);
        throw std::runtime_error(
            "Failed to convert Python result to C++ type: " +
            std::string(e.what()));
    }
    catch (const std::exception& e) {
        Py_DECREF(py_result);
        throw std::runtime_error(
            "Failed to convert Python result: " + std::string(e.what()));
    }
}

// Specialized version for handling potentially mixed binding types
template<typename T>
T call_safe(const std::string& code)
{
    try {
        return call<T>(code);
    }
    catch (const std::exception& e) {
        // If regular nanobind conversion fails, we can't do much more
        // in the core library since we don't link to specific external
        // libraries
        throw std::runtime_error(
            "Mixed binding conversion failed: " + std::string(e.what()) +
            ". Consider using specialized conversion functions for external "
            "libraries.");
    }
}

template<typename T>
void reference(const std::string& name, T* obj)
{
    if (!initialized) {
        throw std::runtime_error("Python interpreter not initialized");
    }

    try {
        // Create nanobind object wrapper with proper policy
        nb::object py_obj = nb::cast(obj, nb::rv_policy::reference);

        // Store in our map to keep it alive
        bound_objects[name] = py_obj;

        // Add to Python's main dict
        PyDict_SetItemString(main_dict, name.c_str(), py_obj.ptr());
    }
    catch (const nb::cast_error& e) {
        throw std::runtime_error(
            "Failed to bind object '" + name + "': " + e.what());
    }
    catch (const std::exception& e) {
        throw std::runtime_error(
            "Failed to bind object '" + name + "': " + e.what());
    }
}

template<typename T>
void send(const std::string& name, const T& value)
{
    if (!initialized) {
        throw std::runtime_error("Python interpreter not initialized");
    }

    try {
        // Create nanobind object from the C++ value
        // Use copy policy for values
        nb::object py_obj = nb::cast(value, nb::rv_policy::copy);

        // Store in our map to keep it alive
        bound_objects[name] = py_obj;

        // Add to Python's main dict
        PyDict_SetItemString(main_dict, name.c_str(), py_obj.ptr());
    }
    catch (const nb::cast_error& e) {
        throw std::runtime_error(
            "Failed to send value to Python variable '" + name +
            "': " + e.what());
    }
    catch (const std::exception& e) {
        throw std::runtime_error(
            "Failed to send value to Python variable '" + name +
            "': " + e.what());
    }
}

template<typename T>
void send_ref(const std::string& name, T& obj)
{
    if (!initialized) {
        throw std::runtime_error("Python interpreter not initialized");
    }

    try {
        // Create nanobind object from the C++ object with reference policy
        // Use reference_internal for non-pointer objects
        nb::object py_obj = nb::cast(&obj, nb::rv_policy::reference);

        // Store in our map to keep it alive
        bound_objects[name] = py_obj;

        // Add to Python's main dict
        PyDict_SetItemString(main_dict, name.c_str(), py_obj.ptr());
    }
    catch (const nb::cast_error& e) {
        throw std::runtime_error(
            "Failed to send reference to Python variable '" + name +
            "': " + e.what());
    }
    catch (const std::exception& e) {
        throw std::runtime_error(
            "Failed to send reference to Python variable '" + name +
            "': " + e.what());
    }
}

template<typename T>
T get(const std::string& name)
{
    if (!initialized) {
        throw std::runtime_error("Python interpreter not initialized");
    }

    try {
        // Get object from Python's main dict
        PyObject* py_obj = PyDict_GetItemString(main_dict, name.c_str());

        if (!py_obj) {
            throw std::runtime_error(
                "Python variable '" + name + "' not found");
        }

        if (py_obj == Py_None) {
            throw std::runtime_error("Python variable '" + name + "' is None");
        }

        // Convert to C++ type using nanobind
        return nb::cast<T>(nb::handle(py_obj));
    }
    catch (const nb::cast_error& e) {
        throw std::runtime_error(
            "Failed to convert Python variable '" + name +
            "' to C++ type: " + e.what());
    }
    catch (const std::exception& e) {
        throw std::runtime_error(
            "Failed to get Python variable '" + name + "': " + e.what());
    }
}

// Specialized send for CudaArrayView - zero-copy transfer to Python
template<typename T>
void send(const std::string& name, const CudaArrayView<T>& cuda_view)
{
    if (!initialized) {
        throw std::runtime_error("Python interpreter not initialized");
    }

    try {
        // Create nanobind ndarray from CUDA pointer without copying
        size_t ndim = cuda_view.shape.size();

        // Create ndarray with external ownership
        nb::ndarray<nb::pytorch, T, nb::device::cuda> array(
            cuda_view.data,
            ndim,
            cuda_view.shape.data(),
            nb::handle()  // No owner - we don't manage the memory
        );

        nb::object py_obj = nb::cast(array);

        // Store in our map to keep it alive
        bound_objects[name] = py_obj;

        // Add to Python's main dict
        PyDict_SetItemString(main_dict, name.c_str(), py_obj.ptr());
    }
    catch (const nb::cast_error& e) {
        throw std::runtime_error(
            "Failed to send CUDA array to Python variable '" + name +
            "': " + e.what());
    }
    catch (const std::exception& e) {
        throw std::runtime_error(
            "Failed to send CUDA array to Python variable '" + name +
            "': " + e.what());
    }
}

}  // namespace python

RUZINO_NAMESPACE_CLOSE_SCOPE
