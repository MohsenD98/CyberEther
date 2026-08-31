#include "module_impl.hh"

namespace Jetstream::Modules {

Result MohsenBlkImpl::define() {
    JST_CHECK(defineTaint(Module::Taint::DISCONTIGUOUS));

    JST_CHECK(defineInterfaceInput("signal"));
    JST_CHECK(defineInterfaceOutput("signal"));

    return Result::SUCCESS;
}

Result MohsenBlkImpl::create() {
    input = inputs().at("signal").tensor;

    JST_CHECK(output.create(input.device(), input.dtype(), input.shape()));
    JST_CHECK(output.propagateAttributes(input));

    outputs()["signal"].produced(name(), "signal", output);

    return Result::SUCCESS;
}

Result MohsenBlkImpl::reconfigure() {
    return Result::RECREATE;
}

}  // namespace Jetstream::Modules
