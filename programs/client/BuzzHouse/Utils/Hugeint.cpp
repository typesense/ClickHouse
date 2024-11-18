#include "Hugeint.h"
#include "UHugeint.h"

#include <cassert>
#include <cmath>

namespace BuzzHouse
{

static void negateInPlace(hugeint_t & input)
{
    input.lower = std::numeric_limits<uint64_t>::max() - input.lower + 1ull;
    input.upper = -1 - input.upper + (input.lower == 0);
}

static uint8_t positiveHugeintHighestBit(hugeint_t bits)
{
    uint8_t out = 0;
    if (bits.upper)
    {
        out = 64;
        uint64_t up = static_cast<uint64_t>(bits.upper);
        while (up)
        {
            up >>= 1;
            out++;
        }
    }
    else
    {
        uint64_t low = bits.lower;
        while (low)
        {
            low >>= 1;
            out++;
        }
    }
    return out;
}

static bool positiveHugeintIsBitSet(hugeint_t lhs, uint8_t bit_position)
{
    if (bit_position < 64)
    {
        return lhs.lower & (uint64_t(1) << uint64_t(bit_position));
    }
    else
    {
        return static_cast<uint64_t>(lhs.upper) & (uint64_t(1) << uint64_t(bit_position - 64));
    }
}

static hugeint_t positiveHugeintLeftShift(hugeint_t lhs, uint32_t amount)
{
    assert(amount > 0 && amount < 64);
    hugeint_t result;
    result.lower = lhs.lower << amount;
    result.upper = static_cast<int64_t>((static_cast<uint64_t>(lhs.upper) << amount) + (lhs.lower >> (64 - amount)));
    return result;
}

static hugeint_t divModPositive(hugeint_t lhs, uint64_t rhs, uint64_t & remainder)
{
    assert(lhs.upper >= 0);
    // divMod code adapted from:
    // https://github.com/calccrypto/uint128_t/blob/master/uint128_t.cpp

    // initialize the result and remainder to 0
    hugeint_t div_result;
    div_result.lower = 0;
    div_result.upper = 0;
    remainder = 0;

    uint8_t highest_bit_set = positiveHugeintHighestBit(lhs);
    // now iterate over the amount of bits that are set in the LHS
    for (uint8_t x = highest_bit_set; x > 0; x--)
    {
        // left-shift the current result and remainder by 1
        div_result = positiveHugeintLeftShift(div_result, 1);
        remainder <<= 1;
        // we get the value of the bit at position X, where position 0 is the least-significant bit
        if (positiveHugeintIsBitSet(lhs, x - 1))
        {
            // increment the remainder
            remainder++;
        }
        if (remainder >= rhs)
        {
            // the remainder has passed the division multiplier: add one to the divide result
            remainder -= rhs;
            div_result.lower++;
            if (div_result.lower == 0)
            {
                // overflow
                div_result.upper++;
            }
        }
    }
    return div_result;
}

int sign(hugeint_t n)
{
    return ((n > hugeint_t(0)) - (n < hugeint_t(0)));
}

hugeint_t abs(hugeint_t n)
{
    assert(n != NumericLimits<hugeint_t>::Minimum());
    return (n * static_cast<hugeint_t>(sign(n)));
}

static hugeint_t divMod(hugeint_t lhs, hugeint_t rhs, hugeint_t & remainder);

static hugeint_t divModMinimum(hugeint_t lhs, hugeint_t rhs, hugeint_t & remainder)
{
    assert(lhs == NumericLimits<hugeint_t>::Minimum() || rhs == NumericLimits<hugeint_t>::Minimum());
    if (rhs == NumericLimits<hugeint_t>::Minimum())
    {
        if (lhs == NumericLimits<hugeint_t>::Minimum())
        {
            remainder = hugeint_t(0);
            return hugeint_t(1);
        }
        remainder = lhs;
        return hugeint_t(0);
    }

    // Add 1 to minimum and run through divMod again
    hugeint_t result = divMod(NumericLimits<hugeint_t>::Minimum() + hugeint_t(1), rhs, remainder);

    // If the 1 mattered we need to adjust the result, otherwise the remainder
    if (abs(remainder) + hugeint_t(1) == abs(rhs))
    {
        result -= static_cast<hugeint_t>(sign(rhs));
        remainder = hugeint_t(0);
    }
    else
    {
        remainder -= hugeint_t(1);
    }
    return result;
}

static hugeint_t divMod(hugeint_t lhs, hugeint_t rhs, hugeint_t & remainder)
{
    if (rhs == hugeint_t(0))
    {
        remainder = lhs;
        return hugeint_t(0);
    }

    // Check if one of the sides is hugeint_t minimum, as that can't be negated.
    if (lhs == NumericLimits<hugeint_t>::Minimum() || rhs == NumericLimits<hugeint_t>::Minimum())
    {
        return divModMinimum(lhs, rhs, remainder);
    }

    bool lhs_negative = lhs.upper < 0;
    bool rhs_negative = rhs.upper < 0;
    if (lhs_negative)
    {
        negateInPlace(lhs);
    }
    if (rhs_negative)
    {
        negateInPlace(rhs);
    }
    // divMod code adapted from:
    // https://github.com/calccrypto/uint128_t/blob/master/uint128_t.cpp

    // initialize the result and remainder to 0
    hugeint_t div_result;
    div_result.lower = 0;
    div_result.upper = 0;
    remainder.lower = 0;
    remainder.upper = 0;

    uint8_t highest_bit_set = positiveHugeintHighestBit(lhs);
    // now iterate over the amount of bits that are set in the LHS
    for (uint8_t x = highest_bit_set; x > 0; x--)
    {
        // left-shift the current result and remainder by 1
        div_result = positiveHugeintLeftShift(div_result, 1);
        remainder = positiveHugeintLeftShift(remainder, 1);

        // we get the value of the bit at position X, where position 0 is the least-significant bit
        if (positiveHugeintIsBitSet(lhs, x - 1))
        {
            remainder += hugeint_t(1);
        }
        if (remainder >= rhs)
        {
            // the remainder has passed the division multiplier: add one to the divide result
            remainder -= rhs;
            div_result += hugeint_t(1);
        }
    }
    if (lhs_negative ^ rhs_negative)
    {
        negateInPlace(div_result);
    }
    if (lhs_negative)
    {
        negateInPlace(remainder);
    }
    return div_result;
}

static hugeint_t divide(hugeint_t lhs, hugeint_t rhs)
{
    hugeint_t remainder;
    return divMod(lhs, rhs, remainder);
}

static hugeint_t modulo(hugeint_t lhs, hugeint_t rhs)
{
    hugeint_t remainder;
    (void)divMod(lhs, rhs, remainder);
    return remainder;
}

static hugeint_t multiply(hugeint_t lhs, hugeint_t rhs)
{
    hugeint_t result;
    bool lhs_negative = lhs.upper < 0;
    bool rhs_negative = rhs.upper < 0;
    if (lhs_negative)
    {
        negateInPlace(lhs);
    }
    if (rhs_negative)
    {
        negateInPlace(rhs);
    }

#if ((__GNUC__ >= 5) || defined(__clang__)) && defined(__SIZEOF_INT128__)
    __uint128_t left = __uint128_t(lhs.lower) + (__uint128_t(lhs.upper) << 64);
    __uint128_t right = __uint128_t(rhs.lower) + (__uint128_t(rhs.upper) << 64);
    __uint128_t result_i128;
    result_i128 = left * right;
    uint64_t upper = uint64_t(result_i128 >> 64);
    result.upper = int64_t(upper);
    result.lower = uint64_t(result_i128 & 0xffffffffffffffff);
#else
    // multiply code adapted from:
    // https://github.com/calccrypto/uint128_t/blob/master/uint128_t.cpp

    // split values into 4 32-bit parts
    uint64_t top[4] = {uint64_t(lhs.upper) >> 32, uint64_t(lhs.upper) & 0xffffffff, lhs.lower >> 32, lhs.lower & 0xffffffff};
    uint64_t bottom[4] = {uint64_t(rhs.upper) >> 32, uint64_t(rhs.upper) & 0xffffffff, rhs.lower >> 32, rhs.lower & 0xffffffff};
    uint64_t products[4][4];

    // multiply each component of the values
    for (auto x = 0; x < 4; x++)
    {
        for (auto y = 0; y < 4; y++)
        {
            products[x][y] = top[x] * bottom[y];
        }
    }

    // first row
    uint64_t fourth32 = (products[3][3] & 0xffffffff);
    uint64_t third32 = (products[3][2] & 0xffffffff) + (products[3][3] >> 32);
    uint64_t second32 = (products[3][1] & 0xffffffff) + (products[3][2] >> 32);
    uint64_t first32 = (products[3][0] & 0xffffffff) + (products[3][1] >> 32);

    // second row
    third32 += (products[2][3] & 0xffffffff);
    second32 += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);
    first32 += (products[2][1] & 0xffffffff) + (products[2][2] >> 32);

    // third row
    second32 += (products[1][3] & 0xffffffff);
    first32 += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);

    // fourth row
    first32 += (products[0][3] & 0xffffffff);

    // move carry to next digit
    third32 += fourth32 >> 32;
    second32 += third32 >> 32;
    first32 += second32 >> 32;

    // remove carry from current digit
    fourth32 &= 0xffffffff;
    third32 &= 0xffffffff;
    second32 &= 0xffffffff;
    first32 &= 0xffffffff;

    // combine components
    result.lower = (third32 << 32) | fourth32;
    result.upper = (first32 << 32) | second32;
#endif
    if (lhs_negative ^ rhs_negative)
    {
        negateInPlace(result);
    }
    return result;
}

template <class DST>
hugeint_t hugeintConvertInteger(DST input)
{
    hugeint_t result;
    result.lower = static_cast<uint64_t>(input);
    result.upper = (input < 0) * -1;
    return result;
}

hugeint_t::hugeint_t(int64_t value)
{
    auto result = hugeintConvertInteger<int64_t>(value);
    this->lower = result.lower;
    this->upper = result.upper;
}

bool hugeint_t::operator==(const hugeint_t & rhs) const
{
    int lower_equals = this->lower == rhs.lower;
    int upper_equals = this->upper == rhs.upper;
    return lower_equals & upper_equals;
}

bool hugeint_t::operator!=(const hugeint_t & rhs) const
{
    int lower_not_equals = this->lower != rhs.lower;
    int upper_not_equals = this->upper != rhs.upper;
    return lower_not_equals | upper_not_equals;
}

bool hugeint_t::operator<(const hugeint_t & rhs) const
{
    int upper_smaller = this->upper < rhs.upper;
    int upper_equal = this->upper == rhs.upper;
    int lower_smaller = this->lower < rhs.lower;
    return upper_smaller | (upper_equal & lower_smaller);
}

bool hugeint_t::operator<=(const hugeint_t & rhs) const
{
    int upper_smaller = this->upper < rhs.upper;
    int upper_equal = this->upper == rhs.upper;
    int lower_smaller_equals = this->lower <= rhs.lower;
    return upper_smaller | (upper_equal & lower_smaller_equals);
}

bool hugeint_t::operator>(const hugeint_t & rhs) const
{
    int upper_bigger = this->upper > rhs.upper;
    int upper_equal = this->upper == rhs.upper;
    int lower_bigger = this->lower > rhs.lower;
    return upper_bigger | (upper_equal & lower_bigger);
}

bool hugeint_t::operator>=(const hugeint_t & rhs) const
{
    int upper_bigger = this->upper > rhs.upper;
    int upper_equal = this->upper == rhs.upper;
    int lower_bigger_equals = this->lower >= rhs.lower;
    return upper_bigger | (upper_equal & lower_bigger_equals);
}

hugeint_t hugeint_t::operator+(const hugeint_t & rhs) const
{
    return hugeint_t(upper + rhs.upper + ((lower + rhs.lower) < lower), lower + rhs.lower);
}

hugeint_t hugeint_t::operator-(const hugeint_t & rhs) const
{
    return hugeint_t(upper - rhs.upper - ((lower - rhs.lower) > lower), lower - rhs.lower);
}

hugeint_t hugeint_t::operator*(const hugeint_t & rhs) const
{
    hugeint_t result = *this;
    result *= rhs;
    return result;
}

hugeint_t hugeint_t::operator/(const hugeint_t & rhs) const
{
    return divide(*this, rhs);
}

hugeint_t hugeint_t::operator%(const hugeint_t & rhs) const
{
    return modulo(*this, rhs);
}

hugeint_t hugeint_t::operator-() const
{
    hugeint_t input = *this;
    negateInPlace(input);
    return input;
}

hugeint_t hugeint_t::operator>>(const hugeint_t & rhs) const
{
    hugeint_t result;
    uint64_t shift = rhs.lower;
    if (rhs.upper != 0 || shift >= 128)
    {
        return hugeint_t(0);
    }
    else if (shift == 0)
    {
        return *this;
    }
    else if (shift == 64)
    {
        result.upper = (upper < 0) ? -1 : 0;
        result.lower = uint64_t(upper);
    }
    else if (shift < 64)
    {
        // perform lower shift in unsigned integer, and mask away the most significant bit
        result.lower = (uint64_t(upper) << (64 - shift)) | (lower >> shift);
        result.upper = upper >> shift;
    }
    else
    {
        assert(shift < 128);
        result.lower = uint64_t(upper >> (shift - 64));
        result.upper = (upper < 0) ? -1 : 0;
    }
    return result;
}

hugeint_t hugeint_t::operator<<(const hugeint_t & rhs) const
{
    if (upper < 0)
    {
        return hugeint_t(0);
    }
    hugeint_t result;
    uint64_t shift = rhs.lower;
    if (rhs.upper != 0 || shift >= 128)
    {
        return hugeint_t(0);
    }
    else if (shift == 64)
    {
        result.upper = int64_t(lower);
        result.lower = 0;
    }
    else if (shift == 0)
    {
        return *this;
    }
    else if (shift < 64)
    {
        // perform upper shift in unsigned integer, and mask away the most significant bit
        uint64_t upper_shift = ((uint64_t(upper) << shift) + (lower >> (64 - shift))) & 0x7FFFFFFFFFFFFFFF;
        result.lower = lower << shift;
        result.upper = int64_t(upper_shift);
    }
    else
    {
        assert(shift < 128);
        result.lower = 0;
        result.upper = static_cast<int64_t>((lower << (shift - 64)) & 0x7FFFFFFFFFFFFFFF);
    }
    return result;
}

hugeint_t hugeint_t::operator&(const hugeint_t & rhs) const
{
    hugeint_t result;
    result.lower = lower & rhs.lower;
    result.upper = upper & rhs.upper;
    return result;
}

hugeint_t hugeint_t::operator|(const hugeint_t & rhs) const
{
    hugeint_t result;
    result.lower = lower | rhs.lower;
    result.upper = upper | rhs.upper;
    return result;
}

hugeint_t hugeint_t::operator^(const hugeint_t & rhs) const
{
    hugeint_t result;
    result.lower = lower ^ rhs.lower;
    result.upper = upper ^ rhs.upper;
    return result;
}

hugeint_t hugeint_t::operator~() const
{
    hugeint_t result;
    result.lower = ~lower;
    result.upper = ~upper;
    return result;
}

hugeint_t & hugeint_t::operator+=(const hugeint_t & rhs)
{
    *this = *this + rhs;
    return *this;
}
hugeint_t & hugeint_t::operator-=(const hugeint_t & rhs)
{
    *this = *this - rhs;
    return *this;
}
hugeint_t & hugeint_t::operator*=(const hugeint_t & rhs)
{
    *this = multiply(*this, rhs);
    return *this;
}
hugeint_t & hugeint_t::operator/=(const hugeint_t & rhs)
{
    *this = divide(*this, rhs);
    return *this;
}
hugeint_t & hugeint_t::operator%=(const hugeint_t & rhs)
{
    *this = modulo(*this, rhs);
    return *this;
}
hugeint_t & hugeint_t::operator>>=(const hugeint_t & rhs)
{
    *this = *this >> rhs;
    return *this;
}
hugeint_t & hugeint_t::operator<<=(const hugeint_t & rhs)
{
    *this = *this << rhs;
    return *this;
}
hugeint_t & hugeint_t::operator&=(const hugeint_t & rhs)
{
    lower &= rhs.lower;
    upper &= rhs.upper;
    return *this;
}
hugeint_t & hugeint_t::operator|=(const hugeint_t & rhs)
{
    lower |= rhs.lower;
    upper |= rhs.upper;
    return *this;
}
hugeint_t & hugeint_t::operator^=(const hugeint_t & rhs)
{
    lower ^= rhs.lower;
    upper ^= rhs.upper;
    return *this;
}

bool hugeint_t::operator!() const
{
    return *this == hugeint_t(0);
}

hugeint_t::operator bool() const
{
    return *this != hugeint_t(0);
}

void hugeint_t::toString(std::string & res) const
{
    std::string in;
    uint64_t remainder;
    hugeint_t input = *this;

    if (input == NumericLimits<hugeint_t>::Minimum())
    {
        res += "-170141183460469231731687303715884105728";
        return;
    }
    bool negative = input.upper < 0;
    if (negative)
    {
        res += "-";
        negateInPlace(input);
    }
    while (true)
    {
        if (!input.lower && !input.upper)
        {
            break;
        }
        input = divModPositive(input, 10, remainder);
        in.insert(0, std::string(1, static_cast<char>('0' + remainder)));
    }
    if (in.empty())
    {
        // value is zero
        res += "0";
    }
    else
    {
        res += in;
    }
}

}
