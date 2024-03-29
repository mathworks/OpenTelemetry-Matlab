// Copyright 2023 The MathWorks, Inc.

#include "opentelemetry-matlab/context/propagation/TextMapCarrierProxy.h"

namespace libmexclass::opentelemetry {
libmexclass::proxy::MakeResult TextMapCarrierProxy::make(const libmexclass::proxy::FunctionArguments& constructor_arguments) {
    matlab::data::StringArray headers_mda = constructor_arguments[0];
    matlab::data::ArrayDimensions headers_size = headers_mda.getDimensions();
    size_t nheaders = headers_size[0];
    assert(headers_size[1] == 2);    // input should be Nx2
	
    HttpTextMapCarrier carrier;
    for (size_t i=0; i<nheaders; ++i) {
       carrier.Set(std::string(headers_mda[i][0]), std::string(headers_mda[i][1]));
    }

    return std::make_shared<TextMapCarrierProxy>(carrier);
}

void TextMapCarrierProxy::getHeaders(libmexclass::proxy::method::Context& context) {
    size_t nheaders = CppCarrier.Headers.size();

    // Allocate output array
    matlab::data::ArrayFactory factory;
    matlab::data::ArrayDimensions dims = {nheaders, 2};
    auto headers_mda = factory.createArray<matlab::data::MATLABString>(dims);

    // Populate output
    size_t i = 0;
    for (const auto& [key, value] : CppCarrier.Headers) {
      headers_mda[i][0] = key;
      headers_mda[i++][1] = value;
    }

    context.outputs[0] = headers_mda;

}
} // namespace libmexclass::opentelemetry
