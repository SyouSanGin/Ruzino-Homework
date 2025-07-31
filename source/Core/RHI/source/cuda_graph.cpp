#include <sstream>
#if USTC_CG_WITH_CUDA

#include <RHI/internal/cuda_extension_utils.h>

#include <RHI/internal/cuda_graph.hpp>
#include <iostream>

USTC_CG_NAMESPACE_OPEN_SCOPE

namespace cuda {

class CUDAGraph : public nvrhi::RefCounter<ICUDAGraph> {
   public:
    CUDAGraph(const CUDAGraphDesc& desc) : desc_(desc), capturing_(false)
    {
        CUDA_CHECK(cudaGraphCreate(&graph_, 0));
    }

    ~CUDAGraph()
    {
        if (graph_) {
            cudaGraphDestroy(graph_);
        }
    }

    const CUDAGraphDesc& getDesc() const override
    {
        return desc_;
    }
    bool beginCapture() override
    {
        if (capturing_) {
            std::cerr << "Graph is already capturing!" << std::endl;
            return false;
        }

        cudaStream_t stream = desc_.stream;
        if (!stream) {
            CUDA_CHECK(cudaStreamCreate(&stream));
            // Need to create a mutable copy to modify
            const_cast<CUDAGraphDesc&>(desc_).stream = stream;
        }

        cudaError_t result =
            cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal);
        if (result != cudaSuccess) {
            std::cerr << "Failed to begin capture: "
                      << cudaGetErrorString(result) << std::endl;
            return false;
        }

        capturing_ = true;
        return true;
    }

    CUDAGraphExecHandle endCapture() override;

    bool isCapturing() const override
    {
        return capturing_;
    }

    cudaGraph_t getCudaGraph() const override
    {
        return graph_;
    }

   private:
    CUDAGraphDesc desc_;
    cudaGraph_t graph_;
    bool capturing_;
};

class CUDAGraphExec : public nvrhi::RefCounter<ICUDAGraphExec> {
   public:
    CUDAGraphExec(cudaGraphExec_t exec, cudaStream_t stream)
        : exec_(exec),
          stream_(stream)
    {
    }

    ~CUDAGraphExec()
    {
        if (exec_) {
            cudaGraphExecDestroy(exec_);
        }
    }

    bool launch(cudaStream_t stream = nullptr) override
    {
        cudaStream_t launchStream = stream ? stream : stream_;

        cudaError_t result = cudaGraphLaunch(exec_, launchStream);
        if (result != cudaSuccess) {
            std::cerr << "Failed to launch graph: "
                      << cudaGetErrorString(result) << std::endl;
            return false;
        }

        return true;
    }
    bool update(CUDAGraphHandle newGraph) override
    {
        cudaGraphExecUpdateResultInfo resultInfo;

        cudaError_t result =
            cudaGraphExecUpdate(exec_, newGraph->getCudaGraph(), &resultInfo);

        if (result != cudaSuccess ||
            resultInfo.result != cudaGraphExecUpdateSuccess) {
            std::cerr << "Failed to update graph" << std::endl;
            return false;
        }

        return true;
    }

    cudaGraphExec_t getCudaGraphExec() const override
    {
        return exec_;
    }

   private:
    cudaGraphExec_t exec_;
    cudaStream_t stream_;
};

CUDAGraphExecHandle CUDAGraph::endCapture()
{
    if (!capturing_) {
        std::cerr << "Graph is not capturing!" << std::endl;
        return nullptr;
    }

    cudaGraph_t capturedGraph;
    cudaError_t result = cudaStreamEndCapture(desc_.stream, &capturedGraph);
    if (result != cudaSuccess) {
        std::cerr << "Failed to end capture: " << cudaGetErrorString(result)
                  << std::endl;
        return nullptr;
    }

    capturing_ = false;

    // Create executable graph
    cudaGraphExec_t graphExec;
    result =
        cudaGraphInstantiate(&graphExec, capturedGraph, nullptr, nullptr, 0);
    if (result != cudaSuccess) {
        std::cerr << "Failed to instantiate graph: "
                  << cudaGetErrorString(result) << std::endl;
        cudaGraphDestroy(capturedGraph);
        return nullptr;
    }

    // Copy the captured graph to our internal graph
    cudaGraphDestroy(graph_);
    graph_ = capturedGraph;

    return nvrhi::RefCountPtr<ICUDAGraphExec>(
        new CUDAGraphExec(graphExec, desc_.stream));
}

// API implementations
CUDAGraphHandle create_cuda_graph(const CUDAGraphDesc& desc)
{
    return nvrhi::RefCountPtr<ICUDAGraph>(new CUDAGraph(desc));
}

CUDAGraphHandle create_cuda_graph(cudaStream_t stream)
{
    CUDAGraphDesc desc(stream);
    return create_cuda_graph(desc);
}

}  // namespace cuda

USTC_CG_NAMESPACE_CLOSE_SCOPE

#endif  // USTC_CG_WITH_CUDA
