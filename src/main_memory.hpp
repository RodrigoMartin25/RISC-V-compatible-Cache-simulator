#ifndef MAIN_MEMORY_HPP
#define MAIN_MEMORY_HPP

// Aus den Tutorien :)

#include <systemc>
#include <map>
using namespace sc_core;

#define LATENCY 100

SC_MODULE(MAIN_MEMORY) {
  // inputs
  sc_in<bool> clk;
  sc_in<uint32_t> addr;
  sc_in<uint32_t> wdata;
  sc_in<bool> r;
  sc_in<bool> w;

  // outputs
  sc_out<uint32_t> rdata;
  sc_out<bool> ready;

  sc_signal<uint32_t> cycles_counter;


  std::map<uint32_t, uint8_t> memory;

  SC_CTOR(MAIN_MEMORY) {
    SC_THREAD(behaviour);
    sensitive << clk.pos();
  }

  void behaviour() {
    while(true) {
      wait();
      cycles_counter.write(cycles_counter.read() + 1);

      if (r.read()) {
        doRead(w.read());
      }
      if (w.read()) {
        doWrite();
      }
    }
  }

  void doRead(bool dontSetReady) {
    ready.write(false);

    uint32_t result = get(addr.read());

    for(int i = 0; i < LATENCY-1; i++) {
      wait();
      cycles_counter.write(cycles_counter.read() + 1);
    }

    rdata.write(result);
    if(!dontSetReady) {
      ready.write(true);
    }
  }

  void doWrite() {
    ready.write(false);
    set(addr.read(), wdata.read());

    for(int i = 0; i < LATENCY-1; i++) {
      wait();
      cycles_counter.write(cycles_counter.read() + 1);
    }

    ready.write(true);
  }

  uint32_t get(uint32_t address) {
    uint32_t result = 0;

    for (int i = 0; i < 4; i++) {
      uint8_t value = 0;
      if(memory.find(address + i) != memory.end()) {
        value = memory[address + i];
      }
      result |= static_cast<uint32_t>(value) << (i * 8);
    }

    return result;
  }

  void set(uint32_t address, uint32_t value) {
    for (int i = 0; i < 4; i++) {
      memory[address + i] = static_cast<uint8_t>(value >> (i * 8));
      if(address + i == UINT32_MAX) {
        break;
      }
    }
  }
};

#endif // MAIN_MEMORY_HPP