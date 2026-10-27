#ifndef LAYER_HPP
#define LAYER_HPP

#include <map>
#include <cmath>
#include <systemc>
using namespace sc_core;

/*
Directmapped:
- Each line holds one Block from the Main memory and thus has a specific Tag
- If line tag is consistent with the searched tag, just read or write(-through)
- If line tag is not consistent, then cache enable_next:
- Read: read from main memory and write to cache
- Write: write to main memory and write to cache

Fully-Associtive:
- Each line holds one Block from the Main memory and thus has a specific Tag
- If the searched tag exists in any of the lines, just read or write(-through)
- If the searched tag does not exist in any of the lines, then cache enable_next:
- Read: read from main memory and write to cache (on the line decided by FIFO queue)
- Write: write to main memory and write to cache (on the line decided by FIFO queue
*/


struct line {
    uint32_t tag;
    bool valid;
    std::vector<uint8_t> data;
};

SC_MODULE(LAYER){ // REPLACEMENT STRATEGY IS FIFO

    // global inputs
    sc_in<bool> clk;
    sc_in<uint32_t> addr; // stays same for full request
    sc_in<uint32_t> wdata; // stays same for full request
    sc_in<bool> r; // stays same for full request
    sc_in<bool> w; // stays same for full request
    sc_in<uint32_t> rangefrom; // different for each subrequest
    sc_in<uint32_t> rangeto; // different for each subrequest

    // global outputs
    sc_out<bool> miss; // turns on when the layer has missed, is part of layer_misses for cache module
    bool internal_miss; // just mirroring the miss output; used in the promote method

    // local inputs
    sc_in<bool> enable; // activates the layer; is the enable_next-output of previous layer
    sc_in<bool> enable_promote; // makes layer recieve promoted data and pass it on; is ready-output of following layer
    sc_vector<sc_in<uint8_t>> promoted_line_data; // the whole cache line that is being promoted; is output_line_data of following layer

    // local outputs
    sc_out<bool> ready; // turns on when the layer has processed a hit or a promote; is enable_promote for prev layer
                        // turns off when enable turns on
    sc_out<bool> enable_next;  // turns on when the layer has missed or completed a write; is enable for next layer
                        // turns off when enable_promote turns on
    sc_vector<sc_out<uint8_t>> output_line_data; // the whole cache line that was read/written/promoted; is promoted_line_data for prev layer


    // local signals
    sc_signal<bool> doing_latency; // turns on when the layer has to enforce its latency

    // Layer Specifications
    uint8_t level; // 1, 2 or 3
    uint32_t line_size; // in Bytes
    uint32_t num_lines;
    uint32_t latency; // in Cycles
    uint8_t strategy; // 0 = direct-mapped, 1 = fully associative

    // For address decomposition
    uint32_t n_offset_bits;
    uint32_t n_index_bits;
    uint32_t n_tag_bits; 

    uint32_t address;
    uint32_t offset;
    uint32_t index;
    uint32_t tag;

    // Queue for FIFO
    sc_fifo<uint32_t> free_q;
    sc_fifo<uint32_t> victim_q;

    // layer lines storage
    std::vector<line> lines;

    
    // events
    sc_event start_latency_event;
    
    // signal writing events
    sc_event write_ready_true_event;
    sc_event write_ready_false_event;
    
    line* line_to_write_to_output_line;
    sc_event write_out_line_event;


    SC_CTOR(LAYER);

    LAYER(sc_module_name name, uint8_t level, uint32_t line_size, uint32_t num_lines, uint32_t latency, uint8_t strategy):
        sc_module(name),
        level(level),
        line_size(line_size),
        num_lines(num_lines),
        latency(latency),
        strategy(strategy),
        lines(num_lines),

        free_q("free_q", num_lines),
        victim_q("victim_q", num_lines),
        output_line_data("output_line_data", line_size),
        promoted_line_data("promoted_line_data", line_size)
    {

        n_offset_bits = std::ceil(log2(line_size));
        n_index_bits = std::ceil(log2(num_lines));
        n_tag_bits = 32 - n_offset_bits - n_index_bits;

        for (uint32_t i = 0; i < num_lines; ++i) free_q.write(i);

        for (uint32_t i = 0; i < num_lines; ++i) {
            lines[i].valid = false;
            lines[i].tag = 0;
            lines[i].data.resize(line_size);
        }

        SC_THREAD(writeReady_OnEvents); // workaround for writing ready from different threads

        SC_THREAD(writeStoredLineToOutputLine_OnEvent); // workaround for writing output_line_data from different threads

        SC_THREAD(doLatency);
        sensitive << clk.pos();
        dont_initialize();

        SC_THREAD(process);
        sensitive << enable;
        dont_initialize();

        SC_THREAD(promote);
        sensitive << enable_promote;
        dont_initialize();
    }

    void writeReady_OnEvents() {
        while (true) {
            wait(write_ready_true_event | write_ready_false_event);

            if (write_ready_true_event.triggered()) {
                ready.write(1);
            }
            if (write_ready_false_event.triggered()) {
                ready.write(0);
            }
        }
    }

    void doLatency() {
        while (true) {
            wait(start_latency_event);
            doing_latency.write(1);

            // start enforcing latency
            for (int i = 0; i < latency; ++i) {
                wait();
            }
            doing_latency.write(0);
            write_ready_true_event.notify(); // indicate that the layer has finished processing
        }
    }

    void process() {
        while (true) {
            if (enable.read()) { // should work like SC_METHOD with "sensitive << enable.pos()"
                miss.write(0);

                std::cout << "Processing Layer " << static_cast<int>(level);
                
                write_ready_false_event.notify();
                
                address = addr.read() + rangefrom.read();
                offset = (address << (32-n_offset_bits)) >> (32-n_offset_bits);

                if (strategy == 0) {
                    index = (address << n_tag_bits) >> (32-n_index_bits);
                    tag = address >> (32-n_tag_bits);
                } else if (strategy == 1) {
                    // no indexbits in fully associative cache
                    tag = address >> n_offset_bits;
                } else {
                    SC_REPORT_ERROR("LAYER", "Unknown mapping strategy");
                }
                
                line* ln;
                
                if (findTargetLineWithStrategy(ln)) {
                    // line was found, HIT
                    std::cout << " - HIT on Layer: " << static_cast<int>(level) << std::endl;
                    miss.write(0);
                    internal_miss = false;
                    
                    if (r.read()) {
                        // handle read hit
                        
                        // imitates: writeStoredLineToOutputLine(ln);
                        line_to_write_to_output_line = ln;
                        
                        write_out_line_event.notify();
                        
                        start_latency_event.notify(); // start enforcing latency
        
                    } else if (w.read()) {
                        // handle write hit
        
                        writeWDataRangeToStoredLine(ln);
                        
                        // imitates: writeStoredLineToOutputLine(ln);
                        line_to_write_to_output_line = ln;
                        write_out_line_event.notify();
                        
    
                        // pass the request on to the following layer to update all the layers
                        // produce rising edge in enable_next
                        enable_next.write(1);
                        wait(SC_ZERO_TIME); 
                        enable_next.write(0);
        
                    } else {
                        write_ready_true_event.notify();
                    }
                } else {
                    // line was not found, MISS
                    std::cout << " - MISS" << std::endl;

                    miss.write(1);
                    internal_miss = true;
        
                    // produce rising edge in enable_next
                    enable_next.write(1);
                    wait(SC_ZERO_TIME);
                    enable_next.write(0);
                }
    
            }
            wait();
        }
    }
    
    bool findTargetLineWithStrategy(line*& ln) {

        if (strategy == 0) {

            ln = &lines[index];
            return ln->tag == tag && ln->valid; // return if its actually a hit

        } else if (strategy == 1) {

            for (int i = 0; i < num_lines; ++i){
                if (lines[i].tag == tag && lines[i].valid){
                    ln = &lines[i];
                    return true;
                }
            }
            return false; // if no line was found, return false

        } else {
            SC_REPORT_ERROR("LAYER", "Unknown mapping strategy");
            return false;
        }
    }

    void promote() {
        while (true) {
            if (enable_promote.read()) { // should work like "sensitive << enable_promote.pos()"
                if (internal_miss) {
                    // if the layer has missed earlier, then it should promote
                    
                    std::cout << "Promoting Layer " << static_cast<int>(level) << std::endl;
                    
                    line* ln;

                    if (strategy == 0) {
                        ln = &(lines[index]);
                    } else if (strategy == 1) {
                        
                        uint32_t slot;

                        // if there is a free slot, then use it, otherwise use the victim queue
                        if (free_q.num_available() > 0) {
                            slot = free_q.read();
                        } else {
                            slot = victim_q.read();
                        }

                        ln = &lines[slot];
                        victim_q.write(slot); // put slot back to the back of the victim queue
                    } else {
                        SC_REPORT_ERROR("LAYER", "Unknown mapping strategy");
                        return;
                    }

                    writePromotedLineToStoredLine(ln); // write promoted input to the line in storage

                    // imitates: writeStoredLineToOutputLine(ln);
                    line_to_write_to_output_line = ln;
                    write_out_line_event.notify();
                }

                // even if a write HITS, it passes execution on to next layers for updating, and thus only becomes ready during promotion
                write_ready_true_event.notify();
            }
            wait();
        }
    }

    void writeStoredLineToOutputLine_OnEvent() {
        while (true) {
            wait(write_out_line_event);
            for (size_t i = 0; i < line_size; ++i) {
                output_line_data[i].write(line_to_write_to_output_line->data[i]);
            }
        }
    }

    void writeWDataRangeToStoredLine(line* ln) {
        uint32_t base_offset = (addr.read() << (32-n_offset_bits)) >> (32-n_offset_bits);

        for (uint32_t i = rangefrom.read(); i < rangeto.read(); ++i) { // from 1 to 3
            ln->data[base_offset+i] = static_cast<uint8_t>(wdata.read() >> (i*8));
        }
        
    }

    void writePromotedLineToStoredLine(line* ln) {
        ln->valid = true;
        ln->tag = tag;
        for (size_t i = 0; i < line_size; ++i) {
            ln->data[i] = promoted_line_data[i].read();
        }
    }

    uint8_t getCacheLineContent(uint32_t lineIndex, uint32_t offset){ //WRONG INPUTS GET CATCHED IN THE CACHE MODULE
        return lines[lineIndex].data[offset];
    }





};

#endif