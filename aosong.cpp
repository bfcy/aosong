

#include<iostream>
#include<unistd.h>
#include<wiringPi.h>
#include<iomanip>

using namespace std;

#define USING_DHT11      true //false   // The DHT11 uses only 8 bits
#define DHT_GPIO         22      // Using GPIO 22 for this example
#define LH_THRESHOLD     26      // Low=~14, High=~38 - pick avg.

void printResults(int* results)
{
	
	cout << "Checksum good" << endl;
    cout << "Temperature = " << (float)results[1]/10 << "°C" << endl;
    cout << "Humidity = " << (float)results[0]/10 << "%" << endl;
	
}

unsigned char checkSum(unsigned char* data)
{
	unsigned char chk = 0;
	
	for(int i=0; i<4; i++)
	{ 
	   chk+= data[i]; 
	}
	
	return chk;
}

void convertData(unsigned char* data, int* results)
{
   // result[0] for humidity
   // result[1] for temp
	
	if (USING_DHT11)
	{
		results[0] = data[0] * 10;   // one byte - no fractional part
		results[1] = data[2] * 10;   // multiplying to keep code concise
	}
	else 
	{                              // for DHT22 (AM2302/AM2301)
		results[0] = (data[0]<<8 | data[1]);  // shift MSBs 8 bits left and OR LSBs
		results[1] = (data[2]<<8 | data[3]);   // same again for temperature
	}

}


void readData(unsigned char* data)
{
	int width = 0;
	
  for(int byte=0; byte<5; byte++) 	// for each data byte
   {       
      for(int bit=0; bit<8; bit++) 	// read 8 bits
      {    
         do 						// for each bit of data
         { 
			 delayMicroseconds(1); 
		 } 
		 while(digitalRead(DHT_GPIO)==LOW);
		 
         width = 0;           // measure width of each high
         do 
         {
            width++;
            delayMicroseconds(1);
            if(width>1000) break; // missed a pulse -- data invalid!
         } 
         while(digitalRead(DHT_GPIO)==HIGH);    // time it!
         // shift in the data, msb first if width > the threshold
         data[byte] = data[byte] | ((width > LH_THRESHOLD) << (7-bit));
      }
   }

}


void setup(void)
{
	cout << "Starting the one-wire sensor program" << endl;
	wiringPiSetupGpio();
	piHiPri(99);
}

void init(void)
{
	
	pinMode(DHT_GPIO, OUTPUT);                 // gpio starts as output
	digitalWrite(DHT_GPIO, LOW);               // pull the line low
	usleep(18000);                             // wait for 18ms
	digitalWrite(DHT_GPIO, HIGH);              // set the line high
	pinMode(DHT_GPIO, INPUT);                  // now gpio is an input
	
}




int main(int argc, char *argv[])
{

	int results[2] = {0, 0};
	bool finished = false;
	unsigned char chk = 0;

	setup();

	while(!finished)
	{
		unsigned char data[5] = {0,0,0,0,0}; 
		
		init();

		// ignore the first and second high after going low
		usleep(160);
   
		readData(&data[0]);
		convertData(&data[0], &results[0]);
		chk = checkSum(&data[0]);
   
		if(chk==data[4])
		{
			finished = true;
			printResults(&results[0]);
		}
		else
		{
			cout << "Checksum bad - data error - retrying!" << endl;
			usleep(2000000);   // must delay for 1-2 seconds between readings
			chk = 0;   // the checksum will overflow automatically
		}		
	}
 
	return 0;
}
