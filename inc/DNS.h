#pragma once

#include <functional>
#include <string>
#include <tuple>
#include <vector>

#define LABEL_POINTER_FLAGS 0xc0

namespace DNice {
    enum class Opcode: uint8_t {
        StandardQuery =      0,
        InverseQuery =       1,
        ServerStatus =       2,
        Unassigned =         3,
        Notify =             4,
        Update =             5,
        StatefulOperations = 6,
    };

    enum class ResponseCode: uint8_t {
        NoError =    0,
        FormErr =    1,
        ServFail =   2,
        NXDomain =   3,
        NotImp =     4,
        Refused =    5,
        YXDomain =   6,
        YXRRSet =    7,
        NXRRSet =    8,
        NotAuth =    9,
        NotZone =   10,
        DSOTYPENI = 11,
        Unknown =   12,
    };

    enum class Type: uint16_t {
        A =            1,
        NS =           2,
        MD =           3,
        MF =           4,
        CNAME =        5,
        SOA =          6,
        MB =           7,
        MG =           8,
        MR =           9,
        NULLDATA =    10,
        WKS =         11,
        PTR =         12,
        HINFO =       13,
        MINFO =       14,
        MX =          15,
        TXT =         16,
        RP =          17,
        AFSDB =       18,
        X25 =         19,
        ISDN =        20,
        RT =          21,
        NSAP =        22,
        NsapPtr =     23,
        SIG =         24,
        KEY =         25,
        PX =          26,
        GPOS =        27,
        AAAA =        28,
        LOC =         29,
        NXT =         30,
        EID =         31,
        NIMLOC =      32,
        SRV =         33,
        ATMA =        34,
        NAPTR =       35,
        KX =          36,
        CERT =        37,
        A6 =          38,
        DNAME =       39,
        SINK =        40,
        OPT =         41,
        APL =         42,
        DS =          43,
        SSHFP =       44,
        IPSECKEY =    45,
        RRSIG =       46,
        NSEC =        47,
        DNSKEY =      48,
        DHCID =       49,
        NSEC3 =       50,
        NSEC3PARAM =  51,
        TLSA =        52,
        SMIMEA =      53,
        // 53 is unassigned
        HIP =         55,
        NINFO =       56,
        RKEY =        57,
        TALINK =      58,
        CDS =         59,
        CDNSKEY =     60,
        OPENPGPKEY =  61,
        CSYNC =       62,
        // 62-98 are unassigned
        SPF =         99,
        UINFO =      100,
        UID =        101,
        GID =        102,
        UNSPEC =     103,
        NID =        104,
        L32 =        105,
        L64 =        106,
        LP =         107,
        EUI48 =      108,
        EUI64 =      109,
        // 110-248 are unassigned
        TKEY =       249,
        TSIG =       250,
        IXFR =       251,
        AXFR =       252,
        MAILB =      253,
        MAILA =      254,
        ANY =        255,
        URI =        256,
        CAA =        257,
        AVC =        258,
        DOA =        259,
        // 260-32767 are unassigned
        TA =         32768,
        DLV =        32769,
    };

    enum class Class: uint16_t {
        Internet = 1,
        // 2 is unassigned
        Chaos = 3,
        Hesiod = 4,
        // 5-253 are unassigned
        QclassNone = 254,
        QclassAny = 255,
    };

    struct Label {
        bool isPointer;
        uint16_t pointerAddress;
        std::string domainName;
    };

    struct Question {
        Label label;
        Type qtype;
        Class qclass;
    };

    struct Resource {
        Label label;
        Type rtype;
        Class rclass;
        uint32_t ttl;
        uint16_t length;
        std::vector<uint8_t> data;
    };

    struct Packet {
        uint16_t id;
        bool isResponse;
        Opcode opcode;
        bool isAuthoritative;
        bool isTruncated;
        bool recursionDesired;
        bool recursionAvailable;
        bool zBit;
        bool isAuthenticData;
        bool checkingDisabled;
        ResponseCode responseCode;
        std::vector<Question> questions;
        std::vector<Resource> answers;
        std::vector<Resource> authorities;
        std::vector<Resource> additionalRecords;
    };

    bool parseDnsPacket(const std::vector<uint8_t>& rawPacket, Packet& outPacket, std::string& error);

    void serializeDnsPacket(const Packet& packet, std::vector<uint8_t>& outRawPacket);

    std::string packetToBase64(const Packet& packet);

    std::string resolveLabel(const std::vector<uint8_t>& rawPacket, const Label& label);

    template <typename T>
    size_t collectResources(
        std::vector<T>& receiver,
        const std::function<std::tuple<T, size_t>(const std::vector<uint8_t>&, size_t)>&& parser,
        const std::vector<uint8_t>& data,
        size_t index,
        uint16_t count) {
        auto packetIndex = index;
        for (size_t i = 0; i < count; i++) {
            auto result = parser(data, packetIndex);
            receiver.push_back(std::get<0>(result));
            packetIndex = std::get<1>(result);
        }

        return packetIndex;
    }

    template <typename T>
    T getValue(const std::vector<uint8_t>& bytes, size_t offset) {
        const auto size = sizeof(T);
        T datum = 0;
        for (size_t i = 0; i < size; i++) {
            datum |= (T)bytes[offset + i];
            if (i < size - 1) {
                datum <<= 8;
            }
        }

        return datum;
    }

    template <typename T>
    void pushValue(std::vector<uint8_t>& receiver, T value) {
        const auto size = sizeof(T);
        for (size_t i = 0; i < size; i++) {
            const auto val = (uint8_t)(value >> (size - (i + 1)) * 8);
            receiver.push_back(val);
        }
    }
}
