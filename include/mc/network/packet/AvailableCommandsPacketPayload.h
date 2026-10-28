#pragma once

#include "mc/_HeaderOutputPredefine.h"

// auto generated inclusion list
#include "mc/server/commands/CommandPermissionLevel.h"

class AvailableCommandsPacketPayload {
public:
    // AvailableCommandsPacketPayload inner types declare
    // clang-format off
    struct ChainedSubcommandRelationship;
    struct ChainedSubcommandData;
    struct ConstrainedValueData;
    struct EnumData;
    struct ParamData;
    struct OverloadData;
    struct CommandData;
    struct SoftEnumData;
    // clang-format on

    // AvailableCommandsPacketPayload inner types define
    struct ChainedSubcommandRelationship {
    public:
        ::ll::TypedStorage<4, 4, uint> subCommandFirstValue;
        ::ll::TypedStorage<4, 4, uint> subCommandSecondValue;

        ChainedSubcommandRelationship() = default;
        ~ChainedSubcommandRelationship() = default;
        ChainedSubcommandRelationship(ChainedSubcommandRelationship const&) = default;
        ChainedSubcommandRelationship(ChainedSubcommandRelationship&&) = default;
        ChainedSubcommandRelationship& operator=(ChainedSubcommandRelationship const&) = default;
        ChainedSubcommandRelationship& operator=(ChainedSubcommandRelationship&&) = default;
    };

    struct ChainedSubcommandData {
    public:
        ::ll::TypedStorage<8, 32, ::std::string> name;
        ::ll::TypedStorage<8, 24, ::std::vector<::AvailableCommandsPacketPayload::ChainedSubcommandRelationship>> values;

        ChainedSubcommandData() = default;
        ~ChainedSubcommandData() = default;
        ChainedSubcommandData(ChainedSubcommandData const&) = default;
        ChainedSubcommandData(ChainedSubcommandData&&) = default;
        ChainedSubcommandData& operator=(ChainedSubcommandData const&) = default;
        ChainedSubcommandData& operator=(ChainedSubcommandData&&) = default;
    };

    struct ConstrainedValueData {
    public:
        ::ll::TypedStorage<4, 4, uint>                  enumValueSymbol;
        ::ll::TypedStorage<4, 4, uint>                  enumSymbol;
        ::ll::TypedStorage<8, 24, ::std::vector<uchar>> constraints;

        ConstrainedValueData() = default;
        ~ConstrainedValueData() = default;
        ConstrainedValueData(ConstrainedValueData const&) = default;
        ConstrainedValueData(ConstrainedValueData&&) = default;
        ConstrainedValueData& operator=(ConstrainedValueData const&) = default;
        ConstrainedValueData& operator=(ConstrainedValueData&&) = default;
    };

    struct EnumData {
    public:
        ::ll::TypedStorage<8, 32, ::std::string>       name;
        ::ll::TypedStorage<8, 24, ::std::vector<uint>> values;

        EnumData() = default;
        ~EnumData() = default;
        EnumData(EnumData const&) = default;
        EnumData(EnumData&&) = default;
        EnumData& operator=(EnumData const&) = default;
        EnumData& operator=(EnumData&&) = default;
    };

    struct ParamData {
    public:
        ::ll::TypedStorage<8, 32, ::std::string> name;
        ::ll::TypedStorage<4, 4, uint>           parseSymbol;
        ::ll::TypedStorage<1, 1, bool>           optional;
        ::ll::TypedStorage<1, 1, uchar>          paramOptions;

        ParamData() = default;
        ~ParamData() = default;
        ParamData(ParamData const&) = default;
        ParamData(ParamData&&) = default;
        ParamData& operator=(ParamData const&) = default;
        ParamData& operator=(ParamData&&) = default;
    };

    struct OverloadData {
    public:
        ::ll::TypedStorage<8, 24, ::std::vector<::AvailableCommandsPacketPayload::ParamData>> params;
        ::ll::TypedStorage<1, 1, bool>                                                        isChaining;

        OverloadData() = default;
        ~OverloadData() = default;
        OverloadData(OverloadData const&) = default;
        OverloadData(OverloadData&&) = default;
        OverloadData& operator=(OverloadData const&) = default;
        OverloadData& operator=(OverloadData&&) = default;
    };

    struct CommandData {
    public:
        ::ll::TypedStorage<8, 32, ::std::string>                                                 name;
        ::ll::TypedStorage<8, 32, ::std::string>                                                 description;
        ::ll::TypedStorage<2, 2, ushort>                                                         flags;
        ::ll::TypedStorage<1, 1, ::CommandPermissionLevel>                                       permission;
        ::ll::TypedStorage<8, 24, ::std::vector<::AvailableCommandsPacketPayload::OverloadData>> overloads;
        ::ll::TypedStorage<8, 24, ::std::vector<uint>>                                          chainedSubcommandIndexes;
        ::ll::TypedStorage<4, 4, int>                                                            aliasEnum;

        CommandData() = default;
        ~CommandData() = default;
        CommandData(CommandData const&) = default;
        CommandData(CommandData&&) = default;
        CommandData& operator=(CommandData const&) = default;
        CommandData& operator=(CommandData&&) = default;
    };

    struct SoftEnumData {
    public:
        ::ll::TypedStorage<8, 32, ::std::string>                name;
        ::ll::TypedStorage<8, 24, ::std::vector<::std::string>> values;

        SoftEnumData() = default;
        ~SoftEnumData() = default;
        SoftEnumData(SoftEnumData const&) = default;
        SoftEnumData(SoftEnumData&&) = default;
        SoftEnumData& operator=(SoftEnumData const&) = default;
        SoftEnumData& operator=(SoftEnumData&&) = default;
    };

public:
    ::ll::TypedStorage<8, 24, ::std::vector<::std::string>>                              mEnumValues;
    ::ll::TypedStorage<8, 24, ::std::vector<::std::string>>                              mPostfixes;
    ::ll::TypedStorage<8, 24, ::std::vector<::AvailableCommandsPacketPayload::EnumData>> mEnums;
    ::ll::TypedStorage<8, 24, ::std::vector<::std::string>>                              mChainedSubcommandValues;
    ::ll::TypedStorage<8, 24, ::std::vector<::AvailableCommandsPacketPayload::ChainedSubcommandData>>
                                                                                             mChainedSubcommands;
    ::ll::TypedStorage<8, 24, ::std::vector<::AvailableCommandsPacketPayload::CommandData>>  mCommands;
    ::ll::TypedStorage<8, 24, ::std::vector<::AvailableCommandsPacketPayload::SoftEnumData>> mSoftEnums;
    ::ll::TypedStorage<8, 24, ::std::vector<::AvailableCommandsPacketPayload::ConstrainedValueData>> mConstraints;

public:
    AvailableCommandsPacketPayload() = default;
    ~AvailableCommandsPacketPayload() = default;
    AvailableCommandsPacketPayload(AvailableCommandsPacketPayload const&) = default;
    AvailableCommandsPacketPayload(AvailableCommandsPacketPayload&&) = default;
    AvailableCommandsPacketPayload& operator=(AvailableCommandsPacketPayload const&) = default;
    AvailableCommandsPacketPayload& operator=(AvailableCommandsPacketPayload&&) = default;
};
