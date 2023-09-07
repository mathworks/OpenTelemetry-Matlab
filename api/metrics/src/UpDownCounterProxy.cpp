// Copyright 2023 The MathWorks, Inc.

#include "opentelemetry-matlab/metrics/UpDownCounterProxy.h"

#include "libmexclass/proxy/ProxyManager.h"


#include "MatlabDataArray.hpp"

#include <chrono>

namespace libmexclass::opentelemetry {


void UpDownCounterProxy::add(libmexclass::proxy::method::Context& context){
  
    matlab::data::Array value_mda = context.inputs[0];
    double value = static_cast<double>(value_mda[0]);
    size_t nin = context.inputs.getNumberOfElements();
    if (nin == 1){
        CppUpDownCounter->Add(value);
    } 
    // add attributes
    else { 
        ProcessedAttributes attrs;
        matlab::data::StringArray attrnames_mda = context.inputs[1];
        matlab::data::Array attrvalues_mda = context.inputs[2];
        size_t nattrs = attrnames_mda.getNumberOfElements();
        for (size_t i = 0; i < nattrs; i ++){
            std::string attrname = static_cast<std::string>(attrnames_mda[i]);
            matlab::data::Array attrvalue = attrvalues_mda[i];
            processAttribute(attrname, attrvalue, attrs);
        }
        CppUpDownCounter->Add(value, attrs.Attributes);
    }
    
}



} // namespace libmexclass::opentelemetry
