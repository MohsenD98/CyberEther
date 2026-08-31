#ifndef MOHSEN_USRP_PROBE_BLOCK_HH
#define MOHSEN_USRP_PROBE_BLOCK_HH

#include <jetstream/block.hh>
#include <jetstream/types.hh>

namespace Jetstream::Blocks {

struct MohsenUsrpProbe : public Block::Config {
    std::string modulePath = "";
    std::string filter = "";
    bool deepProbe = false;
    bool rescan = false;
    std::string report = "";

    JST_BLOCK_TYPE(mohsen_usrp_probe);
    JST_BLOCK_DOMAIN("Mohsen");
    JST_BLOCK_NODE_SIZE(XL);
    JST_BLOCK_PARAMS(modulePath, filter, deepProbe, rescan, report);
    JST_BLOCK_DESCRIPTION(
        "USRP Probe",
        "Detects connected USRP devices and reports their capabilities.",
        "# USRP Probe\n"
        "Enumerates every Ettus USRP reachable through the SoapySDR `uhd` driver and "
        "renders the result as a markdown report inside the node. The block has no "
        "signal inputs or outputs; it exists to answer the question *which USRP is "
        "actually connected, and what can it do?* without leaving CyberEther.\n\n"

        "All USRP families share the same SoapySDR driver, so a single block covers "
        "B-Series, X-Series, N-Series, E-Series, and the legacy USRP1/USRP2. The "
        "family is resolved from the `type` field reported during enumeration.\n\n"

        "## Arguments\n"
        "- **SoapySDR Module**: Optional explicit path to the SoapyUHD module "
        "(`uhdSupport.dll` on Windows). Leave empty to rely on the module search path.\n"
        "- **Extra Filter**: Additional SoapySDR filter arguments, for example "
        "`serial=31C92AB` or `type=x300`. The `driver=uhd` key is always applied.\n"
        "- **Deep Probe**: Open each device to read its frequency range, sample rate "
        "range, antennas, gains, and clock sources. This takes exclusive access, so it "
        "fails while another block is streaming from the same device.\n"
        "- **Rescan**: Toggle to run the scan again.\n"
        "- **Report**: The generated markdown report.\n\n"

        "## Useful For\n"
        "- Confirming a USRP is visible to CyberEther before building a flowgraph.\n"
        "- Reading the exact device string to paste into a Soapy SDR block.\n"
        "- Diagnosing a missing SoapyUHD module or a missing WinUSB driver.\n"
        "- Checking the tuning and sample rate limits of the attached hardware.\n\n"

        "## Implementation\n"
        "1. Loads the SoapySDR module explicitly when a path is given, then loads the "
        "modules found on the search path.\n"
        "2. Verifies that the `uhd` find function is registered, and reports a "
        "diagnostic instead of an empty list when it is not.\n"
        "3. Calls `SoapySDR::Device::enumerate` with `driver=uhd` merged with the "
        "extra filter.\n"
        "4. Optionally opens each result to query its capability ranges.\n"
        "5. Renders the collected information as markdown."
    );
};

}  // namespace Jetstream::Blocks

#endif  // MOHSEN_USRP_PROBE_BLOCK_HH
