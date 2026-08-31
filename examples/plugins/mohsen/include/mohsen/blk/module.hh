#ifndef MOHSEN_BLK_MODULE_HH
#define MOHSEN_BLK_MODULE_HH

#include <jetstream/module.hh>
#include <jetstream/types.hh>

namespace Jetstream::Modules {

struct MohsenBlk : public Module::Config {
    JST_MODULE_TYPE(mohsen_blk);
    JST_MODULE_PARAMS();
};

}  // namespace Jetstream::Modules

#endif  // MOHSEN_BLK_MODULE_HH
