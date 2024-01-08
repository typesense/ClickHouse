#pragma once

#include <DataTypes/DataTypeNullable.h>
#include <DataTypes/DataTypeNothing.h>
#include <Columns/IColumn.h>
#include <AggregateFunctions/IAggregateFunction.h>
#include <IO/ReadHelpers.h>
#include <IO/WriteHelpers.h>
#include "DataTypes/IDataType.h"


namespace DB
{
struct Settings;

namespace ErrorCodes
{
    extern const int INCORRECT_DATA;
}


/** Aggregate function that takes arbitrary number of arbitrary arguments and does nothing.
  */
class AggregateFunctionNothing final : public IAggregateFunctionHelper<AggregateFunctionNothing>
{
public:
    static constexpr auto name = "nothing";

    AggregateFunctionNothing(const DataTypes & arguments, const Array & params, const DataTypePtr & result_type_)
        : IAggregateFunctionHelper<AggregateFunctionNothing>(arguments, params, result_type_) {}

    String getName() const override
    {
        return name;
    }

    bool allocatesMemoryInArena() const override { return false; }

    void create(AggregateDataPtr __restrict) const override
    {
    }

    void destroy(AggregateDataPtr __restrict) const noexcept override
    {
    }

    bool hasTrivialDestructor() const override
    {
        return true;
    }

    size_t sizeOfData() const override
    {
        return 0;
    }

    size_t alignOfData() const override
    {
        return 1;
    }

    void add(AggregateDataPtr __restrict, const IColumn **, size_t, Arena *) const override
    {
    }

    void merge(AggregateDataPtr __restrict, ConstAggregateDataPtr, Arena *) const override
    {
    }

    void serialize(ConstAggregateDataPtr __restrict, WriteBuffer & buf, std::optional<size_t>) const override
    {
        writeChar('\0', buf);
    }

    void deserialize(AggregateDataPtr __restrict, ReadBuffer & buf, std::optional<size_t>, Arena *) const override
    {
        [[maybe_unused]] char symbol;
        readChar(symbol, buf);
        if (symbol != '\0')
            throw Exception(ErrorCodes::INCORRECT_DATA, "Incorrect state of aggregate function 'nothing', it should contain exactly one zero byte, while it is {}.", static_cast<UInt32>(symbol));
    }

    void insertResultInto(AggregateDataPtr __restrict, IColumn & to, Arena *) const override
    {
        to.insertDefault();
    }
};

}
