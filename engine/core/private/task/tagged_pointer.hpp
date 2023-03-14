#pragma once

#include <cstdint>
#include <limits>

namespace violet::core
{
template <typename T>
class tagged_pointer_compression
{
public:
    using value_type = T;

    using tag_type = std::uint16_t;
    using address_type = std::uint64_t;

public:
    tagged_pointer_compression() : m_address(0) {}
    tagged_pointer_compression(value_type* p) { m_address = reinterpret_cast<address_type>(p); }
    tagged_pointer_compression(value_type* p, tag_type tag)
    {
        m_address = reinterpret_cast<address_type>(p);
        m_tag[TAG_INDEX] = tag;
    }

    tag_type next_tag() const
    {
        tag_type next = (m_tag[TAG_INDEX] + 1) & std::numeric_limits<tag_type>::max();
        return next;
    }
    tag_type tag() const { return m_tag[TAG_INDEX]; }

    void pointer(value_type* p)
    {
        tag_type tag = m_tag[TAG_INDEX];
        m_address = reinterpret_cast<address_type>(p);
        m_tag[TAG_INDEX] = tag;
    }

    value_type* pointer() const
    {
        address_type address = m_address & ADDRESS_MASK;
        return reinterpret_cast<value_type*>(address);
    }

    value_type* operator->() { return pointer(); }

    bool operator==(const tagged_pointer_compression& p) const { return m_address == p.m_address; }
    bool operator!=(const tagged_pointer_compression& p) const { return !operator==(p); }

private:
    static constexpr address_type ADDRESS_MASK = 0xFFFFFFFFFFFFUL;
    static constexpr int TAG_INDEX = 3;

    union {
        tag_type m_tag[4];
        address_type m_address;
    };
};

template <typename T>
using tagged_pointer = tagged_pointer_compression<T>;

template <typename T1, typename T2>
tagged_pointer<T1> tagged_pointer_cast(tagged_pointer<T2> p)
{
    tagged_pointer<T1> result(static_cast<T1*>(p.pointer()), p.tag());
    return result;
}
} // namespace violet::core