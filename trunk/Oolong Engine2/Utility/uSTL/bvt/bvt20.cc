// This file is part of the ustl library, an STL implementation.
//
// Copyright (C) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//

#include "stdtest.h"

void TestStackAndQueue (void)
{
    stack<vector<int> > s;
    cout << "Testing stack: ";
    for (size_t i = 0; i < 5; ++ i)
	s.push (1 + i);
    cout << "popping: ";
    for (size_t j = 0; j < 5; ++ j) {
	cout << s.top() << ' ';
	s.pop();
    }
    cout << endl;

    queue<vector<int> > q;
    cout << "Testing queue: ";
    for (size_t k = 0; k < 5; ++ k)
	q.push (1 + k);
    cout << "popping: ";
    for (size_t l = 0; l < 5; ++ l) {
	cout << q.front() << ' ';
	q.pop();
    }
    cout << endl;
}

StdBvtMain (TestStackAndQueue)

