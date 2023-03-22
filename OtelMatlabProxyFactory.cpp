// Copyright 2023 The MathWorks, Inc.

#include "OtelMatlabProxyFactory.h"

#include "opentelemetry-matlab/trace/TracerProviderProxy.h"
#include "opentelemetry-matlab/trace/TracerProxy.h"
#include "opentelemetry-matlab/trace/SpanProxy.h"
//#include "opentelemetry-matlab/trace/ScopeProxy.h"
#include "opentelemetry-matlab/trace/SpanContextProxy.h"
#include "opentelemetry-matlab/sdk/trace/TracerProviderProxy.h"
#include "opentelemetry-matlab/sdk/trace/SimpleSpanProcessorProxy.h"
#include "opentelemetry-matlab/sdk/trace/BatchSpanProcessorProxy.h"
#include "opentelemetry-matlab/sdk/trace/AlwaysOnSamplerProxy.h"
#include "opentelemetry-matlab/sdk/trace/AlwaysOffSamplerProxy.h"
#include "opentelemetry-matlab/sdk/trace/TraceIdRatioBasedSamplerProxy.h"
#include "opentelemetry-matlab/sdk/trace/ParentBasedSamplerProxy.h"
#include "opentelemetry-matlab/exporters/otlp/OtlpHttpSpanExporterProxy.h"

std::shared_ptr<libmexclass::proxy::Proxy>
OtelMatlabProxyFactory::make_proxy(const libmexclass::proxy::ClassName& class_name,
                               const libmexclass::proxy::FunctionArguments& constructor_arguments) {

    REGISTER_PROXY(libmexclass.opentelemetry.TracerProviderProxy, libmexclass::opentelemetry::TracerProviderProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.TracerProxy, libmexclass::opentelemetry::TracerProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.SpanProxy, libmexclass::opentelemetry::SpanProxy);
    //REGISTER_PROXY(libmexclass.opentelemetry.ScopeProxy, libmexclass::opentelemetry::ScopeProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.SpanContextProxy, libmexclass::opentelemetry::SpanContextProxy);

    REGISTER_PROXY(libmexclass.opentelemetry.sdk.TracerProviderProxy, libmexclass::opentelemetry::sdk::TracerProviderProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.sdk.SimpleSpanProcessorProxy, libmexclass::opentelemetry::sdk::SimpleSpanProcessorProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.sdk.BatchSpanProcessorProxy, libmexclass::opentelemetry::sdk::BatchSpanProcessorProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.sdk.AlwaysOnSamplerProxy, libmexclass::opentelemetry::sdk::AlwaysOnSamplerProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.sdk.AlwaysOffSamplerProxy, libmexclass::opentelemetry::sdk::AlwaysOffSamplerProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.sdk.TraceIdRatioBasedSamplerProxy, libmexclass::opentelemetry::sdk::TraceIdRatioBasedSamplerProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.sdk.ParentBasedSamplerProxy, libmexclass::opentelemetry::sdk::ParentBasedSamplerProxy);
    REGISTER_PROXY(libmexclass.opentelemetry.exporters.OtlpHttpSpanExporterProxy, libmexclass::opentelemetry::exporters::OtlpHttpSpanExporterProxy);
    return nullptr;
}
