/**Tested perfectly with Xbee SB2 in API mode
   Doesn't work in XBee AT mode
   Bogor, 6 April 2017
   Diwa Perkasa
   Format receiving data: "header(arguments)"
*/

#include "Xbee_command.h"
#include "XBee.h"
XBee xbee = XBee();

//declare class data here
char Xbee_cmd::_commandText[DATABUFFERSIZE + 1];
int Xbee_cmd::args[MAX_ARGS];
char Xbee_cmd::raw_args[MAX_ARGS];

unsigned int SL = 0x0;
unsigned int SH = 0x0;
uint8_t payload[max_text];
static char dataBuffer[DATABUFFERSIZE + 1]; // buffer command
boolean commandReady = false;

XBeeAddress64 addr64 = XBeeAddress64(SH, SL);
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBRxResponse zbRx = ZBRxResponse();
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

void Xbee_cmd::send(char message[max_text])
{
  memset(payload, 0, sizeof payload);
  strcpy(payload, message);
  xbee.send(zbTx);
  Serial.println(message);
}

void Xbee_cmd::setup(Stream &serial)
{
  xbee.setSerial(serial);
}

boolean getserialdata()
{
  xbee.readPacket();
  if (xbee.getResponse().isAvailable())
  {
    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE)
    {
      xbee.getResponse().getZBRxResponse(zbRx);
      byte dataBufferIndex = 0;
      memset(dataBuffer, 0, sizeof dataBuffer);
      for (int i = 0; i < zbRx.getDataLength(); i++) {
        int incomingbyte = zbRx.getData()[i];
        if (dataBufferIndex == DATABUFFERSIZE) {
          //Oops, our index is pointing to an array element outside our buffer.
          dataBufferIndex = 0;
          break;
        }
        dataBuffer[dataBufferIndex++] = incomingbyte;
        dataBuffer[dataBufferIndex] = 0; //null terminate the C string
      }
      if (strlen(dataBuffer) == 0)
      {
        return 0;
      }
      return 1;
    }
  }
  return 0;
}

boolean Xbee_cmd::get()
{
  memset(_commandText, 0, sizeof _commandText);
  for (int i = 0; i < MAX_ARGS; i++) {
    args[i] = 0;
    memset(raw_args[i], 0, sizeof raw_args[i]);
  }
  commandReady = false;
  if (getserialdata())
  {
    parse();
    commandReady = true;
    return true;
  }
  return false;
}

void Xbee_cmd::parse()
{
  char * pch;
  byte i = 0;
  pch = strtok (dataBuffer, ",()");
  while (pch != NULL) {
    if (i == 0) {
      //this is the command text
      Serial.print(F("cmd>"));
      Serial.print(pch);
      Serial.print('(');
      strcpy(_commandText, pch);
    } else {
      //this is a parameter
      strcpy(raw_args[i], pch);
      args[i] = atoi(pch);
      if (i > 1) {
        Serial.print(',');
      }
      Serial.print(pch);
    }
    i++;
    pch = strtok (NULL, ",()");
  }
  Serial.println(")");
  args[0] = i - 1;
}

boolean Xbee_cmd::cmp(char const* targetcommand)
{
  if (!commandReady)
  {
    return false;
  }
  char* pos = strstr(_commandText, targetcommand);
  if (pos == _commandText)
  { //starts with
    return true;
  }
  return false;
}
