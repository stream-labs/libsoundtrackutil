
// Copyright Twitch Interactive, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT

#pragma once

#include "ByteStream.h"
#include <optional>

namespace Twitch::Audio {

enum class SampleFormat {
    Unsigned8Bit,
    Signed16Bit,
    Signed32Bit,
    Float,
};

struct TwitchAudioPacket {
public:
    int version;
    int64_t timestampInNs;

    int channelCount;
    int sampleRate;
    SampleFormat sampleFormat;
    bool isPlanar;

    int frameCount;
    bool silence;
    bool discontinuity;

    // autogenerated
    uint32_t audioDataSize;

    // must always come last since it is chopped from the serialized data
    std::vector<uint8_t> audioData;

    size_t sampleSize() const
    {
        switch(sampleFormat) {
        case SampleFormat::Unsigned8Bit:
            return 1;

        case SampleFormat::Signed16Bit:
            return 2;

        case SampleFormat::Signed32Bit:
        case SampleFormat::Float:
            return 4;

        default:
            return 0;
        }
    }

    size_t frameSize() const
    {
        return sampleSize() * channelCount;
    }

    std::vector<uint8_t> serialize()
    {
        audioDataSize = static_cast<uint32_t>(audioData.size());

        Twitch::Utility::OutputByteStream stream;
        stream.reserve(35 + audioDataSize);
        stream.write<uint32_t>(kVersion); // 0x00 offset
        stream.write<int64_t>(timestampInNs); // 0x04 offset
        stream.write<uint32_t>(channelCount); // 0x0c offset
        stream.write<uint32_t>(sampleRate); // 0x10 offset
        stream.write<uint32_t>(sampleFormat); // 0x14 offset
        stream.write<bool>(isPlanar); // 0x18 offset
        stream.write<uint32_t>(frameCount); // 0x19 offset
        stream.write<bool>(silence); // 0x1d offset
        stream.write<bool>(discontinuity); // 0x1e offset
        stream.write<uint32_t>(audioDataSize); // 0x1f offset
        stream.writeBytes(audioData);

        return stream.consume();
    }

    static std::optional<TwitchAudioPacket> deserialize(const std::vector<uint8_t> &bytes)
    {
        if(bytes.size() < headerSize) {
            return std::nullopt;
        }

        TwitchAudioPacket ret;
        Twitch::Utility::InputByteStream stream(bytes);
        ret.version = stream.read<uint32_t>();
        if(ret.version != kVersion) {
            return std::nullopt;
        }
        ret.timestampInNs = stream.read<int64_t>();
        ret.channelCount = stream.read<uint32_t>();
        ret.sampleRate = stream.read<uint32_t>();
        ret.sampleFormat = static_cast<SampleFormat>(stream.read<uint32_t>());
        ret.isPlanar = stream.read<bool>();
        ret.frameCount = stream.read<uint32_t>();
        ret.silence = stream.read<bool>();
        ret.discontinuity = stream.read<bool>();
        ret.audioDataSize = stream.read<uint32_t>();
        if(stream.remainingSize() < ret.audioDataSize) {
            return std::nullopt;
        }
        ret.audioData.resize(ret.audioDataSize);
        stream.readBytes(ret.audioData);

        return ret;
    }

private:
    static constexpr int kVersion = 1;
    static constexpr size_t headerSize = 35; // size 0x23
};
} // namespace Twitch::Audio
