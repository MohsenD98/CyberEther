#include "module_impl.hh"

namespace Jetstream::Modules {

Result HadiPassthroughImpl::define() {
    JST_CHECK(defineTaint(Module::Taint::DISCONTIGUOUS));

    JST_CHECK(defineInterfaceInput("signal"));
    JST_CHECK(defineInterfaceOutput("signal"));

    return Result::SUCCESS;
}

Result HadiPassthroughImpl::create() {
    input = inputs().at("signal").tensor;

    JST_CHECK(output.create(input.device(), input.dtype(), input.shape()));
    JST_CHECK(output.propagateAttributes(input));

    outputs()["signal"].produced(name(), "signal", output);

    return Result::SUCCESS;
}

Result HadiPassthroughImpl::reconfigure() {
    return Result::RECREATE;
}

}  // namespace Jetstream::Modules
