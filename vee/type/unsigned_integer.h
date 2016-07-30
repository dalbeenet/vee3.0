#pragma once
#ifndef _VEE_TYPE_UNSIGNED_INTEGER_
#define _VEE_TYPE_UNSIGNED_INTEGER_

#include <vee/mpl.h>

namespace vee {

/* Generic N-byte unsigned integer class for Little endian system */
template <size_t TypeSize>
class unsigned_integer final
{
public:
    using this_t = unsigned_integer<TypeSize>;
    using ref_t = this_t&;
    using rref_t = this_t&&;
    static const int size = TypeSize;
    unsigned_integer() = default;
    template <typename T>
    unsigned_integer(T&& value)
    {
        this->operator=(std::forward<T>(value));
    }
    template <
        typename ReferType,
        typename ValueType = typename std::decay<ReferType>::type
    >
        ref_t __thiscall operator=(ReferType&& rhs)
    {
        return assign(std::forward<ReferType>(rhs), mpl::binary_dispatch<TypeSize <= sizeof(ValueType)>{});
    }
    template <typename T>
    operator T() const
    {
        return casting<T>(mpl::binary_dispatch< TypeSize <= sizeof(T) >{});
    }
    template<
        typename T,
        size_t Offset = sizeof(T) - TypeSize
    >
        T casting(mpl::binary_dispatch<true> /* TypeSize <= sizeof(T) */) const
    {
        T temp;
        memset(&temp, 0, sizeof(temp));
        // On the Little endian system
        memmove(&temp, data, TypeSize);
        // On the Big endian system
        //memmove(&temp + Offset, data, TypeSize);
        return temp;
    }
    template<
        typename T,
        size_t Offset = TypeSize - sizeof(T)
    >
        T casting(mpl::binary_dispatch<false> /* TypeSize > sizeof(T) */) const
    {
        T temp;
        memset(&temp, 0, sizeof(temp));
        // On the Little endian system
        memmove(&temp, data, sizeof(temp));
        // On the Big endian system
        //memmove(&temp + Offset, data, sizeof(temp));
        return temp;
    }
private:
    template <
        typename ReferType,
        typename ValueType = typename std::decay<ReferType>::type,
        //x typename ValueType = typename std::enable_if< std::is_integral<typename std::decay<ReferType>::type>::value, typename std::remove_reference<ReferType>::type >::type,
        size_t Offset = sizeof(ValueType) - TypeSize,
        //x typename = typename std::enable_if< std::is_unsigned<ValueType>::value >::type,
        typename = typename std::enable_if< (TypeSize <= sizeof(ValueType)) >::type
    >
        ref_t __thiscall assign(ReferType&& rhs, vee::binary_dispatch<true> /* TypeSize <= sizeof(ValueType) */)
    {
        // On the Little endian system
        memmove(data, &rhs, size);
        // On the Big endian system
        // memmove(data, &rhs + Offset, size);
        return *this;
    }
    template <
        typename ReferType,
        typename ValueType = typename std::decay<ReferType>::type,
        //x typename ValueType = typename std::enable_if< std::is_integral<typename std::decay<ReferType>::type>::value, typename std::remove_reference<ReferType>::type >::type,
        size_t Offset = TypeSize - sizeof(ValueType),
        //x typename = typename std::enable_if< std::is_unsigned<ValueType>::value >::type,
        typename = typename std::enable_if< (TypeSize > sizeof(ValueType)) >::type
    >
        ref_t __thiscall assign(ReferType&& rhs, vee::binary_dispatch<false> /* TypeSize > sizeof(ValueType) */)
    {
        memset(data, 0, size);
        // On the Little endian system
        memmove(data, &rhs, sizeof(ValueType));
        // On the Big endian system
        // memmove(data + Offset, &rhs, sizeof(ValueType));

        return *this;
    }
private:
    uint8_t data[size]{ 0, }; // signed �� size -1 bit�� �����ͷ� ����ϴ� �������� ���� �� ���� �� ����.
};

} // !namespace vee

#endif // !_VEE_TYPE_UNSIGNED_INTEGER_