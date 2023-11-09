classdef tmetrics_sdk < matlab.unittest.TestCase
    % tests for metrics SDK

    % Copyright 2023 The MathWorks, Inc.

    properties
        OtelConfigFile
        JsonFile
        PidFile
        OtelcolName
        Otelcol
        ListPid
        ReadPidList
        ExtractPid
        Sigint
        Sigterm
        ShortIntervalReader
    end

    methods (TestClassSetup)
        function setupOnce(testCase)
            commonSetupOnce(testCase);
            testCase.ShortIntervalReader = opentelemetry.sdk.metrics.PeriodicExportingMetricReader(...
                opentelemetry.exporters.otlp.OtlpHttpMetricExporter(), ...
                "Interval", seconds(2), "Timeout", seconds(1));
        end
    end

    methods (TestMethodSetup)
        function setup(testCase)
            commonSetup(testCase);
        end
    end

    methods (TestMethodTeardown)
        function teardown(testCase)
            commonTeardown(testCase);
        end
    end

    methods (Test)
        function testDefaultExporter(testCase)
            exporter = opentelemetry.exporters.otlp.defaultMetricExporter;
            verifyEqual(testCase, string(class(exporter)), "opentelemetry.exporters.otlp.OtlpHttpMetricExporter");
            verifyEqual(testCase, string(exporter.Endpoint), "http://localhost:4318/v1/metrics");
            verifyEqual(testCase, exporter.Timeout, seconds(10));
            verifyEqual(testCase, string(exporter.PreferredAggregationTemporality), "cumulative");
        end


        function testExporterBasic(testCase)
            timeout = seconds(5);
            temporality = "delta";
            exporter = opentelemetry.exporters.otlp.OtlpHttpMetricExporter("Timeout", timeout, ...
                "PreferredAggregationTemporality", temporality);
            verifyEqual(testCase, exporter.Timeout, timeout);
            verifyEqual(testCase, string(exporter.PreferredAggregationTemporality), temporality);
        end

        
        function testDefaultReader(testCase)
            reader = opentelemetry.sdk.metrics.PeriodicExportingMetricReader();
            verifyEqual(testCase, string(class(reader.MetricExporter)), ...
                "opentelemetry.exporters.otlp.OtlpHttpMetricExporter");
            verifyEqual(testCase, reader.Interval, minutes(1));
            verifyEqual(testCase, reader.Interval.Format, 'm');  
            verifyEqual(testCase, reader.Timeout, seconds(30));
            verifyEqual(testCase, reader.Timeout.Format, 's');
        end


        function testReaderBasic(testCase)
            exporter = opentelemetry.exporters.otlp.defaultMetricExporter;
            interval = hours(1);
            timeout = minutes(30);
            reader = opentelemetry.sdk.metrics.PeriodicExportingMetricReader(exporter, ...
                "Interval", interval, ...
                "Timeout", timeout);
            verifyEqual(testCase, reader.Interval, interval);
            verifyEqual(testCase, reader.Interval.Format, 'h');  % should not be converted to other units  
            verifyEqual(testCase, reader.Timeout, timeout);
            verifyEqual(testCase, reader.Timeout.Format, 'm');
        end

        
        function testAddMetricReader(testCase)
            metername = "foo";
            countername = "bar";
            exporter1 = opentelemetry.exporters.otlp.OtlpHttpMetricExporter(...
                "PreferredAggregationTemporality", "delta");
            exporter2 = opentelemetry.exporters.otlp.OtlpHttpMetricExporter(...
                "PreferredAggregationTemporality", "delta");
            reader1 = opentelemetry.sdk.metrics.PeriodicExportingMetricReader(exporter1, ...,
                "Interval", seconds(2), "Timeout", seconds(1));
            reader2 = opentelemetry.sdk.metrics.PeriodicExportingMetricReader(exporter2, ...,
                "Interval", seconds(2), "Timeout", seconds(1));
            p = opentelemetry.sdk.metrics.MeterProvider(reader1);
            p.addMetricReader(reader2);
            mt = p.getMeter(metername);
            ct = mt.createCounter(countername);

            % verify if the provider has two metric readers attached
            reader_count = numel(p.MetricReader);
            verifyEqual(testCase,reader_count, 2);

            % verify if the json results has two exported instances after
            % adding a single value
            ct.add(1);
            pause(2.5);
            clear p;
            results = readJsonResults(testCase);
            result_count = numel(results);
            verifyEqual(testCase,result_count, 2);
        end

        function testCustomResource(testCase)
            % testCustomResource: check custom resources are included in
            % emitted metrics
            customkeys = ["foo" "bar"];
            customvalues = [1 5];
            mp = opentelemetry.sdk.metrics.MeterProvider(testCase.ShortIntervalReader, ...
                "Resource", dictionary(customkeys, customvalues)); 
            
            m = getMeter(mp, "mymeter");
            c = createCounter(m, "mycounter");

            % create testing value 
            val = 10;

            % add value and attributes
            c.add(val);

            pause(2.5);

            clear mp;

            % perform test comparisons
            results = readJsonResults(testCase);
            results = results{1};

            resourcekeys = string({results.resourceMetrics.resource.attributes.key});
            for i = length(customkeys)
                idx = find(resourcekeys == customkeys(i));
                verifyNotEmpty(testCase, idx);
                verifyEqual(testCase, results.resourceMetrics.resource.attributes(idx).value.doubleValue, customvalues(i));
            end
        end

        function testShutdown(testCase)
            % testShutdown: shutdown method should stop exporting
            % of metrics
            mp = opentelemetry.sdk.metrics.MeterProvider(testCase.ShortIntervalReader);

            % shutdown the meter provider
            verifyTrue(testCase, shutdown(mp));

            % create an instrument and add some values
            m = getMeter(mp, "foo");
            c = createCounter(m, "bar");
            c.add(5);

            % wait a little and then gather results, verify no metrics are
            % generated
            pause(2.5);
            clear mp;
            results = readJsonResults(testCase);
            verifyEmpty(testCase, results);
        end

        function testCleanupSdk(testCase)
            % testCleanupSdk: shutdown an SDK meter provider through the Cleanup class

            % Shut down an SDK meter provider instance
            mp = opentelemetry.sdk.metrics.MeterProvider(testCase.ShortIntervalReader);

            % shutdown the meter provider through the Cleanup class
            verifyTrue(testCase, opentelemetry.sdk.common.Cleanup.shutdown(mp));

            % create an instrument and add some values
            m = getMeter(mp, "foo");
            c = createCounter(m, "bar");
            c.add(5);

            % wait a little and then gather results, verify no metrics are
            % generated
            pause(2.5);
            clear mp;
            results = readJsonResults(testCase);
            verifyEmpty(testCase, results);
        end

        function testCleanupApi(testCase)
            % testCleanupApi: shutdown an API meter provider through the Cleanup class
  
            % Shut down an API meter provider instance
            mp = opentelemetry.sdk.metrics.MeterProvider(testCase.ShortIntervalReader);
            setMeterProvider(mp);
            clear("mp");
            mp_api = opentelemetry.metrics.Provider.getMeterProvider();

            % shutdown the API meter provider through the Cleanup class
            verifyTrue(testCase, opentelemetry.sdk.common.Cleanup.shutdown(mp_api));

            % create an instrument and add some values
            m = getMeter(mp_api, "foo");
            c = createCounter(m, "bar");
            c.add(5);

            % wait a little and then gather results, verify no metrics are
            % generated
            pause(2.5);
            clear("mp_api");
            results = readJsonResults(testCase);
            verifyEmpty(testCase, results);
        end
    end
end