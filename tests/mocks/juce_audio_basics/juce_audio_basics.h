#pragma once
#include <vector>
#include <algorithm>

namespace juce {

template <typename Type>
class AudioBuffer {
public:
    AudioBuffer() = default;
    AudioBuffer(int numChannels, int numSamples) {
        setSize(numChannels, numSamples);
    }

    void setSize(int numChannels, int numSamples, bool keepExistingContent = false, bool clearExtraSpace = false, bool avoidReallocating = false) {
        channels.resize((size_t)numChannels, std::vector<Type>((size_t)numSamples, 0.0f));
    }

    int getNumSamples() const noexcept { return channels.empty() ? 0 : (int)channels[0].size(); }
    int getNumChannels() const noexcept { return (int)channels.size(); }

    Type* getWritePointer(int channelIndex) noexcept { return channels[(size_t)channelIndex].data(); }
    const Type* getReadPointer(int channelIndex) const noexcept { return channels[(size_t)channelIndex].data(); }

    void clear() {
        for (auto& ch : channels) std::fill(ch.begin(), ch.end(), 0.0f);
    }

private:
    std::vector<std::vector<Type>> channels;
};

} // namespace juce
