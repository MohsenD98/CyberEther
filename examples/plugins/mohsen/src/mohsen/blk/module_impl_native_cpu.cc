#include <functional>

#include <jetstream/module_context.hh>
#include <jetstream/registry.hh>
#include <jetstream/runtime_context_native_cpu.hh>
#include <jetstream/scheduler_context.hh>
#include <jetstream/tools/automatic_iterator.hh>

#include "module_impl.hh"

namespace Jetstream::Modules {

struct MohsenBlkImplNativeCpu : public MohsenBlkImpl,
                                public NativeCpuRuntimeContext,
                                public Scheduler::Context {
  public:
    Result create() final;

    Result computeSubmit() override;

  private:
    Result kernelF32();
    Result kernelCF32();

    std::function<Result()> kernel;
};

Result MohsenBlkImplNativeCpu::create() {
    JST_CHECK(MohsenBlkImpl::create());

    if (input.dtype() == DataType::F32 && output.dtype() == DataType::F32) {
        kernel = [this]() { return kernelF32(); };
        return Result::SUCCESS;
    }

    if (input.dtype() == DataType::CF32 && output.dtype() == DataType::CF32) {
        kernel = [this]() { return kernelCF32(); };
        return Result::SUCCESS;
    }

    JST_ERROR("[MOHSEN_BLK_NATIVE_CPU] Unsupported data type '{}'.", input.dtype());
    return Result::ERROR;
}

Result MohsenBlkImplNativeCpu::computeSubmit() {
    return kernel();
}

Result MohsenBlkImplNativeCpu::kernelF32() {
    return AutomaticIterator<F32, F32>(
        [](const auto& in, auto& out) {
            out = in;
        },
    input, output);
}

Result MohsenBlkImplNativeCpu::kernelCF32() {
    return AutomaticIterator<CF32, CF32>(
        [](const auto& in, auto& out) {
            out = in;
        },
    input, output);
}

JST_REGISTER_MODULE(MohsenBlkImplNativeCpu, DeviceType::CPU, RuntimeType::NATIVE, "generic");

}  // namespace Jetstream::Modules
