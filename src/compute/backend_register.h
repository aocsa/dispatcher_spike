#pragma once

#include <iostream>
#include <cassert>
#include "backend.h"

#ifdef __CUDACC__
#define CUDF_SUPPORT  1
#endif

template <typename FnPtr, typename T> struct DispatchStub;

template <typename rT, typename T, typename... Args>
struct DispatchStub<rT (*)(Args...), T> {
  using FnPtr = rT (*)(Args...);

  DispatchStub() = default;
  DispatchStub(const DispatchStub &) = delete;
  DispatchStub &operator=(const DispatchStub &) = delete;

public:
  template <typename... ArgTypes>
  rT operator()(ral::execution::execution_backend backend, ArgTypes &&...args) {
    FnPtr call_ptr = nullptr;
    if (backend.id() == ral::execution::backend_id::ARROW){
      call_ptr = CPU_BACKEND;
    } else {
#ifdef __CUDACC__
      call_ptr = CUDA_BACKEND;
#endif
    }
    assert(call_ptr != nullptr);
    return (*call_ptr)(std::forward<ArgTypes>(args)...);
  }

  static FnPtr CPU_BACKEND;
#ifdef __CUDACC__
  static FnPtr CUDA_BACKEND;
#endif
};

#define DECLARE_DISPATCH(fn, name)                                             \
  struct name : DispatchStub<fn, name> {                                       \
    name() = default;                                                          \
    name(const name &) = delete;                                               \
    name &operator=(const name &) = delete;                                    \
  };                                                                           \
  extern struct name name

#define DEFINE_DISPATCH(name) struct name name

#define REGISTER_ARCH_DISPATCH(name, arch, fn)                                 \
  template <> decltype(fn) DispatchStub<decltype(fn), struct name>::arch = fn;

#define REGISTER_CUDA_DISPATCH(name, fn) \
  REGISTER_ARCH_DISPATCH(name, CUDA_BACKEND, fn)
                                         \
#if defined(__CUDACC__)
#define REGISTER_DISPATCH(name, fn)                                            \
  REGISTER_CUDA_DISPATCH(name, fn)
#else
#define REGISTER_DISPATCH(name, fn)                                            \
  REGISTER_ARCH_DISPATCH(name, CPU_BACKEND, fn)
#endif

#define REGISTER_NO_CPU_DISPATCH(name, fn_type)                                \
  REGISTER_ARCH_DISPATCH(name, CPU_BACKEND, static_cast<fn_type>(nullptr))

#define REGISTER_NO_CUDA_DISPATCH(name, fn_type)                                \
  REGISTER_ARCH_DISPATCH(name, CUDA_BACKEND, static_cast<fn_type>(nullptr))