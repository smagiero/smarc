// **********************************************************************
// smicro/src/AccelMemBridge.hpp
// **********************************************************************
// Sebastian Claudiusz Magierowski Feb 16 2026
/*
Tiny MemCtrl client shim for accelerators to facilitate more realistic SoC-memory paths for accelerators.

Sits on the MemReq/MemResp protocol boundary (same as RvCore / MemTester).
Accelerator (or other host) calls start_load32/start_store32(), and this bridge turns that into exactly one MemReq and then waits for exactly one MemResp.

host API
  +-------------------- AccelMemBridge --------------------+
->| start_load32()                                         |
->| start_store32()                                        |
->| resp_consume()    update() pushes one MemReq -> m_req  |==>                           |
<-| can_accept()      update() pops one MemResp <- m_resp  |<==                                  |
<-| resp_valid()                                           |
<-| resp_data()                                            |
  +--------------------------------------------------------+

Typical smicro wiring today (SoC.cpp, use_test_driver_ == false):

--- AccelMemBridge --+   +-------------- MemCtrl ---------------+   +-- Dram --
        ,---m_req -->|==>| in_core_req   update_issue()   s_req |==>| s_req 
update()             |   |                                      |   |         
        '--m_resp <--|<==| out_core_resp update_retire() s_resp |<==| s_resp
---------------------+   +--------------------------------------+   +----------

Notes / v1 constraints:
- Single-outstanding: at most one request in flight (plus at most one pending req).
- Blocking-style completion: resp_valid() stays true until resp_consume().
- Stores also wait for a MemResp "ack" (assumes MemCtrl returns a completion resp).
- The Tile1 core in smicro is *not* on this MemCtrl path yet; Tile1Core still talks
  directly to DRAM via its private MemoryPort shim. This bridge exists so accelerators
  can exercise the SoC memory timing path through MemCtrl before the core LSU is ported.
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
  // FifoOutput and FifoInput declare ports backed by FIFO channels
  FifoOutput(MemReq,  m_req);   
  FifoInput (MemResp, m_resp);

  // Host-facing non-blocking API (single outstanding request)
  bool can_accept() const; // true  iff no request is pending/enqueued and no request is in flight

  void start_load32(uint32_t addr);
  void start_store32(uint32_t addr, uint32_t data);

  bool     resp_valid() const;
  uint32_t resp_data() const;
  void     resp_consume();

  void update();
  void reset();

private:
  // One pending request that has not yet been pushed to m_req
  bool have_pending_req_ = false;
  bool is_store_         = false;
  uint32_t addr_         = 0;
  uint32_t data_         = 0;

  // One in-flight request waiting for MemResp
  bool waiting_resp_ = false;

  // Sticky response latch (must be consumed explicitly)
  bool resp_valid_    = false;
  uint32_t resp_data_ = 0;
};
