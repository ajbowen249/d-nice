#include "DNS.h"

#include <sstream>

namespace DNice {
    void pushValue(std::vector<uint8_t>& receiver, const std::vector<uint8_t>& value) {
        receiver.insert(receiver.end(), value.begin(), value.end());
    }

    bool getFlag(uint8_t byte, uint8_t index) {
        return (byte & (0x01 << index)) != 0;
    }

    void setFlag(uint8_t& byte, uint8_t index, bool value) {
        uint8_t mask = 0x01 << index;

        if (value) {
            byte |= mask;
        } else {
            byte &= !mask;
        }
    }

    std::tuple<Label, size_t> parseLabel(const std::vector<uint8_t>& bytes, size_t start) {
        // If the first two bits are set, this is a pointer.
        if ((bytes[start] & LABEL_POINTER_FLAGS) == LABEL_POINTER_FLAGS) {
            Label label;
            label.isPointer = true;

            uint16_t address = (uint16_t)(bytes[start] & ~LABEL_POINTER_FLAGS);
            address <<= 8;
            address |= bytes[start + 1];
            label.pointerAddress = address;

            return std::make_tuple(label, start + 2);
        }

        std::stringstream domain;

        auto i = start;
        bool isEmpty = true;

        while (i < bytes.size()) {
            auto len = (size_t)bytes[i];
            if (len == 0) {
                i += 1;
                break;
            }

            if (!isEmpty) {
                domain << '.';
            }

            auto end = i + len;
            i += 1;

            while (i <= end) {
                domain << (char)bytes[i];
                i += 1;
                isEmpty = false;
            }
        }

        Label label;
        label.isPointer = false;
        label.domainName = domain.str();

        return std::make_tuple(label, i);
    }

    void serializeLabel(std::vector<uint8_t>& bytes, const Label& label) {
        if (label.isPointer) {
            pushValue(bytes, label.pointerAddress | LABEL_POINTER_FLAGS);
        } else {
            std::string part;
            std::stringstream domainStream(label.domainName);
            bool isFirst = true;
            while (getline(domainStream, part, '.')) {
                const auto partLength = part.length();

                if (isFirst) {
                    isFirst = false;
                    // Label part lengths are a single byte. However, having the first
                    // two bits of the first byte set signifies a QNAME pointer, so the
                    // actual range is 6 bits, 0-63.
                    if (partLength >= 64) {
                        // IMPROVE: This whole interface could do better with errors.
                        exit(-1);
                    }
                }

                bytes.push_back((uint8_t)partLength);
                pushValue(bytes, std::vector<uint8_t>(part.begin(), part.end()));
            }

            bytes.push_back(0);
        }
    }

    std::tuple<Question, size_t> parseQuestion(const std::vector<uint8_t>& bytes, size_t start) {
        auto index = start;
        auto labelResult = parseLabel(bytes, index);
        index = std::get<1>(labelResult);

        Question question;

        question.qtype = (Type)getValue<uint16_t>(bytes, index);
        index += 2;
        question.qclass = (Class)getValue<uint16_t>(bytes, index);
        index += 2;

        return std::make_tuple(question, index);
    }

    void serializeQuestion(std::vector<uint8_t>& bytes, const Question& question) {
        serializeLabel(bytes, question.label);
        pushValue(bytes, (uint16_t)question.qtype);
        pushValue(bytes, (uint16_t)question.qclass);
    }

    std::tuple<Resource, size_t> parseResource(const std::vector<uint8_t>& bytes, size_t start) {
        Resource resource;

        auto index = start;
        auto labelResult = parseLabel(bytes, index);
        index = std::get<1>(labelResult);

        resource.label = std::get<0>(labelResult);

        resource.rtype = (Type)getValue<uint16_t>(bytes, index);
        index += 2;
        resource.rclass = (Class)getValue<uint16_t>(bytes, index);
        index += 2;

        resource.ttl = getValue<uint32_t>(bytes, index);
        index += 4;

        resource.length = getValue<uint16_t>(bytes, index);
        index += 2;

        for (uint16_t dataIndex = 0; dataIndex < resource.length; dataIndex++) {
            resource.data.push_back(bytes[index]);
            index += 1;
        }

        return std::make_tuple(resource, index);
    }

    void serializeResource(std::vector<uint8_t>& bytes, const Resource& resource) {
        serializeLabel(bytes, resource.label);
        pushValue(bytes, (uint16_t)resource.rtype);
        pushValue(bytes, (uint16_t)resource.rclass);
        pushValue(bytes, resource.ttl);
        pushValue(bytes, resource.length);
        pushValue(bytes, resource.data);
    }

    bool parseDnsPacket(const std::vector<uint8_t>& rawPacket, Packet& outPacket, std::string& error) {
        if (rawPacket.size() < 12) {
            error = "Packet is less than the size of a header.";
            return false;
        }

        outPacket.id = getValue<uint16_t>(rawPacket, 0);

        const auto flagsPart1 = rawPacket[2];
        outPacket.isResponse = getFlag(flagsPart1, 7);
        // The opcode is four bits, running from index 6 to 3. ANDing with 0x78 masks out the bits we need here.
        outPacket.opcode = (Opcode)((flagsPart1 & 0x78) >> 3);
        outPacket.isAuthoritative = getFlag(flagsPart1, 2);
        outPacket.isTruncated = getFlag(flagsPart1, 1);
        outPacket.recursionDesired = getFlag(flagsPart1, 0);

        const auto flagsPart2 = rawPacket[3];

        outPacket.recursionAvailable = getFlag(flagsPart2, 7);
        outPacket.zBit = getFlag(flagsPart2, 6);
        outPacket.isAuthenticData = getFlag(flagsPart2, 5);
        outPacket.checkingDisabled = getFlag(flagsPart2, 4);
        // The response code is the last nibble of this byte, so masking with 0x0f.
        outPacket.responseCode = (ResponseCode)(flagsPart2 & 0x0f);

        const auto questionCount = getValue<uint16_t>(rawPacket, 4);
        const auto answerCount = getValue<uint16_t>(rawPacket, 6);
        const auto authorityCount = getValue<uint16_t>(rawPacket, 8);
        const auto additionalRecordCount = getValue<uint16_t>(rawPacket, 10);

        size_t packetIndex = 12;

        packetIndex = collectResources<Question>(
            outPacket.questions,
            [](const std::vector<uint8_t>& bytes, uint8_t start) { return parseQuestion(bytes, start); },
            rawPacket,
            packetIndex,
            questionCount
        );

        packetIndex = collectResources<Resource>(
            outPacket.answers,
            [](const std::vector<uint8_t>& bytes, uint8_t start) { return parseResource(bytes, start); },
            rawPacket,
            packetIndex,
            answerCount
        );

        packetIndex = collectResources<Resource>(
            outPacket.authorities,
            [](const std::vector<uint8_t>& bytes, uint8_t start) { return parseResource(bytes, start); },
            rawPacket,
            packetIndex,
            authorityCount
        );

        collectResources<Resource>(
            outPacket.additionalRecords,
            [](const std::vector<uint8_t>& bytes, uint8_t start) { return parseResource(bytes, start); },
            rawPacket,
            packetIndex,
            additionalRecordCount
        );

        return true;
    }

    void serializeDnsPacket(const Packet& packet, std::vector<uint8_t>& outRawPacket) {
        pushValue(outRawPacket, packet.id);

        uint8_t flagsPart1 = 0;
        setFlag(flagsPart1, 7, packet.isResponse);
        flagsPart1 |= ((uint8_t)packet.opcode << 3);
        setFlag(flagsPart1, 2, packet.isAuthoritative);
        setFlag(flagsPart1, 1, packet.isTruncated);
        setFlag(flagsPart1, 0, packet.recursionDesired);
        outRawPacket.push_back(flagsPart1);

        uint8_t flagsPart2 = 0;
        setFlag(flagsPart2, 7, packet.recursionAvailable);
        setFlag(flagsPart2, 6, packet.zBit);
        setFlag(flagsPart2, 5, packet.isAuthenticData);
        setFlag(flagsPart2, 4, packet.checkingDisabled);
        flagsPart2 |= ((uint8_t)packet.responseCode & 0x0F);
        outRawPacket.push_back(flagsPart2);

        pushValue(outRawPacket, (uint16_t)packet.questions.size());
        pushValue(outRawPacket, (uint16_t)packet.answers.size());
        pushValue(outRawPacket, (uint16_t)packet.authorities.size());
        pushValue(outRawPacket, (uint16_t)packet.additionalRecords.size());

        for (const auto& question : packet.questions) {
            serializeQuestion(outRawPacket, question);
        }

        for (const auto& answer : packet.answers) {
            serializeResource(outRawPacket, answer);
        }

        for (const auto& authority : packet.authorities) {
            serializeResource(outRawPacket, authority);
        }

        for (const auto& additionalRecord : packet.additionalRecords) {
            serializeResource(outRawPacket, additionalRecord);
        }
    }

    std::string resolveLabel(const std::vector<uint8_t>& rawPacket, const Label& label) {
        Label currentLabel = label;
        while (currentLabel.isPointer) {
            currentLabel = std::get<0>(parseLabel(rawPacket, (size_t)currentLabel.pointerAddress));
        }

        return currentLabel.domainName;
    }
}
