// **********************************************************************
// smicro/src/Tile1Core.cpp
// **********************************************************************
// Sebastian Claudiusz Magierowski Dec 2 2025

#include "Tile1Core.hpp"

using namespace Cascade;

// Simple adapter/shim class that exposes smicro's Dram as a MemoryPort for Tile1 (only viewable in this .cpp)
class Tile1Core::DramMemoryPort : public MemoryPort {
public:
  explicit DramMemoryPort(Dram& dram) : dram_(dram) {}

  uint32_t read32(uint32_t addr) override {
    uint32_t value = 0;
    const uint64_t phys = dram_.get_base() + static_cast<uint64_t>(addr);
    dram_.read(phys, &value, sizeof(value));
    return value;
  }

  void write32(uint32_t addr, uint32_t value) override {
    const uint64_t phys = dram_.get_base() + static_cast<uint64_t>(addr);
    dram_.write(phys, &value, sizeof(value));
  }

private:
  Dram& dram_;
};


Tile1Core::Tile1Core(std::string name, IMPL_CTOR) // Tile1Core constructor implementation
  : tile_("tile1")  // Tile1 has a convenience ctor taking just a name
{
  tile_.clk << clk; // Forward the smicro clock into Tile1 so it can tick with the rest of the system.
}

void Tile1Core::attach_dram(Dram* dram) { // to tell Tile1Core which DRAM instance to use, 
  dram_ = dram;   // remember the DRAM pointer

  // Clean up any existing adapter
  if (dram_port_) {
    delete dram_port_;
    dram_port_ = nullptr;
  }

  // If a valid DRAM is provided, create a new adapter and hook Tile1 to it
  if (dram_) {
    dram_port_ = new DramMemoryPort(*dram_); // re-uses DramMemoryPort defined above, so tick() sees a synchronous memory just like tb_tile1.cpp
    tile_.attach_memory(dram_port_);
  }
}

void Tile1Core::update() {
  tile_.tick();   // for now: just tick Tile1. Memory is handled synchronously via DramMemoryPort.
}

void Tile1Core::reset() {
  // You can add Tile1-specific reset if/when you have one.
}