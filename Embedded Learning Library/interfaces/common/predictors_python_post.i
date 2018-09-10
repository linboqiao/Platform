// Additional C++ code to make Api more natural for Python callers
namespace ell
{
namespace api
{
namespace predictors
{
namespace neural
{

    %extend LayerParameters
    {
        LayerParameters(const LayerShape& inputShape,
                        const PaddingParameters& inputPaddingParameters,
                        const LayerShape& outputShape,
                        const PaddingParameters& outputPaddingParameters)

        {
            return new ell::api::predictors::neural::LayerParameters{inputShape, inputPaddingParameters, outputShape, outputPaddingParameters};
        }
    };

}
}
}
}

namespace ell
{
namespace predictors
{
namespace neural
{

    %extend PaddingParameters
    {
        PaddingParameters(PaddingScheme paddingScheme, size_t paddingSize)
        {
            return new ell::predictors::neural::PaddingParameters{paddingScheme, paddingSize};
        }
    };

    %extend BinaryConvolutionalParameters
    {
        BinaryConvolutionalParameters(size_t receptiveField, size_t stride, BinaryConvolutionMethod binaryConvolutionMethod, BinaryWeightsScale weightScale)
        {
            return new ell::predictors::neural::BinaryConvolutionalParameters{receptiveField, stride, binaryConvolutionMethod, weightScale};
        }
    };

    %extend ConvolutionalParameters
    {
        ConvolutionalParameters(size_t receptiveField, size_t stride, ConvolutionMethod convolutionMethod, size_t numFiltersAtATime)
        {
            return new ell::predictors::neural::ConvolutionalParameters{receptiveField, stride, convolutionMethod, numFiltersAtATime};
        }
    };

    %extend PoolingParameters
    {
        PoolingParameters(size_t poolingSize, size_t stride)
        {
            return new ell::predictors::neural::PoolingParameters{poolingSize, stride};
        }
    };

    %extend RegionDetectionParameters
    {
        RegionDetectionParameters(int width, int height, int numBoxesPerCell, int numClasses, int numAnchors, bool applySoftmax)
        {
            return new ell::predictors::neural::RegionDetectionParameters{width, height, numBoxesPerCell, numClasses, numAnchors, applySoftmax};
        }
    };
}
}
}


// Additional Python code to make the Api more natural for Python callers
%pythoncode %{

# Python friendly class for PaddingScheme
class PaddingScheme:
    zeros = PaddingScheme_zeros
    minusOnes = PaddingScheme_minusOnes
    alternatingZeroAndOnes = PaddingScheme_alternatingZeroAndOnes
    randomZeroAndOnes = PaddingScheme_randomZeroAndOnes
    min = PaddingScheme_min
    max = PaddingScheme_max

# Remove flat defines so callers only see the class above
del PaddingScheme_zeros
del PaddingScheme_minusOnes
del PaddingScheme_alternatingZeroAndOnes
del PaddingScheme_randomZeroAndOnes
del PaddingScheme_min
del PaddingScheme_max

# Python friendly class for LayerType
class LayerType:
    activation = LayerType_activation
    base = LayerType_base
    batchNormalization = LayerType_batchNormalization
    bias = LayerType_bias
    binaryConvolution = LayerType_binaryConvolution
    convolution = LayerType_convolution
    fullyConnected = LayerType_fullyConnected
    gru = LayerType_gru
    input = LayerType_input
    lstm = LayerType_lstm
    pooling = LayerType_pooling
    recurrent = LayerType_recurrent
    region = LayerType_region
    scaling = LayerType_scaling

# Remove flat defines so callers only see the class above
del LayerType_activation
del LayerType_base
del LayerType_batchNormalization
del LayerType_bias
del LayerType_binaryConvolution
del LayerType_convolution
del LayerType_fullyConnected
del LayerType_gru
del LayerType_input
del LayerType_lstm
del LayerType_pooling
del LayerType_recurrent
del LayerType_region
del LayerType_scaling

# Python friendly class for ActivationType
class ActivationType:
    relu = ActivationType_relu
    leaky = ActivationType_leaky
    sigmoid = ActivationType_sigmoid
    tanh = ActivationType_tanh
    prelu = ActivationType_prelu
    hardSigmoid = ActivationType_hardSigmoid

# Remove flat defines so callers only see the class above
del ActivationType_relu
del ActivationType_leaky
del ActivationType_sigmoid
del ActivationType_tanh
del ActivationType_prelu
del ActivationType_hardSigmoid

# Python friendly class for PoolingType
class PoolingType:
    max = PoolingType_max
    mean = PoolingType_mean

# Remove flat defines so callers only see the class above
del PoolingType_max
del PoolingType_mean

# Python friendly class for BinaryConvolutionMethod
class BinaryConvolutionMethod:
    gemm = BinaryConvolutionMethod_gemm
    bitwise = BinaryConvolutionMethod_bitwise

# Remove flat defines so callers only see the class above
del BinaryConvolutionMethod_gemm
del BinaryConvolutionMethod_bitwise

# Python friendly class for BinaryWeightsScale
class BinaryWeightsScale:
    none = BinaryWeightsScale_none
    mean = BinaryWeightsScale_mean

# Remove flat defines so callers only see the class above
del BinaryWeightsScale_none
del BinaryWeightsScale_mean

# Python friendly class for ConvolutionMethod
class ConvolutionMethod:
    automatic = ConvolutionMethod_automatic
    diagonal = ConvolutionMethod_diagonal
    simple = ConvolutionMethod_simple
    winograd = ConvolutionMethod_winograd
    unrolled = ConvolutionMethod_unrolled

# Remove flat defines so callers only see the class above
del ConvolutionMethod_automatic
del ConvolutionMethod_diagonal
del ConvolutionMethod_simple
del ConvolutionMethod_winograd
del ConvolutionMethod_unrolled

# Python friendly class for EpsilonSummand
class EpsilonSummand:
    sqrtVariance = EpsilonSummand_sqrtVariance
    variance = EpsilonSummand_variance

del EpsilonSummand_sqrtVariance
del EpsilonSummand_variance

%}
