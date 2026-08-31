#ifndef HADI_PASSTHROUGH_BLOCK_HH
#define HADI_PASSTHROUGH_BLOCK_HH

#include <jetstream/block.hh>
#include <jetstream/types.hh>

namespace Jetstream::Blocks {

struct HadiPassthrough : public Block::Config {
    JST_BLOCK_TYPE(hadi_passthrough);
    JST_BLOCK_DOMAIN("Hadi");
    JST_BLOCK_PARAMS();
    JST_BLOCK_DESCRIPTION(
        "Hadi",
        "Passes each input sample through unchanged.",
        "# Hadi\n"
        "A minimal example plugin block. It reads an F32 or CF32 signal tensor "
        "and writes an identical copy to the output.\n\n"
        "## Inputs\n"
        "- **Signal**: F32 or CF32 input tensor.\n\n"
        "## Outputs\n"
        "- **Signal**: Tensor with the same shape, type, and values as the input."
    );
};

}  // namespace Jetstream::Blocks

#endif  // HADI_PASSTHROUGH_BLOCK_HH
