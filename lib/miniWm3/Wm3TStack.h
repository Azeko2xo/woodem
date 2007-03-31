// Geometric Tools, Inc.
// http://www.geometrictools.com
// Copyright (c) 1998-2006.  All Rights Reserved
//
// The Wild Magic Library (WM3) source code is supplied under the terms of
// the license agreement
//     http://www.geometrictools.com/License/WildMagic3License.pdf
// and may not be copied or disclosed except in accordance with the terms
// of that agreement.

#ifndef WM3TSTACK_H
#define WM3TSTACK_H

#include "Wm3FoundationLIB.h"

// The class T is either native data or is class data that has the following
// member functions:
//   T::T ()
//   T::T (const T&);
//   T& T::operator= (const T&)

namespace Wm3
{

template <class T>
class TStack
{
public:
    TStack (int iMaxQuantity = 64);
    ~TStack ();

    bool IsEmpty () const;
    bool IsFull () const;
    void Push (const T& rkItem);
    void Pop (T& rkItem);
    void Clear ();
    bool GetTop (T& rkItem) const;

    // for iteration over the stack elements
    int GetQuantity () const;
    const T* GetData () const;

private:
    int m_iMaxQuantity, m_iTop;
    T* m_atStack;
};

#include "Wm3TStack.inl"

}

#endif

