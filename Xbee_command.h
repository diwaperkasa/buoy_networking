#ifndef __Xbee_command_H_
#define __Xbee_command_H_
#include <Arduino.h>

#define MAX_ARGS 10
#define max_text 100
#define DATABUFFERSIZE 20

class Xbee_cmd
{
  private:
    static char _commandText[DATABUFFERSIZE + 1];
    void parse();

  public:
    boolean get();
    void send(char message[max_text]);
    void setup(Stream &serial);
    boolean cmp(char const* targetcommand);
    static int args[MAX_ARGS];
    static char raw_args[MAX_ARGS];
};

#endif
