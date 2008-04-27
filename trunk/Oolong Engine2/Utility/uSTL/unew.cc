// This file is part of the ustl library, an STL implementation.
//
// Copyright (C) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// unew.cc
//

#include "unew.h"
#include <stdlib.h>

void* throwing_malloc (size_t n) throw (ustl::bad_alloc)
{
    void* p = malloc (n);
    if (!p)
	throw ustl::bad_alloc (n);
    return (p);
}

void free_nullok (void* p) throw()
{
    if (p)
	free (p);
}

