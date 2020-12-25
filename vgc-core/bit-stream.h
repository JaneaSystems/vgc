#pragma once

#include "pch.h"

namespace vgc
{
    /*
     * Bit stream wrapper. Used for capturing the output of compression
     * and for writing the contents of an image to a file. Construct it
     * by passing it a function which takes a BYTE as an argument.
     * This function will be called each time a whole byte is added.
     */
    template<class ByteFunc>
    class BitStream
    {
        ByteFunc m_func;

        // The offset will always be in 0..7 before/after member function calls
        UINT m_offset;

        uint64_t m_part;

        void WriteByte(BYTE byte)
        {
            if (m_offset == 0)
            {
                m_func(byte);
            }
            else
            {
                m_part |= (uint64_t)byte << m_offset;
                m_func((BYTE)(m_part & 0xff));
                m_part >>= 8;
            }
        }

    public:
        BitStream(ByteFunc func) : m_func(func), m_offset(0), m_part(0) {}

        /*
         * Write the argument's bit representation.
         */
        template<class T>
        BitStream& operator<<(T arg)
        {
            static_assert(sizeof(T) <= 4);
            static_assert(std::is_integral<T>::value);

            for (UINT i = 0; i < sizeof(T); i++)
            {
                WriteByte((BYTE)(arg & 0xff));
                arg >>= 8;
            }

            return *this;
        }

        /*
         * Write the lowest m bits of the given 32-bit unsigned integer.
         * The number of bits written must be strictly less than 32.
         */
        BitStream& WriteBits(UINT num, UINT m)
        {
            num &= (1u << m) - 1;
            m_part |= (uint64_t)num << m_offset;
            m_offset += m;
            while (m_offset >= 8)
            {
                m_func((BYTE)(m_part & 0xff));
                m_offset -= 8;
                m_part >>= 8;
            }
            return *this;
        }

        /*
         * Return the current bit offset.
         */
        UINT Offset() const
        {
            return m_offset;
        }

        /*
         * If we have unwritten bits, fill the unassigned bits with zeroes and call the
         * receiving function.
         */
        BitStream& Flush()
        {
            if (m_offset)
            {
                m_func((BYTE)(m_part & 0xff));
                m_part = 0;
                m_offset = 0;
            }
            return *this;
        }
    };

    void InsertByteLengthHeaders(std::vector<BYTE>& bytes);
}
