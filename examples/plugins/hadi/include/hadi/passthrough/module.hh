#ifndef HADI_PASSTHROUGH_MODULE_HH
#define HADI_PASSTHROUGH_MODULE_HH

#include <jetstream/module.hh>
#include <jetstream/types.hh>

namespace Jetstream::Modules {

struct HadiPassthrough : public Module::Config {
    JST_MODULE_TYPE(hadi_passthrough);
    JST_MODULE_PARAMS();
};

}  // namespace Jetstream::Modules

#endif  // HADI_PASSTHROUGH_MODULE_HH
