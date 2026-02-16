// **********************************************************************
// smicro/src/AccelMemBridge.hpp
// **********************************************************************
// Sebastian Claudiusz Magierowski Feb 16 2026
/*
Bridge for accelerator memory traffic through MemCtrl.  That is, a minimial client to facilitate more realistic SoC-memory paths for accelerators.
*/
#pragma once
#include <cascade/Cascade.hpp>
#include "MemTypes.hpp"
#include <cstdint>

class AccelMemBridge : public Component {
  DECLARE_COMPONENT(AccelMemBridge);
public:
  AccelMemBridge(std::string name, COMPONENT_CTOR);

  Clock(clk);
  FifoOutput(MemReq,  m_req);
  FifoInput (MemResp, m_resp);

  // Host-facing non-blocking API (single outstanding request).
  // can_accept() is true iff no request is pending/enqueued and no request is in flight.
  bool can_accept() const;

  void start_load32(uint32_t addr);
  void start_store32(uint32_t addr, uint32_t data);

  bool resp_valid() const;
  uint32_t resp_data() const;
  void resp_consume();

  void update();
  void reset();

private:
  // One pending request that has not yet been pushed to m_req.
  bool have_pending_req_ = false;
  bool is_store_ = false;
  uint32_t addr_ = 0;
  uint32_t data_ = 0;

  // One in-flight request waiting for MemResp.
  bool waiting_resp_ = false;

  // Sticky response latch (must be consumed explicitly).
  bool resp_valid_ = false;
  uint32_t resp_data_ = 0;
};
