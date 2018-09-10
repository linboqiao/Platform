////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     ReorderDataNode.h (nodes)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// model
#include "CompilableNode.h"
#include "IRMapCompiler.h"
#include "Model.h"
#include "ModelTransformer.h"
#include "Node.h"
#include "Port.h"
#include "PortMemoryLayout.h"

// utilities
#include "ArchiveVersion.h"
#include "IArchivable.h"

// stl
#include <algorithm>
#include <array>
#include <numeric>
#include <string>
#include <vector>

namespace ell
{
namespace nodes
{
    /// <summary> A node that can reorder dimensions (transpose) and add or remove padding </summary>
    template <typename ValueType>
    class ReorderDataNode : public model::CompilableNode
    {
    public:
        /// @name Input and Output Ports
        /// @{
        const model::InputPort<ValueType>& input = _input;
        const model::OutputPort<ValueType>& output = _output;
        /// @}

        /// <summary> Default constructor. </summary>
        ReorderDataNode();

        /// <summary> Constructor with no reordering </summary>
        ///
        /// <param name="input"> The input to reorder. </param>
        /// <param name="outputMemoryLayout"> The memory layout of the output. Data will be copied into the "active" area, and the rest will be zeroed out. </param>
        ReorderDataNode(const model::PortElements<ValueType>& input, const model::PortMemoryLayout& outputMemoryLayout, ValueType paddingValue = 0);

        /// <summary> Constructor with no reordering </summary>
        ///
        /// <param name="input"> The input to reorder. </param>
        /// <param name="inputMemoryLayout"> The memory layout of the input. Only data in the "active" area will be copied. </param>
        /// <param name="outputMemoryLayout"> The memory layout of the output. Data will be copied into the "active" area, and the rest will be zeroed out. </param>
        ReorderDataNode(const model::PortElements<ValueType>& input, const model::PortMemoryLayout& inputMemoryLayout, const model::PortMemoryLayout& outputMemoryLayout, ValueType paddingValue = 0);

        /// <summary> Constructor with reordering </summary>
        ///
        /// <param name="input"> The input to reorder. </param>
        /// <param name="order"> The permutation vector to apply to the dimensions when copying. Input dimension `i` will get copied to output dimension `order[i]`. If left empty, no reordering is done.
        //    For instance, to reorder the normal interleaved image order into a planar order, the `order` parameter would be
        ///   set to {2, 0, 1} --- reordering {row, column, channel} to {channel, row, column} </param>
        ReorderDataNode(const model::PortElements<ValueType>& input, const model::DimensionOrder& order);

        /// <summary> Constructor with reordering </summary>
        ///
        /// <param name="input"> The input to reorder. </param>
        /// <param name="outputMemoryLayout"> The memory layout of the output. Data will be copied into the "active" area, and the rest will be zeroed out. </param>
        /// <param name="order"> The permutation vector to apply to the dimensions when copying. Input dimension `i` will get copied to output dimension `order[i]`. If left empty, no reordering is done.
        ///    For instance, to reorder the normal interleaved image order into a planar order, the `order` parameter would be
        ///    set to {2, 0, 1} --- reordering {row, column, channel} to {channel, row, column} </param>
        /// <param name="paddingValue"> The value to use for output padding, if output shape is larger than input shape. </param>
        ReorderDataNode(const model::PortElements<ValueType>& input, const model::PortMemoryLayout& outputMemoryLayout, const model::DimensionOrder& order, ValueType paddingValue = 0);

        /// <summary> Constructor with reordering </summary>
        ///
        /// <param name="input"> The input to reorder. </param>
        /// <param name="inputMemoryLayout"> The memory layout of the input. Only data in the "active" area is guaranteed to be copied. </param>
        /// <param name="outputMemoryLayout"> The memory layout of the output. Data will be copied into the "active" area, and the rest will be zeroed out. </param>
        /// <param name="order"> The permutation vector to apply to the dimensions when copying. Input dimension `i` will get copied to output dimension `order[i]`. If left empty, no reordering is done.
        ///   For instance, to reorder the normal interleaved image order into a planar order, the `order` parameter would be
        ///   set to {2, 0, 1} --- reordering {row, column, channel} to {channel, row, column} </param>
        /// <param name="paddingValue"> The value to use for output padding, if output shape is larger than input shape. </param>
        ReorderDataNode(const model::PortElements<ValueType>& input, const model::PortMemoryLayout& inputMemoryLayout, const model::PortMemoryLayout& outputMemoryLayout, const model::DimensionOrder& order, ValueType paddingValue = 0);

        /// <summary> Gets information about the input memory layout </summary>
        const model::PortMemoryLayout& GetInputMemoryLayout() const { return _inputMemoryLayout; }

        /// <summary> Gets information about the input memory layout </summary>
        model::PortMemoryLayout GetOutputMemoryLayout() const { return _output.GetMemoryLayout(); }

        /// <summary> Returns padding value </summary>
        ///
        /// <returns> Padding value </returns>
        ValueType GetPaddingValue() const { return _paddingValue; }

        /// <summary> Returns true if the node can accept input with this memory layout order, else false </summary>
        ///
        /// <param name="order"> The memory layout order for all the input ports </summary>
        /// <returns> If the node can accept the input memory layout order, true, else false </returns>
        bool CanAcceptInputLayout(const utilities::DimensionOrder& order) const override
        {
            return GetInputMemoryLayout().GetLogicalDimensionOrder() == order;
        }

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        static std::string GetTypeName() { return utilities::GetCompositeTypeName<ValueType>("ReorderDataNode"); }

        /// <summary> Gets the name of this type (for serialization). </summary>
        ///
        /// <returns> The name of this type. </returns>
        std::string GetRuntimeTypeName() const override { return GetTypeName(); }

    protected:
        model::MemoryCoordinates ReorderOutputToInputLocation(model::MemoryCoordinates outputLocation) const;
        std::vector<emitters::IRLocalScalar> ReorderOutputToInputLocation(std::vector<emitters::IRLocalScalar> outputLocation) const;

        void Copy(model::ModelTransformer& transformer) const override;
        void Compute() const override;
        void Compile(model::IRMapCompiler& compiler, emitters::IRFunctionEmitter& function) override;

        utilities::ArchiveVersion GetArchiveVersion() const override;
        bool CanReadArchiveVersion(const utilities::ArchiveVersion& version) const override;
        void WriteToArchive(utilities::Archiver& archiver) const override;
        void ReadFromArchive(utilities::Unarchiver& archiver) override;
        bool HasState() const override { return true; } // stored state: inputMemoryLayout, paddingValue

    private:
        void ComputeDimensionLoop(const model::PortMemoryLayout& inputMemoryLayout, const model::PortMemoryLayout& outputMemoryLayout, int dimension, std::vector<int>& coordinates, std::vector<ValueType>& output) const;
        void CompileDimensionLoop(emitters::IRFunctionEmitter& function, emitters::IRLocalArray input, const model::PortMemoryLayout& inputMemoryLayout, emitters::IRLocalArray output, const model::PortMemoryLayout& outputMemoryLayout, int dimension, std::vector<emitters::IRLocalScalar>& coordinates) const;

        // Input
        model::InputPort<ValueType> _input;

        // Output
        model::OutputPort<ValueType> _output;

        model::PortMemoryLayout _inputMemoryLayout;

        ValueType _paddingValue;
    };
}
}

#include "../tcc/ReorderDataNode.tcc"
