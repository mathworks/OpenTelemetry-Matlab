cmake_minimum_required(VERSION 3.9.0)

cmake_policy(SET CMP0074 NEW)


# ###########################
# vcpkg
# ###########################

if(WIN32)
  add_compile_definitions(_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
endif()

include(FetchContent)

# check if VCPKG_ROOT is defined, which should point to an existing installation
if(DEFINED ENV{VCPKG_ROOT})
   # Autodetect vcpkg toolchain
   if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
       string(REPLACE "\\" "/" CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
   endif()
else()
   # VCPKG_ROOT not defined, get it using FetchContent

   if(DEFINED VCPKG_PREFIX})
       # download location specified
       string(REPLACE "\\" "/" VCPKG_PREFIX "${VCPKG_PREFIX}")
   else()
       set(VCPKG_PREFIX ${CMAKE_BINARY_DIR}/vcpkg)
   endif()

   # Define custom triplets for vcpkg
   if(APPLE)
       # On Mac, there is a conflict between libcurl and the version in MATLAB, so 
       # use libcurl as a shared library and load the MATLAB version at runtime
       # run uname -m to determine whether arm64 or x86_64
       exec_program(uname ARGS -m OUTPUT_VARIABLE MAC_HOST_SYSTEM)
       set(VCPKG_OTEL_TRIPLET ${MAC_HOST_SYSTEM}-osx-otel-matlab)
       set(VCPKG_OVERLAY_TRIPLETS ${CMAKE_SOURCE_DIR}/cmake/vcpkg_triplets)
       set(VCPKG_TARGET_TRIPLET ${VCPKG_OTEL_TRIPLET})
       set(TRIPLET_DEFINITIONS -DVCPKG_OVERLAY_TRIPLETS="${VCPKG_OVERLAY_TRIPLETS}" -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET})
   elseif(WIN32)
       # On Windows, there is a conflict between abseil_dll.dll and the version used by Simulink. 
       # The shared library doesn't seem ABI stable and different versions cannot be used interchangeably. 
       # To sidestep the problem, use static library.
       set(VCPKG_OVERLAY_TRIPLETS ${CMAKE_SOURCE_DIR}/cmake/vcpkg_triplets)
       set(VCPKG_TARGET_TRIPLET x64-windows-otel-matlab)
       set(TRIPLET_DEFINITIONS -DVCPKG_OVERLAY_TRIPLETS="${VCPKG_OVERLAY_TRIPLETS}" -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET})
   endif()

   set(VCPKG_FETCH_CONTENT_NAME vcpkg)
   set(VCPKG_GIT_REPOSITORY "https://github.com/microsoft/vcpkg.git")
   set(VCPKG_GIT_TAG "fba75d0")
   FetchContent_Declare(
       ${VCPKG_FETCH_CONTENT_NAME}
       GIT_REPOSITORY ${VCPKG_GIT_REPOSITORY}
       GIT_TAG ${VCPKG_GIT_TAG}
       PREFIX ${VCPKG_PREFIX}
       UPDATE_DISCONNECTED 1
   )

   FetchContent_MakeAvailable(
       ${VCPKG_FETCH_CONTENT_NAME}
   )

   FetchContent_GetProperties(${VCPKG_FETCH_CONTENT_NAME})
   string(REPLACE "\\" "/" CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake")

endif()

if(NOT DEFINED VCPKG_INSTALLED_DIR)
   set(VCPKG_INSTALLED_DIR ${CMAKE_BINARY_DIR}/vcpkg_installed)
endif()

# ######################################
# Options
# ######################################

option(WITH_OTLP_HTTP "Whether to include the OTLP HTTP exporter" ON)
option(WITH_OTLP_GRPC "Whether to include the OTLP gRPC exporter" OFF)
if(NOT WITH_OTLP_HTTP AND NOT WITH_OTLP_GRPC)
    message(FATAL_ERROR "At least one of WITH_OTLP_HTTP and WITH_OTLP_GRPC must be ON")
endif()
option(SKIP_OTEL_CPP_PATCH "Whether to skip patching OpenTelemetry-cpp" OFF)
option(WITH_EXAMPLES "Whether to build examples" OFF)

# set vcpkg features depending on specified options
set(VCPKG_MANIFEST_FEATURES "")  # start with empty
if(WITH_OTLP_HTTP)
    set(VCPKG_MANIFEST_FEATURES ${VCPKG_MANFIEST_FEATURES} "otlp-http")
endif()
if(WITH_OTLP_GRPC)
    set(VCPKG_MANIFEST_FEATURES ${VCPKG_MANIFEST_FEATURES} "otlp-grpc")
endif()

# ######################################
# Project Declaration
# ######################################

set(CLIENT_PROJECT_NAME otel-matlab)

# read version number
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt OTEL_MATLAB_VERSION_RAW)
string(STRIP ${OTEL_MATLAB_VERSION_RAW} OTEL_MATLAB_VERSION)


project(${CLIENT_PROJECT_NAME} VERSION ${OTEL_MATLAB_VERSION} LANGUAGES CXX)

# ######################################
# libmexclass 
# ######################################

set(LIBMEXCLASS_FETCH_CONTENT_NAME libmexclass)

set(LIBMEXCLASS_FETCH_CONTENT_GIT_REPOSITORY "https://github.com/mathworks/libmexclass.git")

set(LIBMEXCLASS_FETCH_CONTENT_GIT_TAG "bc2b9f2") 

set(LIBMEXCLASS_FETCH_CONTENT_SOURCE_SUBDIR "libmexclass/cpp")


FetchContent_Declare(
    ${LIBMEXCLASS_FETCH_CONTENT_NAME}
    GIT_REPOSITORY ${LIBMEXCLASS_FETCH_CONTENT_GIT_REPOSITORY}
    GIT_TAG ${LIBMEXCLASS_FETCH_CONTENT_GIT_TAG}
    SOURCE_SUBDIR ${LIBMEXCLASS_FETCH_CONTENT_SOURCE_SUBDIR}
)
FetchContent_MakeAvailable(
    ${LIBMEXCLASS_FETCH_CONTENT_NAME}
)


# ###########################
# OpenTelemetry-cpp 
# ###########################

# Determine OTEL_CPP_PREFIX
if(DEFINED OTEL_CPP_INSTALLED_DIR)
   # Set OTEL_CPP_INSTALLED_DIR to use a pre-installed OpenTelemetry-cpp
   # It should point to the installed location
   string(REPLACE "\\" "/" OTEL_CPP_PREFIX ${OTEL_CPP_INSTALLED_DIR})
elseif(DEFINED OTEL_CPP_PREFIX)
   # Set OTEL_CPP_PREFIX to specify an install location of the automatically 
   # downloaded and built OpenTelemetry-cpp, instead of using the default 
   # location with the build directory. This can preserve the OpenTelemetry-cpp
   # install even if the build directory is removed.
     string(REPLACE "\\" "/" OTEL_CPP_PREFIX ${OTEL_CPP_PREFIX})
else()
     set(OTEL_CPP_PREFIX ${CMAKE_BINARY_DIR}/otel-cpp)
endif()

if(WIN32)
    set(OTEL_PROTO_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
    set(OTEL_PROTO_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

set(OTEL_CPP_LIBRARIES ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_common${CMAKE_STATIC_LIBRARY_SUFFIX}
	${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_otlp_recordable${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${OTEL_CPP_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}opentelemetry_proto${OTEL_PROTO_LIBRARY_SUFFIX}
	${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_resources${CMAKE_STATIC_LIBRARY_SUFFIX}
	${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_trace${CMAKE_STATIC_LIBRARY_SUFFIX}
	${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_version${CMAKE_STATIC_LIBRARY_SUFFIX}
	${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_logs${CMAKE_STATIC_LIBRARY_SUFFIX}
	${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_metrics${CMAKE_STATIC_LIBRARY_SUFFIX})
if(WITH_OTLP_HTTP)
    set(OTEL_CPP_LIBRARIES ${OTEL_CPP_LIBRARIES} ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_exporter_otlp_http${CMAKE_STATIC_LIBRARY_SUFFIX}
	    ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_exporter_otlp_http_client${CMAKE_STATIC_LIBRARY_SUFFIX}
	    ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_exporter_otlp_http_metric${CMAKE_STATIC_LIBRARY_SUFFIX}
	    ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_exporter_otlp_http_log${CMAKE_STATIC_LIBRARY_SUFFIX}
	    ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_http_client_curl${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
if(WITH_OTLP_GRPC)
    set(OTEL_CPP_LIBRARIES ${OTEL_CPP_LIBRARIES} ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_exporter_otlp_grpc${CMAKE_STATIC_LIBRARY_SUFFIX}
	    ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_exporter_otlp_grpc_client${CMAKE_STATIC_LIBRARY_SUFFIX}
	    ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_exporter_otlp_grpc_metrics${CMAKE_STATIC_LIBRARY_SUFFIX}
	    ${OTEL_CPP_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}opentelemetry_exporter_otlp_grpc_log${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${OTEL_CPP_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}opentelemetry_proto_grpc${OTEL_PROTO_LIBRARY_SUFFIX})
endif()

if(NOT DEFINED OTEL_CPP_INSTALLED_DIR)
   # No installed location supplied. Fetch it as an external project
   include(ExternalProject)
   set(OTEL_CPP_PROJECT_NAME opentelemetry-cpp)
   set(OTEL_CPP_GIT_REPOSITORY "https://github.com/open-telemetry/opentelemetry-cpp.git")
   set(OTEL_CPP_GIT_TAG "054b0dc")

   set(OTEL_CPP_CXX_STANDARD 14)

   if(SKIP_OTEL_CPP_PATCH)
       set(patch_comand "")
   elseif(WIN32)
       set(patch_command git apply ${CMAKE_SOURCE_DIR}/otel-cpp-windows.patch)
   elseif(APPLE)
       set(patch_command git apply ${CMAKE_SOURCE_DIR}/otel-cpp.patch)
   else()
       set(patch_command "")
   endif()

   ExternalProject_Add(
       ${OTEL_CPP_PROJECT_NAME}
       GIT_REPOSITORY ${OTEL_CPP_GIT_REPOSITORY}
       GIT_TAG ${OTEL_CPP_GIT_TAG}
       PREFIX ${OTEL_CPP_PREFIX}
       UPDATE_DISCONNECTED 1
       PATCH_COMMAND ${patch_command}
       CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DWITH_OTLP_HTTP=${WITH_OTLP_HTTP} -DWITH_OTLP_GRPC=${WITH_OTLP_GRPC} -DBUILD_TESTING=OFF -DWITH_BENCHMARK=OFF -DOPENTELEMETRY_INSTALL=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_CXX_STANDARD=${OTEL_CPP_CXX_STANDARD} -DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR} ${TRIPLET_DEFINITIONS}
       BUILD_BYPRODUCTS ${OTEL_CPP_LIBRARIES}
       INSTALL_DIR ${OTEL_CPP_PREFIX}
       INSTALL_COMMAND ${CMAKE_COMMAND} --install . --prefix ${OTEL_CPP_PREFIX} --config $<CONFIG>
   )
endif()

# ###########################
# OpenTelemetry Proxy Library
# ###########################


set(OPENTELEMETRY_PROXY_LIBRARY_NAME "OtelMatlabProxy")

find_package(Matlab REQUIRED)
find_package(Protobuf REQUIRED)
find_package(nlohmann_json REQUIRED)
if(WIN32)
    find_package(zlib REQUIRED)
endif()

if(WITH_OTLP_HTTP)
    find_package(CURL REQUIRED)
endif()

if(WITH_OTLP_GRPC)
    find_package(gRPC REQUIRED)
    find_package(absl REQUIRED)
    find_package(c-ares REQUIRED)
    find_package(re2 REQUIRED)
    if(WIN32)
        find_package(openssl REQUIRED)
    elseif(UNIX AND NOT APPLE AND NOT CYGWIN)
        find_package(upb REQUIRED)
    endif()
endif()

set(TRACE_API_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/trace/include)
set(METRICS_API_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/metrics/include)
set(LOGS_API_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/logs/include)
set(CONTEXT_API_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/context/include) 
set(BAGGAGE_API_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/baggage/include) 
set(COMMON_API_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/common/include) 
set(TRACE_SDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk/trace/include)
set(METRICS_SDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk/metrics/include)
set(LOGS_SDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk/logs/include)
set(COMMON_SDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk/common/include)
set(OTLP_EXPORTER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/include)
set(OPENTELEMETRY_PROXY_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${TRACE_API_INCLUDE_DIR} ${METRICS_API_INCLUDE_DIR} ${LOGS_API_INCLUDE_DIR} ${CONTEXT_API_INCLUDE_DIR} ${BAGGAGE_API_INCLUDE_DIR} ${COMMON_API_INCLUDE_DIR} ${TRACE_SDK_INCLUDE_DIR} ${METRICS_SDK_INCLUDE_DIR} ${LOGS_SDK_INCLUDE_DIR} ${COMMON_SDK_INCLUDE_DIR} ${OTLP_EXPORTER_INCLUDE_DIR} ${OTEL_CPP_PREFIX}/include ${Matlab_INCLUDE_DIRS})

set(OPENTELEMETRY_PROXY_FACTORY_CLASS_NAME OtelMatlabProxyFactory)
set(OPENTELEMETRY_PROXY_FACTORY_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TRACE_API_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/trace/src)
set(METRICS_API_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/metrics/src)
set(LOGS_API_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/logs/src)
set(CONTEXT_API_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/context/src)
set(BAGGAGE_API_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/baggage/src)
set(COMMON_API_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api/common/src)
set(TRACE_SDK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk/trace/src)
set(METRICS_SDK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk/metrics/src)
set(LOGS_SDK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk/logs/src)
set(COMMON_SDK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk/common/src)
set(OTLP_EXPORTER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/src)
set(OPENTELEMETRY_PROXY_SOURCES
    ${OPENTELEMETRY_PROXY_FACTORY_SOURCES_DIR}/${OPENTELEMETRY_PROXY_FACTORY_CLASS_NAME}.cpp
    ${TRACE_API_SOURCE_DIR}/TracerProviderProxy.cpp
    ${TRACE_API_SOURCE_DIR}/TracerProxy.cpp
    ${TRACE_API_SOURCE_DIR}/SpanProxy.cpp
    ${TRACE_API_SOURCE_DIR}/SpanContextProxy.cpp
    ${COMMON_API_SOURCE_DIR}/attribute.cpp
    ${METRICS_API_SOURCE_DIR}/MeterProviderProxy.cpp
    ${METRICS_API_SOURCE_DIR}/MeterProxy.cpp
    ${METRICS_API_SOURCE_DIR}/CounterProxy.cpp
    ${METRICS_API_SOURCE_DIR}/UpDownCounterProxy.cpp
    ${METRICS_API_SOURCE_DIR}/HistogramProxy.cpp
    ${METRICS_API_SOURCE_DIR}/SynchronousInstrumentProxyFactory.cpp
    ${METRICS_API_SOURCE_DIR}/MeasurementFetcher.cpp
    ${METRICS_API_SOURCE_DIR}/AsynchronousInstrumentProxy.cpp
    ${METRICS_API_SOURCE_DIR}/AsynchronousInstrumentProxyFactory.cpp
    ${LOGS_API_SOURCE_DIR}/LoggerProviderProxy.cpp
    ${LOGS_API_SOURCE_DIR}/LoggerProxy.cpp
    ${CONTEXT_API_SOURCE_DIR}/TextMapPropagatorProxy.cpp
    ${CONTEXT_API_SOURCE_DIR}/CompositePropagatorProxy.cpp
    ${CONTEXT_API_SOURCE_DIR}/TextMapCarrierProxy.cpp
    ${CONTEXT_API_SOURCE_DIR}/ContextProxy.cpp
    ${BAGGAGE_API_SOURCE_DIR}/BaggageProxy.cpp
    ${TRACE_SDK_SOURCE_DIR}/TracerProviderProxy.cpp
    ${TRACE_SDK_SOURCE_DIR}/SimpleSpanProcessorProxy.cpp
    ${TRACE_SDK_SOURCE_DIR}/BatchSpanProcessorProxy.cpp
    ${TRACE_SDK_SOURCE_DIR}/ParentBasedSamplerProxy.cpp
    ${METRICS_SDK_SOURCE_DIR}/MeterProviderProxy.cpp
    ${METRICS_SDK_SOURCE_DIR}/ViewProxy.cpp
    ${METRICS_SDK_SOURCE_DIR}/PeriodicExportingMetricReaderProxy.cpp
    ${LOGS_SDK_SOURCE_DIR}/LoggerProviderProxy.cpp
    ${LOGS_SDK_SOURCE_DIR}/SimpleLogRecordProcessorProxy.cpp
    ${LOGS_SDK_SOURCE_DIR}/BatchLogRecordProcessorProxy.cpp
    ${COMMON_SDK_SOURCE_DIR}/resource.cpp)
if(WITH_OTLP_HTTP)
    set(OPENTELEMETRY_PROXY_SOURCES 
    ${OPENTELEMETRY_PROXY_SOURCES} 
    ${OTLP_EXPORTER_SOURCE_DIR}/OtlpHttpSpanExporterProxy.cpp
    ${OTLP_EXPORTER_SOURCE_DIR}/OtlpHttpMetricExporterProxy.cpp
    ${OTLP_EXPORTER_SOURCE_DIR}/OtlpHttpLogRecordExporterProxy.cpp)
endif()
if(WITH_OTLP_GRPC)
    set(OPENTELEMETRY_PROXY_SOURCES 
    ${OPENTELEMETRY_PROXY_SOURCES}
    ${OTLP_EXPORTER_SOURCE_DIR}/OtlpGrpcSpanExporterProxy.cpp
    ${OTLP_EXPORTER_SOURCE_DIR}/OtlpGrpcMetricExporterProxy.cpp
    ${OTLP_EXPORTER_SOURCE_DIR}/OtlpGrpcLogRecordExporterProxy.cpp)
endif()

libmexclass_client_add_proxy_library(
    NAME ${OPENTELEMETRY_PROXY_LIBRARY_NAME}
    SOURCES ${OPENTELEMETRY_PROXY_SOURCES}
    INCLUDE_DIRS ${OPENTELEMETRY_PROXY_INCLUDE_DIRS}
)

if(NOT DEFINED OTEL_CPP_INSTALLED_DIR)
    add_dependencies(${OPENTELEMETRY_PROXY_LIBRARY_NAME} ${OTEL_CPP_PROJECT_NAME})
endif()

# Additional compiler flags for HTTP/gRPC exporters
if(WITH_OTLP_HTTP)
    target_compile_definitions(${OPENTELEMETRY_PROXY_LIBRARY_NAME} PRIVATE WITH_OTLP_HTTP)
endif()

if(WITH_OTLP_GRPC)
    target_compile_definitions(${OPENTELEMETRY_PROXY_LIBRARY_NAME} PRIVATE WITH_OTLP_GRPC)
endif()

# pass in version number
target_compile_definitions(${OPENTELEMETRY_PROXY_LIBRARY_NAME} PRIVATE OTEL_MATLAB_VERSION="${OTEL_MATLAB_VERSION}")
if(WIN32)
    # On Windows, suppress a compiler warning about deprecation of result_of
    target_compile_definitions(${OPENTELEMETRY_PROXY_LIBRARY_NAME} PUBLIC _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING)
endif()

# link against OpenTelemetry-cpp libraries and their dependencies
set(OPENTELEMETRY_PROXY_LINK_LIBRARIES ${OTEL_CPP_LIBRARIES} ${Protobuf_LIBRARIES} ${Matlab_MEX_LIBRARY})
if(WITH_OTLP_HTTP)
    set(OPENTELEMETRY_PROXY_LINK_LIBRARIES ${OPENTELEMETRY_PROXY_LINK_LIBRARIES} ${CURL_LIBRARIES})
endif()
if(WITH_OTLP_GRPC)
    set(OPENTELEMETRY_PROXY_LINK_LIBRARIES ${OPENTELEMETRY_PROXY_LINK_LIBRARIES} gRPC::grpc++ absl::synchronization)
endif()

target_link_libraries(${OPENTELEMETRY_PROXY_LIBRARY_NAME} PRIVATE ${OPENTELEMETRY_PROXY_LINK_LIBRARIES})

# On Linux, when linking with certain static libraries, need to force include entire archive to avoid the linker mistakenly leaving out symbols
if(UNIX AND NOT APPLE AND NOT CYGWIN)
    set(OPENTELEMETRY_PROXY_LINK_OPTIONS -Wl,--whole-archive 
	"${OTEL_CPP_PREFIX}/lib/libopentelemetry_trace.a"
	"${OTEL_CPP_PREFIX}/lib/libopentelemetry_logs.a"
	"${OTEL_CPP_PREFIX}/lib/libopentelemetry_common.a"
	"${OTEL_CPP_PREFIX}/lib/libopentelemetry_otlp_recordable.a"
        ${ABSL_LIBRARIES}
        ${UPB_LIBRARIES} -Wl,--no-whole-archive)
    target_link_options(${OPENTELEMETRY_PROXY_LIBRARY_NAME} PRIVATE ${OPENTELEMETRY_PROXY_LINK_OPTIONS})
elseif(APPLE)
    set_target_properties(${OPENTELEMETRY_PROXY_LIBRARY_NAME} PROPERTIES INSTALL_RPATH "@executable_path")
endif()

# Use C++17 
target_compile_features(${OPENTELEMETRY_PROXY_LIBRARY_NAME} PRIVATE cxx_std_17)

if(WIN32)
    # runtime dependent libraries
    # The TARGET_FILE generator command is somehow returning the .lib file instead of .dll for Protobuf and Zlib. 
    # As a result, we have to hardcode those library names instead.
    set(PROTOBUF_DLL $<$<CONFIG:Debug>:libprotobufd.dll>$<$<CONFIG:Release>:libprotobuf.dll>)
    set(ZLIB_DLL $<$<CONFIG:Debug>:zlibd1.dll>$<$<CONFIG:Release>:zlib1.dll>)
    set(OPENTELEMETRY_PROXY_RUNTIME_LIBRARIES $<TARGET_FILE_DIR:protobuf::libprotobuf>/../bin/${PROTOBUF_DLL}
	    $<TARGET_FILE_DIR:ZLIB::ZLIB>/../bin/${ZLIB_DLL})

    if(WITH_OTLP_HTTP)
	set(OPENTELEMETRY_PROXY_RUNTIME_LIBRARIES ${OPENTELEMETRY_PROXY_RUNTIME_LIBRARIES}
            $<TARGET_FILE:CURL::libcurl>)
    endif()

    if(WITH_OTLP_GRPC)
	# The TARGET_FILE generator command fails to return the DLL libraries for OpenSSL.
        # As a result, we have to hardcode those library names instead.
	set(OPENSSL_DLL libssl-3-x64.dll)
	set(OPENSSL_CRYPTO_DLL libcrypto-3-x64.dll)
	set(OPENTELEMETRY_PROXY_RUNTIME_LIBRARIES ${OPENTELEMETRY_PROXY_RUNTIME_LIBRARIES}
	    $<TARGET_FILE:c-ares::cares>
	    $<TARGET_FILE_DIR:OpenSSL::SSL>/../bin/${OPENSSL_DLL}
	    $<TARGET_FILE_DIR:OpenSSL::SSL>/../bin/${OPENSSL_CRYPTO_DLL}
	    $<TARGET_FILE:re2::re2>)
    endif()
elseif(UNIX AND NOT CYGWIN)
    if(WITH_OTLP_GRPC)
        set(OTEL_CPP_RUNTIME ${OTEL_CPP_PREFIX}/lib/libopentelemetry_proto${CMAKE_SHARED_LIBRARY_SUFFIX}
                             ${OTEL_CPP_PREFIX}/lib/libopentelemetry_proto_grpc${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(OTEL_CPP_RUNTIME ${OTEL_CPP_PREFIX}/lib/libopentelemetry_proto${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()
    set(OPENTELEMETRY_PROXY_RUNTIME_LIBRARIES ${OTEL_CPP_RUNTIME})
endif()

# ##############################
# OpenTelemetry MEX Gateway
# ##############################

# Create the MEX gateway target.

set(OPENTELEMETRY_MEX_GATEWAY_NAME "gateway")
set(OPENTELEMETRY_MEX_GATEWAY_SOURCES ${CMAKE_SOURCE_DIR}/mex/gateway.cpp)

libmexclass_client_add_mex_gateway(
    NAME ${OPENTELEMETRY_MEX_GATEWAY_NAME}
    CLIENT_PROXY_LIBRARY_NAME ${OPENTELEMETRY_PROXY_LIBRARY_NAME}
    SOURCES ${OPENTELEMETRY_MEX_GATEWAY_SOURCES}
)

# ###############################
# libmexclass Client Installation
# ###############################

libmexclass_client_install(
    CLIENT_PROXY_LIBRARY_NAME ${OPENTELEMETRY_PROXY_LIBRARY_NAME}
    CLIENT_MEX_GATEWAY_NAME ${OPENTELEMETRY_MEX_GATEWAY_NAME}
    DESTINATION "."
)

# Install M files
set(TRACE_API_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/api/trace/+opentelemetry)
set(METRICS_API_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/api/metrics/+opentelemetry)
set(LOGS_API_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/api/logs/+opentelemetry)
set(CONTEXT_API_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/api/context/+opentelemetry)
set(BAGGAGE_API_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/api/baggage/+opentelemetry)
set(COMMON_API_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/api/common/+opentelemetry)
set(TRACE_SDK_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sdk/trace/+opentelemetry)
set(METRICS_SDK_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sdk/metrics/+opentelemetry)
set(LOGS_SDK_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sdk/logs/+opentelemetry)
set(COMMON_SDK_MATLAB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sdk/common/+opentelemetry)
set(EXPORTER_MATLAB_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/defaultSpanExporter.m
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/defaultMetricExporter.m
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/defaultLogRecordExporter.m
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/OtlpValidator.m)
set(OTLP_HTTP_EXPORTER_MATLAB_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/OtlpHttpSpanExporter.m
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/OtlpHttpMetricExporter.m
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/OtlpHttpLogRecordExporter.m
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/OtlpHttpValidator.m)
set(OTLP_GRPC_EXPORTER_MATLAB_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/OtlpGrpcSpanExporter.m
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/OtlpGrpcMetricExporter.m
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/OtlpGrpcLogRecordExporter.m
    ${CMAKE_CURRENT_SOURCE_DIR}/exporters/otlp/+opentelemetry/+exporters/+otlp/OtlpGrpcValidator.m)
set(OTLP_MISC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt)

set(OTLP_EXPORTERS_DIR +opentelemetry/+exporters/+otlp)

install(DIRECTORY ${TRACE_API_MATLAB_SOURCES} DESTINATION .)
install(DIRECTORY ${METRICS_API_MATLAB_SOURCES} DESTINATION .)
install(DIRECTORY ${LOGS_API_MATLAB_SOURCES} DESTINATION .)
install(DIRECTORY ${CONTEXT_API_MATLAB_SOURCES} DESTINATION .)
install(DIRECTORY ${BAGGAGE_API_MATLAB_SOURCES} DESTINATION .)
install(DIRECTORY ${COMMON_API_MATLAB_SOURCES} DESTINATION .)
install(DIRECTORY ${TRACE_SDK_MATLAB_SOURCES} DESTINATION .)
install(DIRECTORY ${METRICS_SDK_MATLAB_SOURCES} DESTINATION .)
install(DIRECTORY ${LOGS_SDK_MATLAB_SOURCES} DESTINATION .)
install(DIRECTORY ${COMMON_SDK_MATLAB_SOURCES} DESTINATION .)
install(FILES ${EXPORTER_MATLAB_SOURCES} DESTINATION ${OTLP_EXPORTERS_DIR})
if(WITH_OTLP_HTTP)
    install(FILES ${OTLP_HTTP_EXPORTER_MATLAB_SOURCES} DESTINATION ${OTLP_EXPORTERS_DIR})
endif()
if(WITH_OTLP_GRPC)
    install(FILES ${OTLP_GRPC_EXPORTER_MATLAB_SOURCES} DESTINATION ${OTLP_EXPORTERS_DIR})
endif()
# Install license and version. The only reason these are needed is to work around a packaging bug where if 
# all files are in packages (+xxx directories), matlab.addons.toolbox.packageToolbox refuses to add the 
# toolbox path into MATLAB path. This issue is fixed in R2024a
install(FILES ${OTLP_MISC_FILES} DESTINATION .)

# Install dependent runtime libraries
set(LIBMEXCLASS_PROXY_INSTALLED_DIR +libmexclass/+proxy)
install(FILES ${OPENTELEMETRY_PROXY_RUNTIME_LIBRARIES} DESTINATION ${LIBMEXCLASS_PROXY_INSTALLED_DIR})

# ##############################
# Subdirectories
# ##############################
if(WITH_EXAMPLES)
   add_subdirectory(examples)
endif()
