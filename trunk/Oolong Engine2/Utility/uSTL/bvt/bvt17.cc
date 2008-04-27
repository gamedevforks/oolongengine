// 011010011001011001011000100100
// This file is part of the ustl library, an STL implementation.
//
// Copyright (C) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//

#include "stdtest.h"

void TestBitset (void)
{
    bitset<30> bs1;
    cout << "bitset<" << bs1.size() << "> bs1: capacity() = " << bs1.capacity() << ", sizeof() = " << sizeof(bs1) << endl;
    cout << bs1 << endl;
    bs1.set();
    bs1.set (6, false);
    cout << bs1 << endl;
    bs1.flip();
    cout << bs1 << endl;
    bs1.flip();
    cout << bs1 << endl;

    bs1.reset();
    string comment;	// See line 0 in this file
    cin >> comment >> bs1;
    cout << bs1 << endl;
    cout << "count = " << bs1.count() << endl;

    bs1.reset();
    cout << bs1 << endl;
    cout << "any = " << bs1.any() << ", none = " << bs1.none() << ", count = " << bs1.count() << endl;
    bs1.flip();
    cout << bs1 << endl;
    cout << "any = " << bs1.any() << ", none = " << bs1.none() << ", count = " << bs1.count() << endl;
    bs1.reset();
    bs1.set (4);
    bs1.set (7);
    bs1.set (8);
    cout << bs1 << endl;
    cout << "test(7) == " << bs1.test(7);
    cout << ", [9] = " << bs1[9];
    cout << ", [8] = " << bs1[8] << endl;
    cout << "any = " << bs1.any() << ", none = " << bs1.none() << ", count = " << bs1.count() << endl;
    cout << "~bs1 == " << ~bs1 << endl;
    cout << "to_value == 0x" << ios::hex << bs1.to_value() << ios::dec << endl;

    bitset<70> bs2 ("0101101");
    cout << "bitset<" << bs2.size() << "> bs2: capacity() = " << bs2.capacity() << ", sizeof() = " << sizeof(bs2) << endl;
    cout << bs2 << endl;
    bs2.set (34, 40, 13);
    cout << "bs2.set(34,40,13)" << endl;
    cout << bs2 << endl;
    cout << "bs2.at(34,40) = " << bs2.at(34,40) << endl;

    bitset<256> bs3 (0x3030);
    cout << "bitset<" << bs3.size() << "> bs3: capacity() = " << bs3.capacity() << ", sizeof() = " << sizeof(bs3) << endl;
    cout << "bs3.to_value() == 0x" << ios::hex << bs3.to_value() << ios::dec << endl;

    bitset<30> bs4 (bs1);
    if (bs1 == bs4)
	cout << "bs4 == bs1" << endl;

    bs4 = 0x50505050;
    cout << "bs4 = 0x50505050: " << bs4 << endl;
    bs1 = 0x30303030;
    cout << "bs1 = 0x30303030: " << bs1 << endl;
    bs4 &= bs1;
    cout << "bs4 &= bs1; bs4 = " << bs4 << endl;
    bs4 = 0x50505050;
    bs4 &= bs1;
    cout << "bs4 & bs1;  bs4 = " << bs4 << endl;
    bs4 = 0x50505050;
    bs4 |= bs1;
    cout << "bs4 |= bs1; bs4 = " << bs4 << endl;
    bs4 = 0x50505050;
    bs4 = bs4 | bs1;
    cout << "bs4 | bs1;  bs4 = " << bs4 << endl;
    bs4 = 0x50505050;
    bs4 ^= bs1;
    cout << "bs4 ^= bs1; bs4 = " << bs4 << endl;
    bs4 = 0x50505050;
    bs4 = bs4 ^ 0x30303030;
    cout << "bs4 ^ bs1;  bs4 = " << bs4 << endl;
}

StdBvtMain (TestBitset)

