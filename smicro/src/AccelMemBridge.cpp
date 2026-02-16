// **********************************************************************
// smicro/src/AccelMemBridge.cpp
// **********************************************************************
// Sebastian Claudiusz Magierowski Feb 16 2026

#include "AccelMemBridge.hpp"

using namespace Cascade;

AccelMemBridge::AccelMemBridge(std::string /*name*/, IMPL_CTOR) {
  UPDATE(update).reads(m_resp).writes(m_req);
}

bool AccelMemBridge::can_accept() const {
  return !have_pending_req_ && !waiting_resp_;
}

void AccelMemBridge::start_load32(uint32_t addr) {
  assert_always(can_accept(), "AccelMemBridge::start_load32 called while busy");
  have_pending_req_ = true;
  is_store_ = false;
  addr_ = addr;
  data_ = 0;
}

void AccelMemBridge::start_store32(uint32_t addr, uint32_t data) {
  assert_always(can_accept(), "AccelMemBridge::start_store32 called while busy");
  have_pending_req_ = true;
  is_store_ = true;
  addr_ = addr;
  data_ = data;
}

bool AccelMemBridge::resp_valid() const {
  return resp_valid_;
}

uint32_t AccelMemBridge::resp_data() const {
  return resp_data_;
}

void AccelMemBridge::resp_consume() {
  resp_valid_ = false;
  resp_data_ = 0;
}

void AccelMemBridge::update() {
  // Launch one pending request when request FIFO can accept it.
  if (have_pending_req_ && !m_req.full()) {
    MemReq req{};
    req.addr = static_cast<u64>(addr_);
    req.wdata = static_cast<u64>(data_);
    req.size = static_cast<u16>(4); // 32-bit transfer
    req.write = is_store_;
    req.id = static_cast<u16>(0);

    m_req.push(req);
    have_pending_req_ = false;
    waiting_resp_ = true;
  }

  // Retire one response when waiting and available.
  if (waiting_resp_ && !resp_valid_ && !m_resp.empty()) {
    const MemResp resp = m_resp.pop();
    resp_data_ = static_cast<uint32_t>(resp.rdata);
    resp_valid_ = true;
    waiting_resp_ = false;
  }
}

void AccelMemBridge::reset() {
  have_pending_req_ = false;
  is_store_ = false;
  addr_ = 0;
  data_ = 0;
  waiting_resp_ = false;
  resp_valid_ = false;
  resp_data_ = 0;
}
