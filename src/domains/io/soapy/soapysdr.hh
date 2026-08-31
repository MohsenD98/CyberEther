#ifndef JETSTREAM_DOMAINS_IO_SOAPY_SOAPYSDR_HH
#define JETSTREAM_DOMAINS_IO_SOAPY_SOAPYSDR_HH

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Types.hpp>

#include <jetstream/logger.hh>
#include <jetstream/types.hh>

namespace Jetstream::Modules {

inline bool SoapyRangeContains(const std::vector<SoapySDR::Range>& ranges, const F32 value) {
    for (const auto& range : ranges) {
        const F32 minimum = static_cast<F32>(range.minimum());
        const F32 maximum = static_cast<F32>(range.maximum());
        if (value < minimum || value > maximum) {
            continue;
        }

        const double step = range.step();
        if (!std::isfinite(step) || step <= 0.0) {
            return true;
        }

        const double stepCount = std::round((static_cast<double>(value) - range.minimum()) / step);
        const F32 closest = static_cast<F32>(range.minimum() + stepCount * step);
        if (value == closest) {
            return true;
        }
    }
    return false;
}

// Returns the closest value the device can actually accept. Devices with a hard
// lower bound (e.g. the USRP X3x0, whose 200 MHz master clock and maximum decimation
// of 512 put the floor at 390.625 kHz) would otherwise reject a request outright.
inline bool SoapyRangeNearest(const std::vector<SoapySDR::Range>& ranges, const F32 value, F32& nearest) {
    if (!std::isfinite(value)) {
        return false;
    }

    bool found = false;
    F64 bestDistance = 0.0;
    F64 best = 0.0;

    for (const auto& range : ranges) {
        const F64 minimum = range.minimum();
        const F64 maximum = range.maximum();
        if (!std::isfinite(minimum) || !std::isfinite(maximum) || maximum < minimum) {
            continue;
        }

        F64 candidate = std::min(std::max(static_cast<F64>(value), minimum), maximum);

        const F64 step = range.step();
        if (std::isfinite(step) && step > 0.0) {
            const F64 stepCount = std::round((candidate - minimum) / step);
            const F64 maximumStepCount = std::floor((maximum - minimum) / step);
            candidate = minimum + std::min(std::max(stepCount, 0.0), maximumStepCount) * step;
        }

        const F64 distance = std::abs(candidate - static_cast<F64>(value));
        if (!found || distance < bestDistance) {
            found = true;
            bestDistance = distance;
            best = candidate;
        }
    }

    if (found) {
        nearest = static_cast<F32>(best);
    }
    return found;
}

// Human-readable capability summary, so an out-of-range request tells the user what
// the device would have accepted.
inline std::string SoapyRangesToString(const std::vector<SoapySDR::Range>& ranges) {
    if (ranges.empty()) {
        return "none reported";
    }

    std::string description;
    for (const auto& range : ranges) {
        if (!description.empty()) {
            description += ", ";
        }
        if (range.minimum() == range.maximum()) {
            description += jst::fmt::format("{:.6g} MHz", range.minimum() / 1e6);
            continue;
        }
        description += jst::fmt::format("{:.6g}-{:.6g} MHz", range.minimum() / 1e6, range.maximum() / 1e6);
        if (std::isfinite(range.step()) && range.step() > 0.0) {
            description += jst::fmt::format(" (step {:.6g} MHz)", range.step() / 1e6);
        }
    }
    return description;
}

struct JETSTREAM_API SoapyDiscovery {
    using Clock = std::chrono::steady_clock;
    using DeviceEntry = std::map<std::string, std::string>;
    using DeviceList = std::map<std::string, DeviceEntry>;

    static DeviceList ListDevices(const std::string& filter = "", bool refresh = false,
                                  std::optional<Clock::time_point> now = std::nullopt);
    static void ClearDiscoveryCache();
    static Result LoadDriverLibrary(const std::string& path);
    static DeviceList BuildDeviceList(const SoapySDR::KwargsList& entries);
};

class JETSTREAM_API SoapyReceiver {
 public:
    enum class ReadStatus {
        Samples,
        Timeout,
        Overflow,
        Error,
    };

    struct ReadResult {
        ReadStatus status = ReadStatus::Error;
        std::size_t sampleCount = 0;
        std::optional<I64> timestampNs;
        std::string error;
    };

    SoapyReceiver() = default;
    ~SoapyReceiver();
    SoapyReceiver(const SoapyReceiver&) = delete;
    SoapyReceiver& operator=(const SoapyReceiver&) = delete;

    Result open(const SoapySDR::Kwargs& args);
    Result validateSettings(F32 sampleRate, F32 frequency) const;
    const std::vector<std::string>& listAntennas() const;
    Result setAntenna(const std::string& antenna);
    Result setTunerFrequency(F32 frequency);
    Result setSampleRate(F32 sampleRate);
    // Devices may coerce a request to the nearest achievable value (e.g. a USRP B2xx
    // re-selecting its master clock), so callers read back what actually took effect.
    std::optional<F32> getTunerFrequency() const;
    std::optional<F32> getSampleRate() const;
    // Closest rate the device can accept, or nullopt when the request is already
    // supported or the device reports no usable ranges.
    std::optional<F32> nearestSampleRate(F32 sampleRate) const;
    std::string describeSampleRateRanges() const;
    std::string describeFrequencyRanges() const;
    Result setAutomaticGain(bool automaticGain);
    Result setBiasTee(bool enabled);
    Result startStream(const SoapySDR::Kwargs& args);
    void reset();

    ReadResult read(std::span<CF32> samples);

 private:
    enum class State {
        Closed,
        Open,
        StreamReady,
        Streaming,
    };

    Result queryCapabilities();

    State state = State::Closed;
    SoapySDR::Device* device = nullptr;
    SoapySDR::Stream* stream = nullptr;
    std::vector<SoapySDR::Range> sampleRateRanges;
    std::vector<SoapySDR::Range> frequencyRanges;
    std::vector<std::string> antennas;
    bool biasTeeSupported = false;
    bool biasTeeNeedsCleanup = false;
};

struct JETSTREAM_API SoapyReceiveStatus {
    enum class Action {
        Samples,
        Retry,
        Fail,
    };

    Action handle(SoapyReceiver::ReadStatus status);

    U64 deviceOverflows = 0;
};

}  // namespace Jetstream::Modules

#endif  // JETSTREAM_DOMAINS_IO_SOAPY_SOAPYSDR_HH
