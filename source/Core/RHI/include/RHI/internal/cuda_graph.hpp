#pragma once

#if USTC_CG_WITH_CUDA

#include <RHI/api.h>
#include <cuda_runtime.h>
#include <nvrhi/nvrhi.h>

USTC_CG_NAMESPACE_OPEN_SCOPE

namespace cuda {

// Forward declarations
class ICUDAGraph;
class ICUDAGraphExec;
class CUDAGraphCapture;

using CUDAGraphHandle = nvrhi::RefCountPtr<ICUDAGraph>;
using CUDAGraphExecHandle = nvrhi::RefCountPtr<ICUDAGraphExec>;

// CUDA Graph description
struct CUDAGraphDesc {
    cudaStream_t stream = nullptr;  // Stream to capture operations on
    bool enable_capture = true;     // Whether to enable graph capture

    CUDAGraphDesc() = default;
    CUDAGraphDesc(cudaStream_t s) : stream(s)
    {
    }
};

// CUDA Graph interface
class ICUDAGraph : public nvrhi::IResource {
   public:
    virtual ~ICUDAGraph() = default;

    [[nodiscard]] virtual const CUDAGraphDesc& getDesc() const = 0;

    // Begin capturing operations into the graph
    virtual bool beginCapture() = 0;

    // End capturing and create executable graph
    virtual CUDAGraphExecHandle endCapture() = 0;

    // Check if currently capturing
    virtual bool isCapturing() const = 0;

    // Get the underlying CUDA graph
    virtual cudaGraph_t getCudaGraph() const = 0;
};

// CUDA Graph Executable interface
class ICUDAGraphExec : public nvrhi::IResource {
   public:
    virtual ~ICUDAGraphExec() = default;

    // Launch the graph
    virtual bool launch(cudaStream_t stream = nullptr) = 0;

    // Update the graph with a new graph (if topology is compatible)
    virtual bool update(CUDAGraphHandle newGraph) = 0;

    // Get the underlying CUDA graph executable
    virtual cudaGraphExec_t getCudaGraphExec() const = 0;
};

// API Functions
RHI_API CUDAGraphHandle create_cuda_graph(const CUDAGraphDesc& desc);

RHI_API CUDAGraphHandle create_cuda_graph(cudaStream_t stream = nullptr);

// Elegant RAII wrapper that IS the captured graph executable
class CUDAGraphCapture {
   public:
    // Constructor starts capture
    CUDAGraphCapture(CUDAGraphHandle graph) : graph_(graph), exec_(nullptr)
    {
        if (graph_ && graph_->beginCapture()) {
            capturing_ = true;
        }
        else {
            capturing_ = false;
        }
    }

    // Destructor ensures capture is ended
    ~CUDAGraphCapture()
    {
        finalize();
    }

    // Move constructor
    CUDAGraphCapture(CUDAGraphCapture&& other) noexcept
        : graph_(std::move(other.graph_)),
          exec_(std::move(other.exec_)),
          capturing_(other.capturing_)
    {
        other.capturing_ = false;
    }

    // Move assignment
    CUDAGraphCapture& operator=(CUDAGraphCapture&& other) noexcept
    {
        if (this != &other) {
            finalize();
            graph_ = std::move(other.graph_);
            exec_ = std::move(other.exec_);
            capturing_ = other.capturing_;
            other.capturing_ = false;
        }
        return *this;
    }

    // Delete copy operations
    CUDAGraphCapture(const CUDAGraphCapture&) = delete;
    CUDAGraphCapture& operator=(const CUDAGraphCapture&) = delete;

    // Check if capture was successful
    explicit operator bool() const
    {
        return capturing_ || exec_;
    }

    // Get the executable (automatically finalizes capture)
    CUDAGraphExecHandle get()
    {
        finalize();
        return exec_;
    }

    // Dereference operators for direct access
    ICUDAGraphExec* operator->()
    {
        finalize();
        return exec_.Get();
    }

    ICUDAGraphExec& operator*()
    {
        finalize();
        return *exec_;
    }

    // Implicit conversion to handle
    operator CUDAGraphExecHandle()
    {
        return get();
    }

    // Release ownership of the executable
    CUDAGraphExecHandle release()
    {
        finalize();
        return std::move(exec_);
    }

   private:
    void finalize()
    {
        if (capturing_ && graph_ && graph_->isCapturing()) {
            exec_ = graph_->endCapture();
            capturing_ = false;
        }
    }

   private:
    CUDAGraphHandle graph_;
    CUDAGraphExecHandle exec_;
    bool capturing_ = false;
};

// Utility function to capture a lambda into a graph
template<typename F>
CUDAGraphExecHandle capture_cuda_graph(cudaStream_t stream, F&& operations)
{
    auto graph = create_cuda_graph(stream);
    if (!graph->beginCapture()) {
        return nullptr;
    }

    operations();

    return graph->endCapture();
}

// Ultra-elegant RAII factory function that returns a ready-to-use executable
template<typename F>
CUDAGraphCapture capture_graph(cudaStream_t stream, F&& operations)
{
    auto graph = create_cuda_graph(stream);
    CUDAGraphCapture capture(graph);

    if (capture) {
        operations();
    }

    return capture;  // Move semantics, finalization happens on first access
}

// Even more elegant - direct lambda capture
template<typename F>
auto with_cuda_graph(cudaStream_t stream, F&& operations) -> CUDAGraphCapture
{
    return capture_graph(stream, std::forward<F>(operations));
}

}  // namespace cuda

USTC_CG_NAMESPACE_CLOSE_SCOPE

#endif  // USTC_CG_WITH_CUDA
