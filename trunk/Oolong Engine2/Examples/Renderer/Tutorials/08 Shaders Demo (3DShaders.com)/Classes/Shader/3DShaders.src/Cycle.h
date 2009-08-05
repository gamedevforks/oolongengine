//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef CYCLE_H
#define CYCLE_H

#include <wx/timer.h>

class TCycle : public wxTimer {
  public:
      void Start();
      void Notify();
      void Stop();
};

#endif
