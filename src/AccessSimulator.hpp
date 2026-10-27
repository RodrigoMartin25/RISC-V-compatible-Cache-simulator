#ifndef ACCESS_SIMULATOR_HPP
#define ACCESS_SIMULATOR_HPP

#include <systemc>
#include <map>
using namespace sc_core;

// include the Request struct
extern "C" {
    #include "global_structs.h"
}

SC_MODULE(ACCESS_SIMULATOR) {
    // Inputs
    sc_in<bool> cache_ready;
    sc_in<uint32_t> rdata;

    // Outputs
    sc_out<uint32_t> addr;
    sc_out<uint32_t> wdata;
    sc_out<bool> r;
    sc_out<bool> w;

    // Requests data
    uint32_t numRequests;
    Request* requests;

    SC_CTOR(ACCESS_SIMULATOR);

    ACCESS_SIMULATOR(sc_module_name name, uint32_t numRequests, Request* requests) : sc_module(name), numRequests(numRequests), requests(requests) {
        SC_THREAD(behaviour);
        sensitive << cache_ready.pos(); // sensitive for cache_ready
    }

    void behaviour() {
        for (uint32_t i = 0; i < numRequests; i++) {
            // writes the next request initially on the start of the simulation
            // and then waits for every rising edge of cache_ready
  
            addr.write(requests[i].addr);
            
            // recover rdata from previous request if it was a read
            if (i > 0 && !requests[i-1].w) {
                requests[i-1].data = rdata.read();
            }

            // write wdata if current request is a write
            if (requests[i].w) {
                wdata.write(requests[i].data);
            }

            r.write(!requests[i].w); // r is !w
            w.write(requests[i].w);

            wait();
        }
        // when all requests processed and cache becomes ready again, stop the simulation
        sc_stop();
    }
};
#endif