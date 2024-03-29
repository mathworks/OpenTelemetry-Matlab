// Copyright 2023 The MathWorks, Inc.

#pragma once

#include "opentelemetry-matlab/sdk/trace/SamplerProxy.h"

#include "libmexclass/proxy/Proxy.h"
#include "libmexclass/proxy/method/Context.h"

#include "opentelemetry/sdk/trace/samplers/trace_id_ratio_factory.h"

namespace trace_sdk = opentelemetry::sdk::trace;

namespace libmexclass::opentelemetry::sdk {
class TraceIdRatioBasedSamplerProxy : public SamplerProxy {
  public:
    TraceIdRatioBasedSamplerProxy(double ratio) : Ratio(ratio) {}

    static libmexclass::proxy::MakeResult make(const libmexclass::proxy::FunctionArguments& constructor_arguments) {
        matlab::data::TypedArray<double> ratio_mda = constructor_arguments[0];
	return std::make_shared<TraceIdRatioBasedSamplerProxy>(ratio_mda[0]);
    }

    std::unique_ptr<trace_sdk::Sampler> getInstance() override {
        return trace_sdk::TraceIdRatioBasedSamplerFactory::Create(Ratio);
    }

  private:
    double Ratio;
};
} // namespace libmexclass::opentelemetry
