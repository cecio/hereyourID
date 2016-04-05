/********************************************************************
*
* OpenRetriever
* 
* Open RFID reader for dogs and other animals ISO11784/5
*
* written by Cesare Pizzi
*
* Notes:
* Reader: http://rfidshop.net/em4100tk4100so-117845em4305-lf-rfid-moduledthink302tmodule-p-464.html
*         302T-Module
* 
* 
* Command format
* --------------
* Header + Length + Device identifier code + Command + Parameters + Checksum
* Header:            2BYTE,0xAABB
* Length:            2BYTE,Identification from the device identifier code to check word, The
*                    first byte is valid,Second byteis reserved 0 
* device identifier code: 2BYTE, 0x0000
* Command:           2BYTE, Identifies the coding command functions
* Parameters:        Command packets (can be empty)
* Checksum:          1BYTE, Device identification to the parameter byte by byte XOR
* 
* Response:
* 
* Header + Length + Device identifier code + Command + Status + Parameters + Checksum
* Header:            2BYTE,0xAABB
* Length:            2BYTE,Identification from the device identifier code to check word, The
*                    first byte is valid,Second byteis reserved 0
* device identifier code: 2BYTE, 0x0000
* Command:           2BYTE, Identifies the coding command functions
* Status :           1BYTE,00 = Command OK, Nonzero = Command failure
* Parameters:        Command packets (can be empty)
* Checksum:          1BYTE, Device identification to the parameter byte by byte XOR
* 
* 
* Animal ID Format:
*                    14 bytes animal ID
*                    Animal ID format: National ID(5 Bytes) +
*                                      Country ID(2 Bytes) +
*                                      DataFlag(1 Byte) +
*                                      AnimalFlag(1 Byte) +
*                                      CRC(2 Bytes) +
*                                      Trailer(3 Bytes)
*                                      
* $Id: OpenRetriever.ino,v 1.4 2016/04/05 21:33:07 cesare Exp cesare $
*
**********************************************************************/

#define DEBUG

#define MAXBUFFER 30

#include <SoftwareSerial.h>

//SoftwareSerial reader(10,11);    // Rx, Tx
SoftwareSerial reader(9,10);       // Rx, Tx

uint8_t read_INFO[] = {0xAA, 0xBB, 0x05, 0x00, 0x00, 0x00, 0x04, 0x01, 0x05};
uint8_t read_RFID[] = {0xAA, 0xBB, 0x05, 0x00, 0x00, 0x00, 0x01, 0x03, 0x02};
uint8_t beep[] = {0xAA, 0xBB, 0x05, 0x00, 0x00, 0x00, 0x06, 0x01, 0x07};

void setup() {
  Serial.begin(9600);
  reader.begin(19200);
}

void loop() {

  uint8_t replyMod[MAXBUFFER];
  String animalId = "";
  String countryId = "";
  
  uint8_t inByte;
  int i = 0;
  
  while ( reader.available() && i < MAXBUFFER - 1 ) 
  {
    inByte = reader.read();
    Serial.print(inByte,HEX);
    Serial.println();

    replyMod[i] = inByte;    
    i++;
  }

  if ( i > 0 )
  {
    // Beep
    // FIXME reader.write(beep, sizeof(beep));
    delay(100);
    
    // Flush input
    while ( reader.available() ) reader.read();

    Serial.println("--------------------------");

    if ( validCRC(replyMod,i - 1) == true )
    {
      Serial.println("CRC OK");
      if ( validCMD(replyMod) == true )
      {
        Serial.println("Command OK");

        loadId(replyMod,i,animalId,countryId);
      }
    } 
    else 
    {
      Serial.println("CRC NOT OK");
    }
  }

#ifdef DEBUG
  if (Serial.available()) {
    char cmd = Serial.read();
    
    if ( cmd == 'b' )
    {
      reader.write(beep, sizeof(beep));
    }
    if ( cmd == 'i' )
    {
      reader.write(read_INFO, sizeof(read_INFO));
    }
    if ( cmd == 'r' )
    {
      reader.write(read_RFID, sizeof(read_RFID));
    }
  }
#endif
}

//
// Validate CRC
//
boolean validCRC( uint8_t *command, int len )
{
  uint8_t crc = 0;
  int i = 0;
  
  // The check is done with a byte by byte XOR from byte number 5 (position 4)
  // to the second last. The last byte is the CRC I need to compare with

  crc = command[4] ^ command[5];
  
  for ( i=6; i < len; i++ )
  {
    crc = crc ^ command[i];
  }

#ifdef DEBUG
  Serial.print("Computed CRC: ");
  Serial.println(crc);
  Serial.print("Received CRC: ");
  Serial.println(command[i]);
#endif

  if ( crc == command[i] )
  {
    return true;
  }
  else
  {
    return false;
  }
}

//
// Validate command result (0 OK, not 0 NOT OK)
//
boolean validCMD( uint8_t *command )
{

#ifdef DEBUG
  Serial.print("Command return code: ");
  Serial.println(command[8]);
#endif

  // Check 9th byte, if 0 OK
  if ( command[8] == 0 )
  {
    return true;
  }
  else
  {
    return false;
  }
}

//
// Load the ID from the received data
//
void loadId( uint8_t *reply, int len, String animalId, String countryId )
{
  int x = 0;
  int i = 0;
  char tmpBuff[MAXBUFFER];
  long long id;
  
  // Start to load from 10th byte

  // The first 5 bytes are the animal ID
  x = 9;
  while ( i < 5 )
  {
    animalId = animalId + String(reply[x],HEX);
    i++;
    x++;

    // If the byte is 0xAA skip the following 0x00
    if ( reply[x] == 0xAA ) x++;
  }
  // Kludge to handle long long
  animalId.toCharArray(tmpBuff,MAXBUFFER);
  id = hexStrToLL(tmpBuff,10);
  sprintf(tmpBuff, "%06ld", id/1000000L);
  sprintf(tmpBuff + strlen(tmpBuff), "%06ld", id%1000000L);
  animalId = String(tmpBuff);
  
#ifdef DEBUG
  Serial.print("Animal ID: ");
  Serial.println(animalId);
#endif

  // The following 2 bytes are the country ID
  i = 0;
  x = 14;
  while ( i < 2 )
  {
    countryId = countryId + String(reply[x],HEX);
    i++;
    x++;

    // If the byte is 0xAA skip the following 0x00
    if ( reply[x] == 0xAA ) x++;
  }
  countryId.toCharArray(tmpBuff,MAXBUFFER);
  countryId = String(strtol(tmpBuff,NULL,16));

#ifdef DEBUG
  Serial.print("Country ID: ");
  Serial.println(countryId);
#endif
}

// 
// HEX to LL (LONG LONG) conversion 
//
long long hexStrToLL( char *str, int len )
{
  long long x = 0;
  int i = 0;
  char c;
  
  while ( i < 10 )
  {
    c = hexCharToDec(*str++);
    if (c < 0)
    {
      break;
    }
    
    x = (x << 4) | c;
    i++;
  } 
  
  return x;
}

char hexCharToDec( char c ) 
{

  if ( isdigit(c) ) 
  {  
    // 0 - 9
    return c - '0';
  } 
  else if ( isxdigit(c) ) 
  { 
    // A-F, a-f
    return (c & 0xF) + 9;
  }
  return -1;
}
