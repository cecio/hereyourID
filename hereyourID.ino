/********************************************************************
 *
 * hereyourID
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
 * $Id: hereyourID.ino,v 2.5 2016/09/09 16:51:48 cesare Exp cesare $
 *
 **********************************************************************/

// #define DEBUG

#define MAXBUFFER 30

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#define OLED_RESET 6
Adafruit_SSD1306 display(OLED_RESET);

// SoftwareSerial reader(9,10);       // Rx, Tx
SoftwareSerial reader(10,9);       // Rx, Tx

uint8_t read_INFO[] = {0xAA, 0xBB, 0x05, 0x00, 0x00, 0x00, 0x04, 0x01, 0x05};
uint8_t read_RFID[] = {0xAA, 0xBB, 0x05, 0x00, 0x00, 0x00, 0x01, 0x03, 0x02};
uint8_t beep[] = {0xAA, 0xBB, 0x05, 0x00, 0x00, 0x00, 0x06, 0x01, 0x07};

boolean readDone = false;
String animalId = "";
String countryId = "";

// LED
#define LEDPIN 8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LEDPIN, NEO_GRB + NEO_KHZ800);

void setup() {
#ifdef DEBUG
	Serial.begin(9600);
#endif

	reader.begin(19200);

	// Init LED
	strip.begin();
	strip.show(); // Initialize all pixels to 'off'

	// Init display
	// by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
	// init done

	// Show image buffer on the display hardware.
	// Since the buffer is intialized with an Adafruit splashscreen
	// internally, this will display the splashscreen.
	display.display();
	delay(2000);

	// Initial screen
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.clearDisplay();
	display.setCursor(0,0);
	display.println("hereyourID v2.5");
	display.display();
	delay(2000);
}

void loop() {

	uint8_t replyMod[MAXBUFFER];
	uint8_t inByte;
	int i = 0;

	animalId = "";
	countryId = "";

	while ( reader.available() && i < MAXBUFFER - 1 )
	{
		inByte = reader.read();

#ifdef DEBUG
		Serial.print(inByte,HEX);
		Serial.println();
#endif

		replyMod[i] = inByte;
		i++;
	}

	if ( i > 0 )
	{
		// Flush input
		while ( reader.available() ) reader.read();

#ifdef DEBUG
		Serial.println("--------------------------");
#endif

		if ( validCRC(replyMod,i - 1) == true )
		{
#ifdef DEBUG
			Serial.println("CRC OK");
#endif

			if ( validCMD(replyMod) == true )
			{
#ifdef DEBUG
				Serial.println("Command OK");
#endif
				loadId(replyMod,i);

				// Show the red info
				display.clearDisplay();
				display.setTextSize(1);
				display.setCursor(0,0);
				display.print("Country: ");
				display.println(countryId);
				display.print("ID:      ");
				display.println(animalId);
				display.display();

				flashLED(255,0,0,1000,3);
			}
		}
		else
		{
#ifdef DEBUG
			Serial.println("CRC NOT OK");
#endif
		}

		readDone = false;
	}

	if ( readDone == false )
	{
		reader.write(read_RFID, sizeof(read_RFID));
		readDone = true;
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
void loadId( uint8_t *reply, int len )
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
// Flash the led with RGB values, for "millesc" and
// a "num" number of times
//
void flashLED(int r, int g, int b, int millisec, int num)
{
	int i = 0;

	for( i=0; i < num; i++ )
	{
		// Flash LED
		colorWipe(strip.Color(r, g, b), 500); // Turn LED on
		delay(millisec);            // wait for a while
		colorWipe(strip.Color(0, 0, 0), 500); // turn the LED off
		delay(millisec);            // wait for a while
	}
}

// Fill the LED dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
	for(uint16_t i=0; i<strip.numPixels(); i++) {
		strip.setPixelColor(i, c);
		strip.show();
		delay(wait);
	}
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
