#ifndef CACHE_HPP
#define CACHE_HPP

#include <map>
#include <systemc>
using namespace sc_core;

#include "main_memory.hpp"
// #include "AccessSimulator.hpp"
#include "Layer.hpp"

typedef struct OffsetRange {
    uint32_t from_offset; // byte-offset inside a 4-byte block
    uint32_t to_offset; // exclusive
} OffsetRange;


SC_MODULE(CACHE){
    
    MAIN_MEMORY mem; // main memory submodule

    // inputs
    sc_in<bool> clk;
    sc_in<uint32_t> addr;
    sc_in<uint32_t> wdata;
    sc_in<bool> r;
    sc_in<bool> w;

    sc_in<bool> mem_ready;
    sc_in<uint32_t> mem_rdata;

    //outputs
    sc_out<uint32_t> rdata;
    sc_out<bool> ready;
    sc_out<bool> miss;
    
    sc_out<uint32_t> mem_addr;
    sc_out<uint32_t> mem_wdata;
    sc_out<bool> mem_r;
    sc_out<bool> mem_w;
    
    // request signals
    sc_signal<bool> request_enable; // bound to the enable input of the first layer

    sc_signal<bool> request_ready; // bound to the ready output of the first layer
    sc_vector<sc_signal<uint8_t>> request_line_data; // bound to the output_line_data of the first layer
    
    // memory request signals
    sc_signal<bool> memory_request_ready; // bound to the promote_enable input of the last layer
    sc_vector<sc_signal<uint8_t>> memory_line_rdata; // bound to the promoted_line_data input of the last layer
    
    sc_signal<bool> memory_request_enable; // bound to the enable_next output of the last layer
    
    // other signals
    sc_signal<uint32_t> cycles_counter;
    sc_signal<uint32_t> subrange_counter;

    sc_signal<bool> internal_ready; // controls the actual ready output

    sc_signal<bool> has_been_written; // allows to only write to memory on the first subrequest of each write request

    sc_signal<uint32_t> rangefrom; // offset from addr where subrange starts
    sc_signal<uint32_t> rangeto; // offset from addr where subrange ends (exclusive)
    
    sc_vector<sc_signal<bool>> layer_misses; // receives miss outputs of each layer

    // wiring signals
    sc_vector<sc_vector<sc_signal<uint8_t>>> layer_line_data_wires; // each layer has its own output_line_data, which is bound to the promoted_line_data of the next layer via such a signal
    sc_vector<sc_signal<bool>> layer_enable_wires; // each layer has its own enable_next, which is bound to the enable of the next layer via such a signal
    sc_vector<sc_signal<bool>> layer_ready_wires; // each layer has its own promote_enable, which is bound to the ready of the next layer via such a signal
    
    sc_signal<bool> mem_ready_wire;
    sc_signal<uint32_t> mem_rdata_wire;

    sc_signal<uint32_t> mem_addr_wire;
    sc_signal<uint32_t> mem_wdata_wire;
    sc_signal<bool> mem_r_wire;
    sc_signal<bool> mem_w_wire;

    // layers
    sc_vector<LAYER> layers;

    uint32_t num_layers;
    uint32_t line_size;
    uint32_t max_cycles;

    // For address decomposition
    uint32_t n_offset_bits;

    // signal writing events
    sc_event write_ready_true_event;
    sc_event write_ready_false_event;
    sc_event write_miss_true_event;
    sc_event write_miss_false_event;
    sc_event write_has_been_written_true_event;
    sc_event write_has_been_written_false_event;

    uint32_t value_to_write_to_rdata;
    sc_event write_rdata_event;


    SC_CTOR(CACHE);

    CACHE(sc_module_name name, uint8_t num_layers, uint32_t line_size, std::array<uint32_t, 3> layer_sizes, std::array<uint32_t, 3> latencies, uint8_t strategy, uint32_t max_cycles) : 
        sc_module(name),
        mem("mem"),
        num_layers(num_layers),
        line_size(line_size),
        max_cycles(max_cycles),

        layer_misses("layer_misses", num_layers),

        layer_enable_wires("layer_enable_wires", num_layers),
        layer_ready_wires("layer_ready_wires", num_layers),
        layer_line_data_wires("layer_line_data_wires", num_layers),

        memory_line_rdata("memory_line_rdata", line_size),
        request_line_data("request_line_data", line_size),
        mem_w("mem_w"),
        mem_r("mem_r"),
        memory_request_enable("memory_request_enable", 0),
        memory_request_ready("memory_request_ready", 0)
    {

        n_offset_bits = std::ceil(log2(line_size));

        mem.clk(clk);

        mem.addr(mem_addr_wire);
        mem_addr(mem_addr_wire);
        mem.wdata(mem_wdata_wire);
        mem_wdata(mem_wdata_wire);
        mem.w.bind(mem_w_wire);
        mem_w.bind(mem_w_wire);
        mem.r(mem_r_wire);
        mem_r(mem_r_wire);

        mem.ready(mem_ready_wire);
        mem_ready(mem_ready_wire);
        
        mem.rdata(mem_rdata_wire);
        mem_rdata(mem_rdata_wire);


        for (size_t i = 0; i < num_layers; ++i) {
            std::string base = "line_data_" + std::to_string(i);

            layer_line_data_wires[i].init(
                line_size,
                [=](const char* name, size_t j) {
                    return new sc_signal<uint8_t>((base + "_" + std::to_string(j)).c_str());
                }
            );
        }


        // INITIALIZATION OF THE LAYERS (DUN DUN DUUUUN xD)

        layers.init(num_layers, [&](const char* lname, size_t i) {
            return new LAYER(lname, i,
                     line_size,
                     layer_sizes.at(i),
                     latencies.at(i),
                     strategy);
        });


        // binding global inputs and outputs of the layers
        for (int i = 0; i < num_layers; ++i) {
            
            layers[i].clk.bind(clk);
            layers[i].addr.bind(addr);
            layers[i].wdata.bind(wdata);
            layers[i].rangefrom.bind(rangefrom);
            layers[i].rangeto.bind(rangeto);
            layers[i].w.bind(w);
            layers[i].r.bind(r);

            layers[i].miss.bind(layer_misses[i]);
        }


        // binding external local inputs and outputs of first layer
        layers[0].enable(request_enable);

        layers[0].ready(request_ready);
        layers[0].output_line_data(request_line_data);

        // binding internal local inputs and outputs of in-between layers
        // (for each layer bind the connections to the next layer)
        for (int i = 0; i < num_layers-1; ++i) {
            layers[i].enable_promote(layer_ready_wires[i]);
            layers[i+1].ready(layer_ready_wires[i]);

            bindByteVectorsInSigOut(layers[i].promoted_line_data, layer_line_data_wires[i], layers[i+1].output_line_data);

            layers[i].enable_next(layer_enable_wires[i]);
            layers[i+1].enable(layer_enable_wires[i]);
        }

        // binding external local inputs and outputs of last layer
        layers[num_layers-1].enable_promote(memory_request_ready);
        bindByteVectorsInSig(layers[num_layers-1].promoted_line_data, memory_line_rdata);

        layers[num_layers-1].enable_next(memory_request_enable);


        SC_THREAD(behaviour);
        sensitive << clk.pos();
        dont_initialize();

        SC_THREAD(doRequest);
        sensitive << internal_ready;
        dont_initialize();

        SC_THREAD(doMemoryRequest);
        sensitive << memory_request_enable;
        dont_initialize();

        SC_THREAD(writeSignals_OnEvents);

    }

    void writeSignals_OnEvents() {
        while (true) {
            wait(write_ready_true_event |
                write_ready_false_event |
                write_has_been_written_true_event |
                write_has_been_written_false_event |
                write_miss_true_event |
                write_miss_false_event |
                write_rdata_event
                );

            if (write_ready_true_event.triggered()) {
                ready.write(1);
                internal_ready.write(1);
            }
            if (write_ready_false_event.triggered()) {
                ready.write(0);
                internal_ready.write(0);
            }
            if (write_has_been_written_true_event.triggered()) {
                has_been_written.write(1);
            }
            if (write_has_been_written_false_event.triggered()) {
                has_been_written.write(0);
            }
            if (write_miss_true_event.triggered()) {
                miss.write(1);
            }
            if (write_miss_false_event.triggered()) {
                miss.write(0);
            }
            if (write_rdata_event.triggered()) {
                rdata.write(value_to_write_to_rdata);
            }

        }
    }

    void behaviour(){
        write_ready_true_event.notify();
        wait(SC_ZERO_TIME);

        while (true) {

            // increment cycles counter
            cycles_counter.write(cycles_counter.read() + 1);
            if (cycles_counter.read() >= max_cycles) {
                // stop the simulation if max cycles reached
                std::cout << "Max cycles reached: stopping simulation" << std::endl;
                sc_stop();
            }
            
            // wait for r XOR w to be set when cache is ready, then start cache
            if (internal_ready.read() && (r.read() ^ w.read())) {
                
                // reset signals
                write_miss_false_event.notify();

                // imitates: rdata.write(0);
                value_to_write_to_rdata = 0;
                write_rdata_event.notify();

                write_has_been_written_false_event.notify();
                
                write_ready_false_event.notify(); // triggers doRequest
            }
            wait();
        }
    }

    void doRequest() {
        while (true) {
            if (!internal_ready.read()) { // should work like "sensitive << internal_ready.neg()"

                // extract sub-ranges from request
                std::vector<OffsetRange> subRanges = getRequestSubRanges();

                if (r.read()){

                    for (OffsetRange& range : subRanges) {
                        wait(SC_ZERO_TIME);
                        subrange_counter.write(subrange_counter.read() + 1);
                        // handle sub-read request
                        std::cout << "SUB REQUEST READ: with range ("  << range.from_offset  << ", " << range.to_offset << ")" << std::endl;

                        // write signals to start subrequest
                        rangefrom.write(range.from_offset);
                        rangeto.write(range.to_offset);
                        
                        // produce rising edge in request_enable
                        request_enable.write(1);
                        wait(SC_ZERO_TIME); 
                        request_enable.write(0);

                        // wait for request_ready
                        do {
                            wait(request_ready.default_event());
                        } while (!request_ready.read());

                        for (int i = 0; i < num_layers; ++i) {
                            if (layer_misses[i].read()) {
                                write_miss_true_event.notify();
                            } else {
                                write_miss_false_event.notify();
                                break;
                            }
                        }
                        
                        // calculate the cacheline-offset of the current sub-range
                        uint32_t sub_address = addr.read() + range.from_offset;
                        uint32_t sub_offset = sub_address % line_size;

                        // write the relevant bytes from request_line_data to rdata
                        for (uint32_t i = 0; i < range.to_offset - range.from_offset; ++i) {
                            // imitates: rdata.write(...);
                            uint8_t byte = request_line_data[sub_offset + i];
                            value_to_write_to_rdata |= (uint32_t(byte) << ((range.from_offset + i)*8));
                            write_rdata_event.notify();
                            wait(SC_ZERO_TIME);
                        }
                    }

                } else if (w.read()){

                    for (OffsetRange& range : subRanges) {

                        wait(SC_ZERO_TIME);
                        subrange_counter.write(subrange_counter.read() + 1);
                        // handle write for each sub-range individually

                        std::cout << "SUB REQUEST WRITE: with range ("  << range.from_offset  << ", " << range.to_offset << ")" << std::endl;

                        // write signals to start subrequest
                        rangefrom.write(range.from_offset);
                        rangeto.write(range.to_offset);

                        // produce rising edge in request_enable                        
                        request_enable.write(1);
                        wait(SC_ZERO_TIME); 
                        request_enable.write(0);

                        // wait for request_ready
                        do {
                            wait(request_ready.default_event());
                        } while (!request_ready.read());

                        
                        bool was_miss = true;
                        for (int i = 0; i < num_layers; ++i) {
                            if (!layer_misses[i].read()) {
                                was_miss = false;
                                break;
                            }
                        }
                        if (was_miss) {
                            write_miss_true_event.notify();
                        }
                        
                    }

                }

                // write ready at start of cycle, to give wrapper programm opportunity to detect ready-change before next cycle
                // to allow for update of request inputs befor next cycle
                // wait(clk.posedge_event());
                write_ready_true_event.notify();
                sc_pause();

            }
            wait();
        }
    }

    void doMemoryRequest() {
        while (true) {
            if (memory_request_enable.read()) { // should work like "sensitive << memory_request_enable.pos()"
                
                memory_request_ready.write(0);
                wait(SC_ZERO_TIME);

                if (r.read()) {
                    // do read
                    readLineFromMemory();
                } else if (w.read()) {
                    // do write
                    if (!has_been_written.read()) {
                        std::cout << "Write to Memory: " << std::hex << static_cast<int>(wdata.read()) << std::dec;
                        std::cout << "   - at address: " << std::hex << static_cast<int>(addr.read()) << std::dec << std::endl;
                        write_has_been_written_true_event.notify();
                        
                        mem_wdata.write(wdata.read());
                        mem_addr.write(addr.read());
                        //wait(SC_ZERO_TIME);
                        mem_r.write(0);
                        mem_w.write(1); // triggers memory

                        // wait for mem_ready
                        do { wait(mem_ready.default_event()); }
                        while (!mem_ready.read());
                        
                        mem_r.write(0);
                        mem_w.write(0);
                    }
                    
                    // promote_from_memory (layer_misses[num_layers-1]) means that line wasnt found in any layer
                    // so an additional read is necessary for promotion
                    if (layer_misses[num_layers-1].read()) {
                        // do read
                        readLineFromMemory();
                    }
                }
                
                // trigger promote in last layer
                memory_request_ready.write(1);
                
            }
            wait();
        }
    }

    void readLineFromMemory() {
        std::cout << "Read from Memory at address: " << std::hex << static_cast<int>(( (addr.read() + rangefrom.read()) >> n_offset_bits << n_offset_bits)) << std::dec << std::endl;
        for (size_t chunk_i = 0; chunk_i < line_size; chunk_i+=4) {

            // write address of current cacheline-chunk to mem_addr
            mem_addr.write(( (addr.read() + rangefrom.read()) >> n_offset_bits << n_offset_bits) + chunk_i);  // cutoff offset bits, then add multiples of 4
           
           
            mem_r.write(1); // triggers memory
            mem_w.write(0);
    
            // wait for mem_ready
            do { wait(mem_ready.default_event()); }
            while (!mem_ready.read());
    
            mem_r.write(0);
            mem_w.write(0);
    
            uint32_t chunk = mem_rdata.read();
    
            for (size_t offset = 0; offset < 4 && chunk_i+offset < line_size; ++offset) {
                memory_line_rdata[chunk_i+offset] = static_cast<uint8_t>(chunk >> (offset*8));
            }
        }
    }

    // TODO ONLY SPLIT WHEN REALLY NECESSARY (AT CACHELINE-BORDERS)
    std::vector<OffsetRange> getRequestSubRanges() {

        std::vector<OffsetRange> subRanges;

        uint32_t line_index = addr.read() / line_size;
        uint32_t offset_on_line = addr.read() % line_size;

        if (line_size - offset_on_line >= 4) {

            subRanges.push_back({0, 4});

        } else {

            uint32_t mid = line_size - offset_on_line;

            if (line_size < 4) {

                uint32_t bytes_left = 4;
                uint32_t from = 0;
                bool first_pass = true;

                while (bytes_left > 0) {
            
                    uint32_t max_take = first_pass ? (line_size - offset_on_line) : line_size;
                    uint32_t take = std::min(bytes_left, max_take);

                    subRanges.push_back({from, from + take});

                    bytes_left = bytes_left - take;
                    from = from + take;
                    first_pass = false;
                }

            } else {
    
                subRanges.push_back({0, mid});
                subRanges.push_back({mid, 4});

            }
        }

        return subRanges;

    }

    void bindByteVectorsInSigOut(sc_vector<sc_in<uint8_t>>& vec_in, sc_vector<sc_signal<uint8_t>>& vec_sigs, sc_vector<sc_out<uint8_t>>& vec_out) {
        for (size_t i = 0; i < vec_out.size(); ++i) {
            vec_out[i](vec_sigs[i]);
            vec_in[i](vec_sigs[i]); 
        }
    }
    void bindByteVectorsInSig(sc_vector<sc_in<uint8_t>>& vec_in, sc_vector<sc_signal<uint8_t>>& vec_sigs) {
        for (size_t i = 0; i < vec_in.size(); ++i) {
            vec_in[i](vec_sigs[i]); 
        }
    }
    void bindByteVectorsOutSig(sc_vector<sc_out<uint8_t>>& vec_out, sc_vector<sc_signal<uint8_t>>& vec_sigs) {
        for (size_t i = 0; i < vec_out.size(); ++i) {
            vec_out[i](vec_sigs[i]); 
        }
    }

    uint8_t getCacheLineContent (uint32_t level, uint32_t lineIndex, uint32_t index) {
        if (index >= line_size || index < 0) {
            std::cout << "INVALID INDEX in LAYER : " << level;
            return 0;
        }
        return layers[level-1].getCacheLineContent(lineIndex, index);
    }

};

#endif