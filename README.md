# hereyourID
Low cost, Arduino based, RFID tag reader for dogs and other animals

Open RFID reader for dogs and other animals ISO11784/5

written by Cesare Pizzi

Notes:
Reader: http://rfidshop.net/em4100tk4100so-117845em4305-lf-rfid-moduledthink302tmodule-p-464.html
        302T-Module


Command format
--------------
Header + Length + Device identifier code + Command + Parameters + Checksum

Header:            2BYTE,0xAABB

Length:            2BYTE,Identification from the device identifier code to check word, The

device identifier code: 2BYTE, 0x0000

Command:           2BYTE, Identifies the coding command functions

Parameters:        Command packets (can be empty)

Checksum:          1BYTE, Device identification to the parameter byte by byte XOR

Response:

Header + Length + Device identifier code + Command + Status + Parameters + Checksum

Header:            2BYTE,0xAABB

Length:            2BYTE,Identification from the device identifier code to check word, The first byte is valid,Second byteis reserved 0

Device identifier code: 2BYTE, 0x0000

Command:           2BYTE, Identifies the coding command functions

Status :           1BYTE,00 = Command OK, Nonzero = Command failure

Parameters:        Command packets (can be empty)

Checksum:          1BYTE, Device identification to the parameter byte by byte XOR


Animal ID Format:

                   14 bytes animal ID

                   Animal ID format: National ID(5 Bytes) +

                                     Country ID(2 Bytes) +

                                     DataFlag(1 Byte) +

                                     AnimalFlag(1 Byte) +

                                     CRC(2 Bytes) +

                                     Trailer(3 Bytes)
