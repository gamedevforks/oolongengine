// This file is part of the ustl library, an STL implementation.
//
// Copyright (C) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//

#include "stdtest.h"

void TestFStream (void)
{
    fstream fs ("bvt25.std");
    if (!fs)
	cout << "Failed to open bvt25.std" << endl;
    string buf;
    buf.resize (fs.size());
    if (buf.size() != 71)
	cout << "fstream.size() returned " << buf.size() << endl;
    fs.read (buf.begin(), buf.size());
    cout << buf;
    fs.close();
}

StdBvtMain (TestFStream)

