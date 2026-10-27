#include <array>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <functional>

#include "modules.hpp"
#include "AccessSimulator.hpp"
#include "Cache.hpp"
#include "Layer.hpp"
#include "main_memory.hpp"


#include <systemc>


#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

void printCache( // (source ChatGPT)
    uint8_t numCacheLevels,
    uint32_t cachelineSize,
    uint32_t numLinesL1,
    uint32_t numLinesL2,
    uint32_t numLinesL3,
    std::function<uint8_t(uint32_t, uint32_t, uint32_t)> getCacheLineContent
) {
    std::vector<uint32_t> linesPerLevel = {numLinesL1, numLinesL2, numLinesL3};

    std::cout << "\n=== Cache Contents ===\n";

    for (uint32_t level = 1; level <= numCacheLevels; ++level) {
        uint32_t numLines = linesPerLevel[level - 1];
        bool levelHasContent = false;

        std::cout << "\n--- Level L" << level << " ---\n";

        for (uint32_t line = 0; line < numLines; ++line) {
            std::vector<uint8_t> lineData;
            bool lineHasData = false;

            for (uint32_t i = 0; i < cachelineSize; ++i) {
                uint8_t val = getCacheLineContent(level, line, i);
                lineData.push_back(val);
                if (val != 0)
                    lineHasData = true;
            }

            if (lineHasData) {
                levelHasContent = true;
                std::cout << "Line " << std::setw(3) << line << ": ";
                for (uint8_t byte : lineData) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2)  << static_cast<int>(byte) << " ";
                }
                std::cout << std::dec << std::setfill(' ') << std::endl;
            }
        }

        if (!levelHasContent) {
            std::cout << "(empty)\n";
        }
    }

    std::cout << "=========================\n" << std::endl;
}


void printRequests(uint32_t numRequests, Request* requests) { // (source ChatGPT)
    std::cout << "=== Request List ===" << std::endl;
    std::cout << std::left
              << std::setw(10) << "Index"
              << std::setw(12) << "Address"
              << std::setw(12) << "Data"
              << std::setw(5)  << "W" << std::endl;

    std::cout << "-------------------------------" << std::endl;

    for (uint32_t i = 0; i < numRequests; ++i) {
        std::cout << std::left << std::setw(10) << i;

        std::cout << "0x"  << std::setw(8) << std::hex << requests[i].addr << "   ";
        std::cout << "0x"  << std::setw(8) << std::hex << requests[i].data << "   ";
        std::cout << std::dec << std::setfill(' ') << static_cast<int>(requests[i].w) << std::endl;
    }

    std::cout << "=====================" << std::endl << std::endl;
}


Result run_simulation(uint32_t cycles ,
                    const char* tf_path ,
                    uint8_t numCacheLevels ,
                    uint32_t cachelineSize ,
                    uint32_t numLinesL1 ,
                    uint32_t numLinesL2 ,
                    uint32_t numLinesL3 ,
                    uint32_t latencyCacheL1 ,
                    uint32_t latencyCacheL2 ,
                    uint32_t latencyCacheL3 ,
                    uint8_t mappingStrategy ,
                    uint32_t numRequests ,
                    Request* requests) {

    // Pretty print all arguments (source ChatGPT)
    std::cout << "===== run_simulation =====" << std::endl;
    std::cout << "Trace file path     : " << (tf_path != NULL? tf_path : "null")  << std::endl;
    std::cout << "Simulation cycles   : " << cycles << std::endl;
    std::cout << "Cache levels        : " << static_cast<int>(numCacheLevels) << std::endl;
    std::cout << "Cacheline size      : " << cachelineSize << " B" << std::endl;
    std::cout << "L1 cache lines      : " << numLinesL1 << std::endl;
    std::cout << "L2 cache lines      : " << numLinesL2 << std::endl;
    std::cout << "L3 cache lines      : " << numLinesL3 << std::endl;
    std::cout << "L1 cache latency    : " << latencyCacheL1 << " cycles" << std::endl;
    std::cout << "L2 cache latency    : " << latencyCacheL2 << " cycles" << std::endl;
    std::cout << "L3 cache latency    : " << latencyCacheL3 << " cycles" << std::endl;
    std::cout << "Mapping strategy    : " << static_cast<int>(mappingStrategy) << std::endl;
    std::cout << "Total requests      : " << numRequests << std::endl;
    std::cout << "====================================" << std::endl << std::endl;

                        
    std::array<uint32_t, 3> layer_sizes = {numLinesL1, numLinesL2, numLinesL3};
    std::array<uint32_t, 3> latencies = {latencyCacheL1, latencyCacheL2, latencyCacheL3};

    sc_clock clk("clk", 1, SC_MS);
    
    
    
    // driving signals
    sc_signal<uint32_t> addr;
    sc_signal<uint32_t> wdata;
    sc_signal<bool> r;
    sc_signal<bool> w;

    // receiving signals
    sc_signal<uint32_t> rdata;
    sc_signal<bool> ready;
    sc_signal<bool> miss;
    
    CACHE cache("cache",
        numCacheLevels,
        cachelineSize,
        layer_sizes,
        latencies,
        mappingStrategy,
        cycles);
        
    Result result{0, 0, 0};
    
    cache.clk(clk);
    cache.addr(addr);
    cache.wdata(wdata);
    cache.r(r);
    cache.w(w);

    cache.rdata(rdata);
    cache.ready(ready);
    cache.miss(miss);
    
    sc_trace_file* tf = NULL;
    if (tf_path) {
        tf = sc_create_vcd_trace_file(tf_path);
        if (tf) {
            // TODO add more traces
            sc_trace(tf, clk, "clk");
            sc_trace(tf, addr, "addr");
            sc_trace(tf, wdata, "wdata");
            sc_trace(tf, rdata, "rdata");
            sc_trace(tf, ready, "ready");
            sc_trace(tf, miss, "miss");
            sc_trace(tf, cache.request_enable, "request_enable");
            sc_trace(tf, cache.request_ready, "request_ready");
            sc_trace(tf, cache.memory_request_ready, "memory_request_ready");
            sc_trace(tf, cache.memory_request_enable, "memory_request_enable");
            sc_trace(tf, cache.cycles_counter, "cycles_counter");
            sc_trace(tf, cache.internal_ready, "internal_ready");
            sc_trace(tf, cache.has_been_written, "has_been_written");
            sc_trace(tf, cache.rangefrom, "rangefrom");
            sc_trace(tf, cache.rangeto, "rangeto");
            sc_trace(tf, cache.subrange_counter, "subrange_counter");
            sc_trace(tf, cache.mem_ready_wire, "mem_ready_wire");
            sc_trace(tf, cache.mem_rdata_wire, "mem_rdata_wire");
            sc_trace(tf, cache.mem_addr_wire, "mem_addr_wire");
            sc_trace(tf, cache.mem_wdata_wire, "mem_wdata_wire");
            sc_trace(tf, cache.mem_r_wire, "mem_r_wire");
            sc_trace(tf, cache.mem_w_wire, "mem_w_wire");
            sc_trace(tf, r, "r");
            sc_trace(tf, w, "w");
            
            for (uint32_t i = 0; i < numCacheLevels; ++i) {
                sc_trace(tf, cache.layers[i].doing_latency, "layer_"+std::to_string(i)+"_doing_latency");
            }
            for (uint32_t i = 0; i < numCacheLevels-1; ++i) {
                sc_trace(tf, cache.layer_enable_wires[i], "layer_"+std::to_string(i+1)+"_enable");
                sc_trace(tf, cache.layer_ready_wires[i], "layer_"+std::to_string(i+1)+"_ready");
            }

        }
    }

    for (uint32_t i = 0; i < numRequests; i++) {
        std::cout << std::endl << "Request Number: " << i << std::endl;
  
        addr.write(requests[i].addr);

        // write wdata if current request is a write
        if (requests[i].w) {
            wdata.write(requests[i].data);
        }

        r.write(!requests[i].w); // r is !w
        w.write(requests[i].w);

        sc_start();

        // collect miss/hit from the cache
        if (miss.read()) {
            std::cout << "Request was a MISS" << std::endl;
            result.misses++;
        } else {
            std::cout << "Request was a HIT" << std::endl;
            result.hits++;
        }

        // collect rdata if it was a read
        if (!requests[i].w) {
            requests[i].data = rdata.read();
        }

        // break when the simulation stopped itself
        if (!sc_is_running() && sc_get_status() != SC_PAUSED) {
            break;
        }
    }
    // collect cycles from the cache
    result.cycles = cache.cycles_counter.read();

    sc_stop();
    
    if (tf) {
        sc_close_vcd_trace_file(tf);
    }

    printRequests(numRequests, requests);

    printCache(
    numCacheLevels,
    cachelineSize,
    numLinesL1,
    numLinesL2,
    numLinesL3,
    [&](uint32_t level, uint32_t lineIndex, uint32_t index) {
        return cache.getCacheLineContent(level, lineIndex, index);
    });


    return result;
}

// default
int sc_main(int argc, char* argv[])
{
    std::cout << "ERROR" << std::endl;
    return 1;
}