////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     SimpleConvolutionNode.cpp (nodes)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "SimpleConvolutionNode.h"
#include "ConstantNode.h"

// dsp
#include "Convolution.h"

// math
#include "Matrix.h"

namespace ell
{
namespace nodes
{
    namespace
    {
        using namespace ::ell::emitters;
        using namespace ::ell::model;

        //
        // Low-level code-generation
        //
        template <typename ValueType>
        void EmitSimpleConvolutionCode(IRFunctionEmitter& function, llvm::Value* input, llvm::Value* filterWeights, const PortMemoryLayout& inputLayout, const PortMemoryLayout& outputLayout, int filterSize, int stride, llvm::Value* result)
        {
            // input is a d x (w+2p) x (h+2p) array
            // reshaped, it's a d*(w+2p)) x (h+2p) array == d*(w+k-1) x (h+k-1)

            // filterWeights is f x k x k x d array
            // reshaped, it's (f*k) x (k*d) or f x k x (k*d)

            // output is a (w+2p) x (h+2p) x f array

            // Model parameters
            const auto inputPadding = inputLayout.GetOffset(0);
            DEBUG_USED(inputPadding);
            assert((inputPadding == filterSize / 2) && "Input padding must be filterSize/2");

            auto inputMemoryIncrements = inputLayout.GetCumulativeIncrement();

            // For each filter
            const auto numFilters = outputLayout.GetActiveSize(2);
            function.ParallelFor(numFilters, { input, filterWeights, result }, [inputLayout, outputLayout, inputMemoryIncrements, filterSize, stride](IRFunctionEmitter& function, IRLocalScalar filterIndex, const std::vector<llvm::Value*>& capturedValues) {
                auto input = capturedValues[0];
                auto filterWeights = capturedValues[1];
                auto result = capturedValues[2];
                auto outputTensor = function.LocalTensor(result, outputLayout.GetStride().ToVector(), RowMajorTensorLayout);

                // For each output row
                const auto outputRows = outputLayout.GetActiveSize(0);
                function.For(outputRows, [filterIndex, input, filterWeights, inputLayout, outputLayout, inputMemoryIncrements, outputTensor, filterSize, stride](IRFunctionEmitter& function, llvm::Value* loopIndex2) {
                    auto outputRow = function.LocalScalar(loopIndex2);

                    // For each output column
                    const auto outputColumns = outputLayout.GetActiveSize(1);
                    function.For(outputColumns, [outputRow, filterIndex, input, filterWeights, inputLayout, inputMemoryIncrements, outputTensor, filterSize, stride](IRFunctionEmitter& function, llvm::Value* loopIndex3) {
                        auto outputColumn = function.LocalScalar(loopIndex3);

                        const bool canCombineColumns = (inputLayout.GetActiveSize(1) == inputLayout.GetStride(1)) && (stride == 1);
                        const auto inputDepth = inputLayout.GetActiveSize(2);

                        // The filters are typically small, so we unroll the loops here
                        auto val = function.LocalScalar(ValueType{0});
                        for (int windowRow = 0; windowRow < filterSize; ++windowRow)
                        {
                            // Note: if the memory storage from consecutive columns is contiguous, we can process them together and avoid a loop
                            if (canCombineColumns)
                            {
                                auto inputOffset = ((outputRow + windowRow) * inputMemoryIncrements[0]) +
                                                   (outputColumn * inputMemoryIncrements[1]);
                                auto imageRow = function.PointerOffset(input, inputOffset);
                                auto filterOffset = inputDepth * (filterSize * windowRow) +
                                                    filterIndex * (filterSize * filterSize * inputDepth);
                                auto filterRow = function.PointerOffset(filterWeights, filterOffset);
                                val = val + function.DotProduct(filterSize * inputDepth, imageRow, filterRow);
                            }
                            else
                            {
                                for (int windowColumn = 0; windowColumn < filterSize; ++windowColumn)
                                {
                                    // I[r+wc, c+wc]
                                    auto inputRow = outputRow * stride;
                                    auto inputColumn = outputColumn * stride;
                                    auto inputOffset = ((inputRow + windowRow) * inputMemoryIncrements[0]) +
                                                       ((inputColumn + windowColumn) * inputMemoryIncrements[1]);
                                    auto imageRow = function.PointerOffset(input, inputOffset);
                                    auto filterOffset = inputDepth * (filterSize * windowRow + windowColumn) +
                                                        filterIndex * (filterSize * filterSize * inputDepth);
                                    auto filterRow = function.PointerOffset(filterWeights, filterOffset);
                                    val = val + function.DotProduct(inputDepth, imageRow, filterRow);
                                }
                            }
                            outputTensor({ outputRow, outputColumn, filterIndex }) = val;
                        }
                    }); // End outputColumns loop
                }); // End outputRows loop
            }); // End numFilters loop
        }

        template<typename ValueType>
        void EmitSimpleDepthwiseSeparableConvolutionCode(IRFunctionEmitter& function, llvm::Value* input, llvm::Value* filterWeights, const PortMemoryLayout& inputLayout, const PortMemoryLayout& outputLayout, int filterSize, int stride, llvm::Value* result)
        {
            const auto inputDepth = inputLayout.GetActiveSize(2);
            const auto inputPadding = inputLayout.GetOffset(0);
            DEBUG_USED(inputPadding, inputDepth);
            assert((inputPadding == filterSize / 2) && "Input padding must be filterSize/2");

            // output data parameters
            // const auto outputRows = outputLayout.GetActiveSize(0);
            // const auto outputColumns = outputLayout.GetActiveSize(1);
            const auto numFilters = outputLayout.GetActiveSize(2);
            assert(numFilters == inputDepth);
            DEBUG_USED(numFilters);

            // For each filter
            // For each output row
            const auto outputRows = outputLayout.GetActiveSize(0);
            function.ParallelFor(outputRows, { input, filterWeights, result }, [inputLayout, outputLayout, filterSize, stride](IRFunctionEmitter& function, auto outputRow, const std::vector<llvm::Value*>& capturedValues) {
                auto input = capturedValues[0];
                auto filterWeights = capturedValues[1];
                auto result = capturedValues[2];

                auto inputTensor = function.LocalTensor(input, inputLayout.GetStride().ToVector(), RowMajorTensorLayout);
                auto outputTensor = function.LocalTensor(result, outputLayout.GetStride().ToVector(), RowMajorTensorLayout);
                auto filter = function.LocalMultidimArray(filterWeights, { inputLayout.GetStride(2), filterSize, filterSize});

                // For each output column
                const auto outputColumns = outputLayout.GetActiveSize(1);
                function.For(outputColumns, [outputLayout, outputRow, inputTensor, filter, outputTensor, filterSize, stride](IRFunctionEmitter& function, auto outputColumn) {
                    // For each filter
                    const auto numFilters = outputLayout.GetActiveSize(2);
                    function.For(numFilters, [outputRow, outputColumn, inputTensor, filter, outputTensor, filterSize, stride](IRFunctionEmitter& function, auto filterIndex) {
                        // The filters are typically small, so we unroll the loops here
                        auto val = function.LocalScalar(ValueType{0});
                        for (int windowRow = 0; windowRow < filterSize; ++windowRow)
                        {
                            for (int windowColumn = 0; windowColumn < filterSize; ++windowColumn)
                            {
                                auto inputRow = outputRow * stride;
                                auto inputColumn = outputColumn * stride;

                                auto filterRow = function.LocalScalar(windowRow);
                                auto filterColumn = function.LocalScalar(windowColumn);

                                auto inputVal = inputTensor({ inputRow + windowRow, inputColumn + windowColumn, filterIndex });
                                auto filterVal = filter({ filterIndex, filterRow, filterColumn });

                                val += inputVal * filterVal;
                            }
                        }
                        outputTensor({ outputRow, outputColumn, filterIndex }) = val;
                    });
                });
            });
        }
    } // end anonymous namespace

    //
    // SimpleConvolutionNode
    //

    template<typename ValueType>
    SimpleConvolutionNode<ValueType>::SimpleConvolutionNode()
        : CompilableNode({ &_input }, { &_output }), _input(this, {}, defaultInputPortName), _output(this, defaultOutputPortName, 0)
    {
    }

    template<typename ValueType>
    SimpleConvolutionNode<ValueType>::SimpleConvolutionNode(const model::PortElements<ValueType>& input,
                                                            const model::PortMemoryLayout& inputMemoryLayout,
                                                            const model::PortMemoryLayout& outputMemoryLayout,
                                                            const ConstTensorReferenceType& filterWeights,
                                                            size_t stride)
        : CompilableNode({ &_input }, { &_output }), _input(this, input, defaultInputPortName), _output(this, defaultOutputPortName, outputMemoryLayout), _inputMemoryLayout(inputMemoryLayout), _filterWeights(filterWeights), _stride(static_cast<int>(stride))
    {
        _isDepthwiseSeparable = (filterWeights.NumChannels() == 1) && (inputMemoryLayout.GetActiveSize()[2] > 1);
    }

    template<typename ValueType>
    void SimpleConvolutionNode<ValueType>::Copy(model::ModelTransformer& transformer) const
    {
        auto newInput = transformer.TransformPortElements(_input.GetPortElements());
        auto newNode = transformer.AddNode<SimpleConvolutionNode<ValueType>>(newInput, _inputMemoryLayout, GetOutputMemoryLayout(), _filterWeights, _stride);
        transformer.MapNodeOutput(this->output, newNode->output);
    }

    template<typename ValueType>
    bool SimpleConvolutionNode<ValueType>::Refine(model::ModelTransformer& transformer) const
    {
        auto newInput = transformer.TransformPortElements(this->input.GetPortElements());

        // (row, column), channel order:
        const auto& weightsMatrix = _filterWeights.ReferenceAsMatrix();
        const auto weightsValues = weightsMatrix.ToArray();
        const int filterSize = _filterWeights.NumColumns();
        auto weightsNode = transformer.AddNode<ConstantNode<ValueType>>(weightsValues);
        auto convNode = transformer.AddNode<SimpleConvolutionComputeNode<ValueType>>(newInput, weightsNode->output, _inputMemoryLayout, GetOutputMemoryLayout(), filterSize, _stride, _isDepthwiseSeparable);
        transformer.MapNodeOutput(this->output, convNode->output);
        return true;
    }

    template<typename ValueType>
    void SimpleConvolutionNode<ValueType>::Compute() const
    {
        throw utilities::LogicException(utilities::LogicExceptionErrors::notImplemented);
    }

    template<typename ValueType>
    void SimpleConvolutionNode<ValueType>::WriteToArchive(utilities::Archiver& archiver) const
    {
        model::CompilableNode::WriteToArchive(archiver);
        archiver[defaultInputPortName] << _input;
        archiver["inputLayout"] << _inputMemoryLayout;
        archiver["outputLayout"] << GetOutputMemoryLayout();
        archiver["stride"] << _stride;
        math::TensorArchiver::Write(_filterWeights, "weights", archiver);
    }

    template<typename ValueType>
    void SimpleConvolutionNode<ValueType>::ReadFromArchive(utilities::Unarchiver& archiver)
    {
        model::CompilableNode::ReadFromArchive(archiver);
        archiver[defaultInputPortName] >> _input;
        archiver["inputLayout"] >> _inputMemoryLayout;
        model::PortMemoryLayout outputMemoryLayout;
        archiver["outputLayout"] >> outputMemoryLayout;
        _output.SetMemoryLayout(outputMemoryLayout);
        archiver["stride"] >> _stride;
        math::TensorArchiver::Read(_filterWeights, "weights", archiver);

        _isDepthwiseSeparable = (_filterWeights.NumChannels() == 1) && (_inputMemoryLayout.GetActiveSize()[2] > 1);
    }

    //
    // SimpleConvolutionComputeNode
    //

    template<typename ValueType>
    SimpleConvolutionComputeNode<ValueType>::SimpleConvolutionComputeNode()
        : CompilableNode({ &_input }, { &_output }), _input(this, {}, defaultInputPortName), _filterWeights(this, {}, filterWeightsPortName), _output(this, defaultOutputPortName, 0)
    {
    }

    template<typename ValueType>
    SimpleConvolutionComputeNode<ValueType>::SimpleConvolutionComputeNode(const model::PortElements<ValueType>& input,
                                                                          const model::PortElements<ValueType>& filterWeights,
                                                                          const model::PortMemoryLayout& inputMemoryLayout,
                                                                          const model::PortMemoryLayout& outputMemoryLayout,
                                                                          int filterSize,
                                                                          int stride,
                                                                          bool isDepthwiseSeparable)
        : CompilableNode({ &_input, &_filterWeights }, { &_output }), _input(this, input, defaultInputPortName), _filterWeights(this, filterWeights, filterWeightsPortName), _output(this, defaultOutputPortName, outputMemoryLayout), _inputMemoryLayout(inputMemoryLayout), _filterSize(filterSize), _stride(stride), _isDepthwiseSeparable(isDepthwiseSeparable)
    {
    }

    template<typename ValueType>
    void SimpleConvolutionComputeNode<ValueType>::Copy(model::ModelTransformer& transformer) const
    {
        auto newInput = transformer.TransformPortElements(_input.GetPortElements());
        auto newFilterWeights = transformer.TransformPortElements(_filterWeights.GetPortElements());
        auto newNode = transformer.AddNode<SimpleConvolutionComputeNode<ValueType>>(newInput, newFilterWeights, _inputMemoryLayout, GetOutputMemoryLayout(), _filterSize, _stride, _isDepthwiseSeparable);
        transformer.MapNodeOutput(this->output, newNode->output);
    }

    template<typename ValueType>
    void SimpleConvolutionComputeNode<ValueType>::Compute() const
    {
        throw utilities::LogicException(utilities::LogicExceptionErrors::notImplemented);
    }

    // Terminology:
    // fw: filter width
    // d: # input channels
    // f: # filters (== output channels)

    template<typename ValueType>
    void SimpleConvolutionComputeNode<ValueType>::Compile(model::IRMapCompiler& compiler, emitters::IRFunctionEmitter& function)
    {
        // input is a d x (w+2p) x (h+2p) array
        // reshaped, it's a d*(w+2p)) x (h+2p) array == d*(w+k-1) x (h+k-1)
        llvm::Value* pInput = compiler.EnsurePortEmitted(this->input);

        // weights is f x k x k x d array
        // reshaped, it's (f*k) x (k*d) or f x k x (k*d)
        llvm::Value* pWeights = compiler.EnsurePortEmitted(this->filterWeights);

        // output is a (w+2p) x (h+2p) x f array
        llvm::Value* pOutput = compiler.EnsurePortEmitted(this->output);

        // Model parameters
        const auto inputLayout = this->GetInputMemoryLayout();
        const auto outputLayout = this->GetOutputMemoryLayout();
        const auto inputPadding = inputLayout.GetOffset(0);
        DEBUG_USED(inputPadding);
        assert((inputPadding == _filterSize / 2) && "Input padding must be filterSize/2");

        if (!_isDepthwiseSeparable)
        {
            EmitSimpleConvolutionCode<ValueType>(function, pInput, pWeights, inputLayout, outputLayout, _filterSize, _stride, pOutput);
        }
        else
        {
            EmitSimpleDepthwiseSeparableConvolutionCode<ValueType>(function, pInput, pWeights, inputLayout, outputLayout, _filterSize, _stride, pOutput);
        }
    }

    // Explicit specializations
    template class SimpleConvolutionNode<float>;
    template class SimpleConvolutionNode<double>;
} // nodes
} // ell
