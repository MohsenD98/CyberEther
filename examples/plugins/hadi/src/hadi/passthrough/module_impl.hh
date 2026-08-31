#ifndef HADI_PASSTHROUGH_MODULE_IMPL_HH
#define HADI_PASSTHROUGH_MODULE_IMPL_HH

#include <hadi/passthrough/module.hh>
#include <jetstream/detail/module_impl.hh>

namespace Jetstream::Modules {

struct HadiPassthroughImpl : public Module::Impl, public DynamicConfig<HadiPassthrough> {
  public:
    Result define() override;
    Result create() override;
    Result reconfigure() override;

  protected:
    Tensor input;
    Tensor output;
};

}  // namespace Jetstream::Modules

#endif  // HADI_PASSTHROUGH_MODULE_IMPL_HH
