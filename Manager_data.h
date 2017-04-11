#ifndef Manager_data_h
#define Manager_data_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define max_text 80

class Manager_data {
  public:
    void setup();
    void loop();
};


#endif //Manager_Data
