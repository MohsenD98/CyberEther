#include <hadi/passthrough/block.hh>
#include <jetstream/detail/block_impl.hh>

#include <hadi/passthrough/module.hh>

namespace Jetstream::Blocks {

struct HadiPassthroughImpl : public Block::Impl, public DynamicConfig<Blocks::HadiPassthrough> {
    Result define() override;
    Result create() override;

  protected:
    std::shared_ptr<Modules::HadiPassthrough> moduleConfig = std::make_shared<Modules::HadiPassthrough>();
};

Result HadiPassthroughImpl::define() {
    JST_CHECK(defineInterfaceInput("signal", "Signal", "Input samples to pass through."));
    JST_CHECK(defineInterfaceOutput("signal", "Signal", "Output samples, identical to input."));

    return Result::SUCCESS;
}

Result HadiPassthroughImpl::create() {
    JST_CHECK(moduleCreate("hadi_passthrough", moduleConfig, {
        {"signal", inputs().at("signal")},
    }));
    JST_CHECK(moduleExposeOutput("signal", {"hadi_passthrough", "signal"}));

    return Result::SUCCESS;
}

JST_REGISTER_BLOCK(HadiPassthroughImpl);

}  // namespace Jetstream::Blocks
