#pragma once

#include "pch.h"
#include "bit-stream.h"

namespace vgc
{
    /*
     * Compresses the input byte sequence using the Lempel-Ziv-Welch algorithm,
     * adapted for use with the Graphics Interchange Format (GIF).
     * Construct this with two arguments:
     * * func - A function which receievs two arguments:
     * * * the produced code word (as a UINT)
     * * * the length of the code word (as a UINT)
     * * bitDepth - the number of bits in each code word - a number between 1 and 8.
     * After construction, call the += operator to insert a codeword.
     * The compression footer will be added after calling Finish() or when the
     * destructor is invoked.
     *
     * Concurrent access to a single LZW object is not supported.
     */
    template<class CodewordFunc>
    class LZW
    {
        struct CodeTreeNode {
            USHORT next[256];

            void Clear()
            {
                memset(next, 0, sizeof next);
            }

            CodeTreeNode()
            {
                Clear();
            }
        };

        // The callback which receives the new code word.
        CodewordFunc m_func;

        // Current position in the dictionary tree, or -1 if it's undefined.
        int m_treePos;

        // Number of bits in each sample in the input stream
        const UINT m_bitDepth;

        // Current number of bits in each code word.
        UINT m_codeSize;

        // Number of used code words = size of the code tree.
        UINT m_usedCodes;

        // The code tree, stored as an array for efficiency.
        std::vector<CodeTreeNode> m_codeTree;

        // Whether we finished the encoding. Used to stop the destructor
        // from adding the compression footer if Finish() is called. 
        bool m_finished;

        USHORT Next(int source, int label)
        {
            return m_codeTree[source].next[label];
        }

        bool HasNext(int source, int label)
        {
            return Next(source, label) != 0;
        }

        UINT CodewordClear()
        {
            return 1u << m_bitDepth;
        }

        UINT CodewordEndOfInput()
        {
            return CodewordClear() + 1;
        }

        void ClearDictionary()
        {
            m_func(CodewordClear(), m_codeSize);
            m_codeSize = m_bitDepth + 1;
            m_usedCodes = CodewordEndOfInput() + 1;

            m_codeTree.resize(m_usedCodes);
            for (auto& node : m_codeTree)
            {
                node.Clear();
            }

            m_treePos = -1;
        }

        UINT CreateCode()
        {
            m_codeTree.emplace_back();
            return m_usedCodes++;
        }

        void SetDestination(int source, int label, USHORT destination)
        {
            m_codeTree[source].next[label] = destination;
        }

    public:

        LZW(CodewordFunc func, UINT bitDepth) :
            m_func(func),
            m_treePos(-1),
            m_bitDepth(bitDepth),
            m_finished(false)
        {
            ClearDictionary();
        }

        LZW& operator+= (BYTE value)
        {
            if (m_treePos == -1)
            {
                m_treePos = value;
            }
            else if (HasNext(m_treePos, value))
            {
                m_treePos = Next(m_treePos, value);
            }
            else
            {
                m_func((UINT)m_treePos, m_codeSize);
                UINT destination = CreateCode();
                SetDestination(m_treePos, value, destination);

                if (destination == 1u << m_codeSize)
                {
                    m_codeSize++;
                }

                if (m_usedCodes == 4096)
                {
                    ClearDictionary();
                }

                m_treePos = value;
            }

            return *this;
        }

        void Finish()
        {
            if (!m_finished && m_treePos != -1)
            {
                m_finished = true;
                m_func((UINT)m_treePos, m_codeSize);
                m_func(CodewordClear(), m_codeSize);
                m_func(CodewordEndOfInput(), m_bitDepth + 1);
            }
        }

        ~LZW()
        {
            Finish();
        }
    };
}
