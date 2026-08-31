#ifndef MOHSEN_BLK_MODULE_IMPL_HH
#define MOHSEN_BLK_MODULE_IMPL_HH

#include <mohsen/blk/module.hh>
#include <jetstream/detail/module_impl.hh>

namespace Jetstream::Modules {

struct MohsenBlkImpl : public Module::Impl, public DynamicConfig<MohsenBlk> {
  public:
    Result define() override;
    Result create() override;
    Result reconfigure() override;

  protected:
    Tensor input;
    Tensor output;
};

}  // namespace Jetstream::Modules

#endif  // MOHSEN_BLK_MODULE_IMPL_HH
