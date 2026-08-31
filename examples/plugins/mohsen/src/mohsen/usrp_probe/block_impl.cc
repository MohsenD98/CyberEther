#include <mohsen/usrp_probe/block.hh>
#include <jetstream/detail/block_impl.hh>
#include <jetstream/logger.hh>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Types.hpp>

#include <cmath>
#include <string>
#include <vector>

namespace Jetstream::Blocks {

namespace {

// USRP families keyed by the `type` field the UHD driver reports during
// enumeration. Several models share one type string, which is why a single
// block covers the whole product line instead of one block per model.
struct UsrpFamily {
    const char* type;
    const char* family;
    const char* models;
};

constexpr UsrpFamily UsrpFamilies[] = {
    {"usrp1", "USRP1",         "USRP1"},
    {"usrp2", "USRP2",         "USRP2, N200, N210"},
    {"b100",  "USRP B100",     "B100"},
    {"b200",  "USRP B-Series", "B200, B210, B200mini, B205mini"},
    {"e3x0",  "USRP E-Series", "E310, E312, E313"},
    {"e320",  "USRP E-Series", "E320"},
    {"n3xx",  "USRP N-Series", "N300, N310, N320, N321"},
    {"x300",  "USRP X-Series", "X300, X310"},
    {"x4xx",  "USRP X-Series", "X410, X440"},
};

const UsrpFamily* FamilyForType(const std::string& type) {
    for (const auto& family : UsrpFamilies) {
        if (type == family.type) {
            return &family;
        }
    }
    return nullptr;
}

std::string LookupOr(const SoapySDR::Kwargs& kwargs,
                     const std::string& key,
                     const std::string& fallback) {
    const auto it = kwargs.find(key);
    return (it != kwargs.end() && !it->second.empty()) ? it->second : fallback;
}

std::string FormatHertz(const double hertz) {
    if (!std::isfinite(hertz)) {
        return "n/a";
    }
    if (std::abs(hertz) >= 1e9) {
        return jst::fmt::format("{:.3f} GHz", hertz / 1e9);
    }
    if (std::abs(hertz) >= 1e6) {
        return jst::fmt::format("{:.3f} MHz", hertz / 1e6);
    }
    if (std::abs(hertz) >= 1e3) {
        return jst::fmt::format("{:.3f} kHz", hertz / 1e3);
    }
    return jst::fmt::format("{:.0f} Hz", hertz);
}

std::string FormatList(const std::vector<std::string>& values) {
    return values.empty() ? "n/a" : jst::fmt::format("{}", jst::fmt::join(values, ", "));
}

std::string FormatRanges(const std::vector<SoapySDR::Range>& ranges) {
    if (ranges.empty()) {
        return "n/a";
    }

    std::vector<std::string> parts;
    parts.reserve(ranges.size());

    for (const auto& range : ranges) {
        auto part = jst::fmt::format("{} to {}",
                                     FormatHertz(range.minimum()),
                                     FormatHertz(range.maximum()));
        const auto step = range.step();
        if (std::isfinite(step) && step > 0.0) {
            part += jst::fmt::format(" (step {})", FormatHertz(step));
        }
        parts.push_back(std::move(part));
    }

    return FormatList(parts);
}

// Rebuilds a device string that can be pasted straight into a Soapy SDR block.
// Only the keys that identify the device are kept, so the string stays usable
// across reboots and USB re-enumeration.
std::string DeviceStringFor(const SoapySDR::Kwargs& kwargs) {
    SoapySDR::Kwargs identity;
    identity["driver"] = "uhd";

    for (const auto& key : {"type", "serial", "addr", "mgmt_addr", "resource", "name"}) {
        const auto it = kwargs.find(key);
        if (it != kwargs.end() && !it->second.empty()) {
            identity[key] = it->second;
        }
    }

    return SoapySDR::KwargsToString(identity);
}

std::vector<std::string> RegisteredDrivers() {
    std::vector<std::string> drivers;
    for (const auto& entry : SoapySDR::Registry::listFindFunctions()) {
        drivers.push_back(entry.first);
    }
    return drivers;
}

bool UhdDriverRegistered() {
    for (const auto& driver : RegisteredDrivers()) {
        if (driver == "uhd") {
            return true;
        }
    }
    return false;
}

std::string MissingDriverReport() {
    return jst::fmt::format(
        "# USRP Probe\n\n"
        "**The `uhd` SoapySDR driver is not registered.** USRP devices cannot be "
        "detected without it, no matter what is plugged in.\n\n"
        "Registered drivers: {}\n\n"
        "## How to fix\n"
        "1. Copy the SoapyUHD module into `lib/SoapySDR/modules0.8/` "
        "(`uhdSupport.dll` on Windows, taken from RadioConda).\n"
        "2. Or point **SoapySDR Module** above straight at the module file.\n"
        "3. Make sure the UHD runtime (`uhd.dll` and its dependencies) is reachable "
        "on the library search path.\n\n"
        "See `docs/usrp-windows-setup.md` for the full procedure.",
        FormatList(RegisteredDrivers()));
}

// Reads the capability ranges of one device. This opens the device, which takes
// exclusive access, so it stays behind the Deep Probe option.
std::string DeepProbeDevice(const SoapySDR::Kwargs& kwargs) {
    SoapySDR::Device* device = nullptr;

    try {
        device = SoapySDR::Device::make(kwargs);
    } catch (const std::exception& e) {
        return jst::fmt::format("- **Deep probe failed:** {}\n", e.what());
    } catch (...) {
        return "- **Deep probe failed:** unknown error.\n";
    }

    if (device == nullptr) {
        return "- **Deep probe failed:** device could not be opened.\n";
    }

    std::string details;

    try {
        const auto channels = device->getNumChannels(SOAPY_SDR_RX);
        details += jst::fmt::format("- **RX Channels:** {}\n", channels);

        if (channels > 0) {
            details += jst::fmt::format("- **Frequency Range:** {}\n",
                                        FormatRanges(device->getFrequencyRange(SOAPY_SDR_RX, 0)));
            details += jst::fmt::format("- **Sample Rates:** {}\n",
                                        FormatRanges(device->getSampleRateRange(SOAPY_SDR_RX, 0)));
            details += jst::fmt::format("- **Bandwidths:** {}\n",
                                        FormatRanges(device->getBandwidthRange(SOAPY_SDR_RX, 0)));
            details += jst::fmt::format("- **Antennas:** {}\n",
                                        FormatList(device->listAntennas(SOAPY_SDR_RX, 0)));

            const auto gain = device->getGainRange(SOAPY_SDR_RX, 0);
            details += jst::fmt::format("- **Gain Range:** {:.1f} to {:.1f} dB\n",
                                        gain.minimum(), gain.maximum());
            details += jst::fmt::format("- **Gain Elements:** {}\n",
                                        FormatList(device->listGains(SOAPY_SDR_RX, 0)));
        }

        details += jst::fmt::format("- **Clock Sources:** {}\n", FormatList(device->listClockSources()));
        details += jst::fmt::format("- **Time Sources:** {}\n", FormatList(device->listTimeSources()));
        details += jst::fmt::format("- **Master Clock Rate:** {}\n", FormatHertz(device->getMasterClockRate()));
    } catch (const std::exception& e) {
        details += jst::fmt::format("- **Deep probe incomplete:** {}\n", e.what());
    } catch (...) {
        details += "- **Deep probe incomplete:** unknown error.\n";
    }

    SoapySDR::Device::unmake(device);

    return details;
}

}  // namespace

struct MohsenUsrpProbeImpl : public Block::Impl,
                             public DynamicConfig<Blocks::MohsenUsrpProbe> {
    Result configure() override;
    Result define() override;
    Result create() override;

  private:
    std::string scan();
};

Result MohsenUsrpProbeImpl::configure() {
    report = scan();

    return Result::SUCCESS;
}

Result MohsenUsrpProbeImpl::define() {
    JST_CHECK(defineInterfaceConfig("modulePath",
                                    "SoapySDR Module",
                                    "Optional explicit path to the SoapyUHD module.",
                                    {{"type", "filepicker"},
                                     {"extensions", Parser::MakeSequence({"dll", "so", "dylib"})}}));

    JST_CHECK(defineInterfaceConfig("filter",
                                    "Extra Filter",
                                    "Additional SoapySDR filter arguments, such as 'serial=31C92AB'.",
                                    {{"type", "text"}}));

    JST_CHECK(defineInterfaceConfig("deepProbe",
                                    "Deep Probe",
                                    "Open each device to read its capabilities. Takes exclusive access.",
                                    {{"type", "bool"}}));

    JST_CHECK(defineInterfaceConfig("rescan",
                                    "Rescan",
                                    "Toggle to run the scan again.",
                                    {{"type", "bool"}}));

    JST_CHECK(defineInterfaceConfig("report",
                                    "Detected Devices",
                                    "Result of the last scan.",
                                    {{"type", "markdown"}}));

    return Result::SUCCESS;
}

Result MohsenUsrpProbeImpl::create() {
    return Result::SUCCESS;
}

std::string MohsenUsrpProbeImpl::scan() {
    if (!modulePath.empty()) {
        const auto error = SoapySDR::loadModule(modulePath);
        if (!error.empty() && !error.ends_with(" already loaded")) {
            JST_ERROR("[USRP_PROBE] Failed to load SoapySDR module '{}': {}", modulePath, error);
            return jst::fmt::format(
                "# USRP Probe\n\n"
                "**Failed to load the SoapySDR module.**\n\n"
                "- Path: `{}`\n"
                "- Error: {}\n",
                modulePath, error);
        }
    }

    SoapySDR::loadModules();

    if (!UhdDriverRegistered()) {
        JST_WARN("[USRP_PROBE] The 'uhd' SoapySDR driver is not registered.");
        return MissingDriverReport();
    }

    SoapySDR::Kwargs args;
    if (!filter.empty()) {
        args = SoapySDR::KwargsFromString(filter);
    }
    args["driver"] = "uhd";

    SoapySDR::KwargsList entries;
    try {
        entries = SoapySDR::Device::enumerate(args);
    } catch (const std::exception& e) {
        JST_ERROR("[USRP_PROBE] Failed to enumerate devices: {}", e.what());
        return jst::fmt::format("# USRP Probe\n\n**Enumeration failed:** {}\n", e.what());
    } catch (...) {
        JST_ERROR("[USRP_PROBE] Failed to enumerate devices.");
        return "# USRP Probe\n\n**Enumeration failed:** unknown error.\n";
    }

    if (entries.empty()) {
        JST_INFO("[USRP_PROBE] No USRP device found with filter '{}'.",
                 SoapySDR::KwargsToString(args));
        return jst::fmt::format(
            "# USRP Probe\n\n"
            "The `uhd` driver is loaded, but **no USRP was found**.\n\n"
            "- Filter used: `{}`\n\n"
            "## Things to check\n"
            "- The device is powered and the USB or Ethernet link is up.\n"
            "- On Windows, the WinUSB driver is installed on the USRP interface (Zadig).\n"
            "- `UHD_IMAGES_DIR` points at the FPGA images, otherwise the device is found "
            "but cannot be opened.\n"
            "- The extra filter is not excluding the device.\n",
            SoapySDR::KwargsToString(args));
    }

    JST_INFO("[USRP_PROBE] Found {} USRP device(s).", entries.size());

    std::string result = jst::fmt::format("# USRP Probe\n\n"
                                          "Found **{}** USRP device(s).\n\n"
                                          "| # | Family | Model | Serial |\n"
                                          "|---|--------|-------|--------|\n",
                                          entries.size());

    for (U64 index = 0; index < entries.size(); index++) {
        const auto& kwargs = entries[index];
        const auto* family = FamilyForType(LookupOr(kwargs, "type", ""));

        result += jst::fmt::format("| {} | {} | {} | {} |\n",
                                   index + 1,
                                   family != nullptr ? family->family : "USRP (unknown family)",
                                   LookupOr(kwargs, "product", LookupOr(kwargs, "name", "unknown")),
                                   LookupOr(kwargs, "serial", "n/a"));
    }

    for (U64 index = 0; index < entries.size(); index++) {
        const auto& kwargs = entries[index];
        const auto type = LookupOr(kwargs, "type", "");
        const auto* family = FamilyForType(type);

        result += jst::fmt::format("\n## {}. {}\n\n",
                                   index + 1,
                                   LookupOr(kwargs, "label", family != nullptr ? family->family : "USRP"));

        if (family != nullptr) {
            result += jst::fmt::format("- **Family:** {} (`type={}`, covers {})\n",
                                       family->family, family->type, family->models);
        } else {
            result += jst::fmt::format("- **Family:** not recognised (`type={}`). The device still "
                                       "works through the Soapy SDR block.\n",
                                       type.empty() ? "missing" : type);
        }

        result += jst::fmt::format("- **Product:** {}\n", LookupOr(kwargs, "product", "n/a"));
        result += jst::fmt::format("- **Serial:** {}\n", LookupOr(kwargs, "serial", "n/a"));

        const auto address = LookupOr(kwargs, "addr", LookupOr(kwargs, "resource", ""));
        if (!address.empty()) {
            result += jst::fmt::format("- **Address:** {}\n", address);
        }

        result += jst::fmt::format("- **Device String:** `{}`\n", DeviceStringFor(kwargs));

        if (deepProbe) {
            result += DeepProbeDevice(kwargs);
        }
    }

    if (!deepProbe) {
        result += "\nEnable **Deep Probe** to read tuning ranges, sample rates, antennas, "
                  "and clock sources from each device.\n";
    }

    return result;
}

JST_REGISTER_BLOCK_NO_MODULES(MohsenUsrpProbeImpl);

}  // namespace Jetstream::Blocks
