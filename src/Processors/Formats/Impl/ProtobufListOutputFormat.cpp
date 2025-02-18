#include "ProtobufListOutputFormat.h"

#if USE_PROTOBUF
#   include <Formats/FormatFactory.h>
#   include <Formats/ProtobufWriter.h>
#   include <Formats/ProtobufSerializer.h>
#   include <Formats/ProtobufSchemas.h>

namespace DB
{

ProtobufListOutputFormat::ProtobufListOutputFormat(
    WriteBuffer & out_,
    const Block & header_,
    const ProtobufSchemaInfo & schema_info_,
    bool defaults_for_nullable_google_wrappers_,
    const String & google_protos_path)
    : IRowOutputFormat(header_, out_)
    , writer(std::make_unique<ProtobufWriter>(out))
    , descriptor_holder(ProtobufSchemas::instance().getMessageTypeForFormatSchema(
          schema_info_.getSchemaInfo(), ProtobufSchemas::WithEnvelope::Yes, google_protos_path))
    , serializer(ProtobufSerializer::create(
          header_.getNames(),
          header_.getDataTypes(),
          descriptor_holder,
          /* with_length_delimiter = */ true,
          /* with_envelope = */ true,
          defaults_for_nullable_google_wrappers_,
          *writer))
{
}

void ProtobufListOutputFormat::write(const Columns & columns, size_t row_num)
{
    if (row_num == 0)
        serializer->setColumns(columns.data(), columns.size());

    serializer->writeRow(row_num);
}

void ProtobufListOutputFormat::finalizeImpl()
{
    serializer->finalizeWrite();
}

void ProtobufListOutputFormat::resetFormatterImpl()
{
    serializer->reset();
}

void registerOutputFormatProtobufList(FormatFactory & factory)
{
    factory.registerOutputFormat(
        "ProtobufList",
        [](WriteBuffer & buf, const Block & header, const FormatSettings & settings)
        {
            return std::make_shared<ProtobufListOutputFormat>(
                buf,
                header,
                ProtobufSchemaInfo(settings, "Protobuf", header, settings.protobuf.use_autogenerated_schema),
                settings.protobuf.output_nullables_with_google_wrappers,
                settings.protobuf.google_protos_path);
        });
}

}

#else

namespace DB
{
class FormatFactory;
void registerOutputFormatProtobufList(FormatFactory &) {}
}

#endif
