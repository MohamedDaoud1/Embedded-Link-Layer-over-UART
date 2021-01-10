#define Message_Length 70
#define TimetoWait 50
String Msg2TivaSERIAL;
unsigned char Msg2Tiva[Message_Length+2];
unsigned char Mess[Message_Length+2];
unsigned char c =0;
unsigned char i = 0;
unsigned char Temp=0;
unsigned int time;
unsigned char obtainedLength =0 ;
unsigned char Mess_bitArray[(Message_Length-22)*8];
unsigned char DEST_Add[6] = {0x10,0x1A,0xB6,0x83,0x34,0xAA}; 		// Destination Address: 00:1A:B6:03:34:AA in HEX
unsigned char SRC_Add[6] = {0xCC,0x1A,0xB6,0x83,0x34,0xAA};		// Source Address: CC:1A:B6:03:34:AA in HEX

void Initialize_Message()
{
  unsigned char i = 0;
  // Add Preamble
  // First 7 Bytes =>> 0b10101010 = 0xAA
  // Eighth Byte =>> 0b10101011 = 0xAB
  for(i = 0; i < 7 ; i++)
    Msg2Tiva[i] = 0xAA;
  Msg2Tiva[7] = 0xAB;	
  // Add Destination Address
  for(i = 8; i < 14 ; i++)
    Msg2Tiva[i] = DEST_Add[i-8];	
  // Add Source Address
  for(i = 14 ; i < 20 ; i++)
    Msg2Tiva[i] = SRC_Add[i-14];
	
  Msg2Tiva[20] = 0xBB; // Type byte
  Msg2Tiva[21] = 0xBB; // Type byte
}


char MakeCRC(unsigned char *BitArr)
{
	static char Res[8];                                 // CRC Result
	char CRC[7];
	char crcVar = 0;
	int  i;
	char DoInvert;
	for (i = 0; i<7; ++i)  CRC[i] = 0;                    // Init before calculation
	for (i = 0; i<((Msg2TivaSERIAL.length())*8); ++i)
	{
		DoInvert = (BitArr[i]) ^ CRC[6];         // XOR required?
		CRC[6] = CRC[5];
		CRC[5] = CRC[4];
		CRC[4] = CRC[3];
		CRC[3] = CRC[2] ^ DoInvert;
		CRC[2] = CRC[1];
		CRC[1] = CRC[0];
		CRC[0] = DoInvert;
	}
	for (i = 0; i < 7; ++i)
		crcVar = crcVar |( CRC[i] << i);
	return(crcVar);
}

void CRC()
{
	char l=0;
	for (char k = 22; k < (Msg2TivaSERIAL.length()+22); k++)
		for (char c = 0; c < 8; c++)
		{
			Mess_bitArray[l] = ((Msg2Tiva[k] >> (7-c)) & 1);
			l++;
		}			
	Msg2Tiva[Msg2TivaSERIAL.length()+23] = MakeCRC(Mess_bitArray);                                    // Calculate CRC
}


char MakeCRC_CHECK(unsigned char *BitArr)
{
        char Bit_Length= (((obtainedLength-1)*8) +7);
	static char Res[8];                                 // CRC Result
	char CRC[7];
	char crcVar = 0;
	int  i;
	char DoInvert;
	for (i = 0; i<7; ++i)  CRC[i] = 0;                    // Init before calculation
	for (i = 0; i<Bit_Length; ++i)
	{
		DoInvert = (BitArr[i]) ^ CRC[6];         // XOR required?
		CRC[6] = CRC[5];
		CRC[5] = CRC[4];
		CRC[4] = CRC[3];
		CRC[3] = CRC[2] ^ DoInvert;
		CRC[2] = CRC[1];
		CRC[1] = CRC[0];
		CRC[0] = DoInvert;
	}
	for (i = 0; i < 7; ++i)
		crcVar = crcVar |( CRC[i] << i);
	return(crcVar);
}
bool checkCRC()
{
  char l=0;
  char res;
  for (char k = 22; k < (obtainedLength); k++)
  { 
    for (char c = 0; c < 8; c++)
    {
      Mess_bitArray[l] = ((Mess[k] >> (7-c)) & 1);
      l++;
    }
  }
  res = MakeCRC_CHECK(Mess_bitArray);
  return res;	
}
void setup() 
{
  Initialize_Message();
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop() 
{
  if(Serial.available())
  {
    Msg2TivaSERIAL=Serial.readString();
    //Serial.println(Msg2TivaSERIAL.length());
    for(char h =22 ; h < (Msg2TivaSERIAL.length()+23) ; h++)
      Msg2Tiva[h] = Msg2TivaSERIAL[h-22];
    CRC();
    for(char h =0 ; h < (Msg2TivaSERIAL.length()+24) ; h++)
      Serial1.write(Msg2Tiva[h]);
  }
  if(Serial1.available())
  {
    time = millis();
    while(1)
    {
    if(Serial1.available())
    {    
    //x = x + Serial1.read(); 
    Mess[c] = Serial1.read(); 
    c++;
    }    
    if((millis()-time)>TimetoWait) 
    {
      obtainedLength = c;
      break;
    }
  }
  }
  if(c != 0)
  {
    c =0;  
    // Add Preamble
    // First 7 Bytes =>> 0b10101010 = 0xAA
    // Eighth Byte =>> 0b10101011 = 0xAB
    Serial.println("======================================================");
    Serial.println("Frame Received !");
    Serial.println("Preamble:");
    for(i = 0; i < 8 ; i++)
    {
      Serial.print(Mess[i],HEX);
      Serial.print(' ');
    }
    // Add Destination Address
    Serial.println("");
    Serial.println("DEST MAC:");
    for(i = 8; i < 14 ; i++)
    {
      Serial.print(Mess[i],HEX);
      if(i!=13)
        Serial.print(" : ");
    }
    // Add Source Address
    Serial.println("");
    Serial.println("SRC MAC:");
    for(i = 14 ; i < 20 ; i++)
    {
      Serial.print(Mess[i],HEX);
      if(i!=19)
        Serial.print(" : ");
    }
    // Print type bits
    Serial.println("");
    Serial.println("TYPE:");
    for(i = 20 ; i < 22 ; i++)
    {
      Serial.print(Mess[i],HEX);      
      if(i!=21)
        Serial.print(" : ");
    }
    Serial.println("");
    Serial.println("Message:");
    for(i = 22 ; i < (obtainedLength-1) ; i++)
    {
      Serial.write(Mess[i]);
    }
    Serial.println("");
    Serial.println("CRC:");
    Serial.println(Mess[obtainedLength-1],HEX);
    Mess[obtainedLength-1] = Mess[obtainedLength-1] << 1;
    for(int y = 0 ; y < Message_Length ; y++ )
      Mess[y] = 0;
    if(!checkCRC()) Serial.println("CHECKED");
    else Serial.println("ERROR");
  }
}
