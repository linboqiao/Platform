////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     ModelTestUtilities.h (compile_test)
//  Authors:  Umesh Madan, Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// math
#include "Tensor.h"

// model
#include "IRCompiledMap.h"
#include "IRDiagnosticHandler.h"
#include "Map.h"
#include "Port.h"

// testing
#include "testing.h"

// stl
#include <iostream>
#include <string>
#include <vector>

using namespace ell;

// RAII class for setting and restoring verbosity
class VerboseRegion
{
public:
    VerboseRegion(bool verbose = true);
    ~VerboseRegion();

private:
    bool _oldVerbose;
};

void SetVerbose(bool verbose);
bool IsVerbose();

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& v);

void PrintMap(const model::Map& map);

void PrintModel(const model::Model& model);
void PrintModel(const model::Model& model, const model::Node* output);

void PrintHeader(emitters::IRModuleEmitter& module);
void PrintHeader(model::IRCompiledMap& compiledMap);

void PrintIR(emitters::IRModuleEmitter& module);
void PrintIR(model::IRCompiledMap& compiledMap);

model::Model GetSimpleModel();
model::Model GetComplexModel();

template <typename InputType, typename OutputType>
void PrintCompiledOutput(const model::Map& map, const model::IRCompiledMap& compiledMap, const std::vector<std::vector<InputType>>& signal, const std::string& name);

template <typename InputType>
void PrintCompiledOutput(const model::Map& map, const model::IRCompiledMap& compiledMap, const std::vector<std::vector<InputType>>& signal, const std::string& name);

template <typename InputType, typename OutputType>
void VerifyCompiledOutput(const model::Map& map, const model::IRCompiledMap& compiledMap, const std::vector<std::vector<InputType>>& signal, const std::string& name, double epsilon = 1e-5);

template <typename InputType>
void VerifyCompiledOutput(const model::Map& map, const model::IRCompiledMap& compiledMap, const std::vector<std::vector<InputType>>& signal, const std::string& name, double epsilon = 1e-5);

template <typename InputType, typename OutputType>
void VerifyMapOutput(const model::Map& map, std::vector<std::vector<InputType>>& signal, std::vector<std::vector<OutputType>>& expectedOutput, const std::string& name);

void PrintDiagnostics(emitters::IRDiagnosticHandler& handler);

size_t GetShapeSize(const math::IntegerTriplet& shape);

template <typename ElementType>
void FillRandomVector(std::vector<ElementType>& vector, ElementType min = -1, ElementType max = 1);

template <typename ElementType>
std::vector<ElementType> GetRandomVector(size_t size, ElementType min = -1, ElementType max = 1);

template <typename ElementType>
void FillRandomVector(ell::math::ColumnVector<ElementType>& vector, ElementType min = -1, ElementType max = 1);

template <typename ElementType>
void FillRandomTensor(ell::math::ChannelColumnRowTensor<ElementType>& tensor, ElementType min = -1, ElementType max = 1);

template <typename ElementType>
void FillVector(std::vector<ElementType>& vector, ElementType startValue = 0, ElementType step = 1);

template <typename ElementType>
void FillVector(ell::math::ColumnVector<ElementType>& vector, ElementType startValue = 0, ElementType step = 1);

template <typename ElementType>
void FillTensor(ell::math::ChannelColumnRowTensor<ElementType>& tensor, ElementType startValue = 0, ElementType step = 1);

template <typename ElementType>
void FillTensor(math::TensorReference<ElementType, math::Dimension::channel, math::Dimension::column, math::Dimension::row>& tensor, ElementType startValue = 0, ElementType step = 1);

template <typename ElementType>
void FillWeightsTensor(ell::math::ChannelColumnRowTensor<ElementType>& tensor, ElementType startValue = 0, ElementType step = 1);

template <typename ElementType>
void FillMatrix(math::RowMatrix<ElementType>& matrix, ElementType startValue = 0, ElementType step = 1);

#include "../tcc/ModelTestUtilities.tcc"
