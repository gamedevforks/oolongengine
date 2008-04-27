// This file is part of the ustl library, an STL implementation.
//
// Copyright (C) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// \file stdtest.h
// \brief Standard BVT harness.
//

#ifndef STDTEST_H_4980DC406FEEBBDF2235E42C4EFF1A4E
#define STDTEST_H_4980DC406FEEBBDF2235E42C4EFF1A4E

#include <ustl.h>
using namespace ustl;

typedef void (*stdtestfunc_t)(void);

int StdTestHarness (stdtestfunc_t testFunction);

#define StdBvtMain(function)	int main (void) { return (StdTestHarness (&function)); }

#endif

