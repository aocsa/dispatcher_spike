#pragma once

#include <iostream>
#include <cassert>
#include "backend.h"

#ifdef __CUDACC__
#define CUDF_SUPPORT  1
#endif

template <typename FnPtr, typename T> struct DispatchStub;


struct DispatchStubImpl {
      void* get_call_ptr(ral::execution::execution_backend backend, void *CPU_BACKEND) {
        if (backend.id() == ral::execution::backend_id::ARROW) {
          cpu_dispatch_ptr = CPU_BACKEND;
          return cpu_dispatch_ptr;
        } else if (backend.id() == ral::execution::backend_id::CUDF) {
          return cuda_dispatch_ptr;
        }
      }
      void* cpu_dispatch_ptr{nullptr};
      void* cuda_dispatch_ptr{nullptr};
};

template <typename rT, typename T, typename... Args>
struct DispatchStub<rT (*)(Args...), T> {
  using FnPtr = rT (*)(Args...);

  DispatchStub() = default;
  DispatchStub(const DispatchStub &) = delete;
  DispatchStub &operator=(const DispatchStub &) = delete;

private:
  FnPtr get_call_ptr(ral::execution::execution_backend backend, void *backend_fn_ptr) {
    return reinterpret_cast<FnPtr>(
            impl.get_call_ptr(backend, reinterpret_cast<void*>(backend_fn_ptr)
          )
    );
  }
public:
  template <typename... ArgTypes>
  rT operator()(ral::execution::execution_backend backend, ArgTypes &&...args) {
    FnPtr call_ptr = nullptr;
    call_ptr = get_call_ptr(backend, reinterpret_cast<void*>(CPU_BACKEND));
    assert(call_ptr != nullptr);
    return (*call_ptr)(std::forward<ArgTypes>(args)...);
  }

  void set_cuda_dispatch_ptr(FnPtr fn_ptr) {
    // std::cerr << "DispatchStub>>CUDA\n";
    impl.cuda_dispatch_ptr = reinterpret_cast<void*>(fn_ptr);
  }

  static FnPtr CPU_BACKEND;
private:
  DispatchStubImpl impl;
};


template <typename FnPtr, typename T>
struct RegisterCUDADispatch {
  RegisterCUDADispatch(DispatchStub<FnPtr, T>& stub, FnPtr value) {
      // std::cerr << "RegisterCUDADispatch>>RegisterCUDADispatch\n";
      stub.set_cuda_dispatch_ptr(value);
  }
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
  static RegisterCUDADispatch<decltype(fn), struct name> name ## __register(name, fn);
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
