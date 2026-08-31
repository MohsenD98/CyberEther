#include <mohsen/blk/block.hh>
#include <jetstream/detail/block_impl.hh>

#include <mohsen/blk/module.hh>

namespace Jetstream::Blocks {

struct MohsenBlkImpl : public Block::Impl, public DynamicConfig<Blocks::MohsenBlk> {
    Result define() override;
    Result create() override;

  protected:
    std::shared_ptr<Modules::MohsenBlk> moduleConfig = std::make_shared<Modules::MohsenBlk>();
};

Result MohsenBlkImpl::define() {
    JST_CHECK(defineInterfaceInput("signal", "Signal", "Input samples to pass through."));
    JST_CHECK(defineInterfaceOutput("signal", "Signal", "Output samples, identical to input."));

    return Result::SUCCESS;
}

Result MohsenBlkImpl::create() {
    JST_CHECK(moduleCreate("mohsen_blk", moduleConfig, {
        {"signal", inputs().at("signal")},
    }));
    JST_CHECK(moduleExposeOutput("signal", {"mohsen_blk", "signal"}));

    return Result::SUCCESS;
}

JST_REGISTER_BLOCK(MohsenBlkImpl);

}  // namespace Jetstream::Blocks
