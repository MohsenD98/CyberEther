#ifndef MOHSEN_BLK_BLOCK_HH
#define MOHSEN_BLK_BLOCK_HH

#include <jetstream/block.hh>
#include <jetstream/types.hh>

namespace Jetstream::Blocks {

struct MohsenBlk : public Block::Config {
    JST_BLOCK_TYPE(mohsen_blk);
    JST_BLOCK_DOMAIN("Mohsen");
    JST_BLOCK_PARAMS();
    JST_BLOCK_DESCRIPTION(
        "MohsenBLK",
        "Passes each input sample through unchanged.",
        "# MohsenBLK\n"
        "A minimal example plugin block. It reads an F32 or CF32 signal tensor "
        "and writes an identical copy to the output.\n\n"
        "## Inputs\n"
        "- **Signal**: F32 or CF32 input tensor.\n\n"
        "## Outputs\n"
        "- **Signal**: Tensor with the same shape, type, and values as the input."
    );
};

}  // namespace Jetstream::Blocks

#endif  // MOHSEN_BLK_BLOCK_HH
