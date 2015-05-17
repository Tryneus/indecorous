#ifndef CORO_BLOCKER_HPP_
#define CORO_BLOCKER_HPP_

#include "coro/coro.hpp"
#include "coro/coro_event.hpp"

// TODO: RSI: use better template stuff like in the RPC code

#define _BLOCKER_GET_SPAWN_BODY(args...)    \
  Fn fn; Ret res; \
  static void hook(void* param) { \
    BlockingCall* p = (BlockingCall*)param; \
    p->hook_internal(); \
  } \
  void hook_internal() { \
    res = fn(args); \
  }
#define _BLOCKER_GET_RESULT() \
    coro_event_t done_event(false, false); \
    coro_t::spawn_blocking(BlockingCall::hook, &call, &done_event); \
    done_event.wait(); \
    return call.res;

// Since we don't return until the blocking coroutine completes,
// we shouldn't have to worry about parameters going out of scope
// (unless the user does something stupid elsewhere)
template<typename Ret, typename Fn>
Ret co_blocking(Fn fn) {
  struct BlockingCall {
    _BLOCKER_GET_SPAWN_BODY();
  } call = { fn, Ret() };
  _BLOCKER_GET_RESULT();
}
template<typename Ret, typename Fn, typename P1>
Ret co_blocking(Fn fn, P1 p1) {
  struct BlockingCall { P1* p1;
    _BLOCKER_GET_SPAWN_BODY(*p1);
  } call = { &p1, fn, Ret() };
  _BLOCKER_GET_RESULT();
}
template<typename Ret, typename Fn, typename P1, typename P2>
Ret co_blocking(Fn fn, P1 p1, P2 p2) {
  struct BlockingCall { P1* p1; P2* p2;
    _BLOCKER_GET_SPAWN_BODY(*p1, *p2);
  } call = { &p1, &p2, fn, Ret() };
  _BLOCKER_GET_RESULT();
}
template<typename Ret, typename Fn, typename P1, typename P2, typename P3>
Ret co_blocking(Fn fn, P1 p1, P2 p2, P3 p3) {
  struct BlockingCall { P1* p1; P2* p2; P3* p3;
    _BLOCKER_GET_SPAWN_BODY(*p1, *p2, *p3);
  } call = { &p1, &p2, &p3, fn, Ret() };
  _BLOCKER_GET_RESULT();
}
template<typename Ret, typename Fn, typename P1, typename P2, typename P3, typename P4>
Ret co_blocking(Fn fn, P1 p1, P2 p2, P3 p3, P4 p4) {
  struct BlockingCall { P1* p1; P2* p2; P3* p3; P4* p4;
    _BLOCKER_GET_SPAWN_BODY(*p1, *p2, *p3, *p4);
  } call = { &p1, &p2, &p3, &p4, fn, Ret() };
  _BLOCKER_GET_RESULT();
}
template<typename Ret, typename Fn, typename P1, typename P2, typename P3, typename P4, typename P5>
Ret co_blocking(Fn fn, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {
  struct BlockingCall { P1* p1; P2* p2; P3* p3; P4* p4; P5* p5;
    _BLOCKER_GET_SPAWN_BODY(*p1, *p2, *p3, *p4, *p5);
  } call = { &p1, &p2, &p3, &p4, &p5, fn, Ret() };
  _BLOCKER_GET_RESULT();
}
template<typename Ret, typename Fn, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
Ret co_blocking(Fn fn, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) {
  struct BlockingCall { P1* p1; P2* p2; P3* p3; P4* p4; P5* p5; P6* p6;
    _BLOCKER_GET_SPAWN_BODY(*p1, *p2, *p3, *p4, *p5, *p6);
  } call = { &p1, &p2, &p3, &p4, &p5, &p6, fn, Ret() };
  _BLOCKER_GET_RESULT();
}
template<typename Ret, typename Fn, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
Ret co_blocking(Fn fn, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) {
  struct BlockingCall { P1* p1; P2* p2; P3* p3; P4* p4; P5* p5; P6* p6; P7* p7;
    _BLOCKER_GET_SPAWN_BODY(*p1, *p2, *p3, *p4, *p5, *p6, *p7);
  } call = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, fn, Ret() };
  _BLOCKER_GET_RESULT();
}
template<typename Ret, typename Fn, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
Ret co_blocking(Fn fn, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8) {
  struct BlockingCall { P1* p1; P2* p2; P3* p3; P4* p4; P5* p5; P6* p6; P7* p7; P8* p8;
    _BLOCKER_GET_SPAWN_BODY(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8);
  } call = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, fn, Ret() };
  _BLOCKER_GET_RESULT();
}
template<typename Ret, typename Fn, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
Ret co_blocking(Fn fn, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9) {
  struct BlockingCall { P1* p1; P2* p2; P3* p3; P4* p4; P5* p5; P6* p6; P7* p7; P8* p8; P9* p9;
    _BLOCKER_GET_SPAWN_BODY(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9);
  } call = { &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, fn, Ret() };
  _BLOCKER_GET_RESULT();
}

#undef _BLOCKER_GET_SPAWN_BODY
#undef _BLOCKER_GET_RESULT

#endif
