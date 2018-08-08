#include <VirtualWire.h>
#include <DataFlash.h>
#include <SerialCommand.h>
#include <ClickButton.h>
#include <EEPROM.h>
#include <LiquidCrystal-I2C/LiquidCrystal_I2C.h>
#include <BMP085.h>
#include <Adafruit_SHT31.h>
#include <SHT2x.h>
#include <Adafruit_HTU21DF.h>
#include <DHTxx.h>
#include <RTClib.h>
#include <Wire.h>
#include <SPI.h>
#include "Config_RX.h"
#include "data_structs.h"

const char Software_Ver[] = "v2.1 b002";

/***************************************************************

Hardware PINS

***************************************************************/

#define Led1_PIN 2
#define GPO_PIN  3

/***************************************************************

Usefull Macros

***************************************************************/

#define SetPortHIGH(port, pin) ((port) |= (1 << (pin)))
#define setPortLOW(port, pin) ((port) &= ~(1 << (pin)))

/***************************************************************

Initialize LCD Display

***************************************************************/

LiquidCrystal_I2C lcd(LCD_I2C_Address, 20, 4);
byte heart[8] = { 0x0, 0xa, 0x1f, 0x1f, 0xe, 0x4, 0x0 };     // custom character:

															 /***************************************************************

															 Importanti per ricevere i dati dai sensori - NON MODIFICARE

															 ***************************************************************/

#if TH_OutUnit==3 || WIND_OutUnit==3 || RAIN_OutUnit==3
#define	nobits	36
#define smin	7500
#define smax	9900
#define semin	250
#define semax	750
#define	lmin	1700
#define lmax	2300
#define hmin	3700
#define hmax	4300
#endif

															 /***************************************************************

															 Variabili globali generali

															 ***************************************************************/

ClickButton button1(5, LOW, CLICKBTN_PULLUP); //Button pin = 5

const char vaneDirectionsText[16][4] = { "N","NNE","NE","ENE","E","ESE","SE","SSE","S","SSW","SW","WSW","W","WNW","NW","NNW" };
char RX_DataPacket[32];

const float ms_To_WindDisplayUnit[4] = { 3.6, 2.23694, 1.0, 1.94384 };
float SeaLevel_ATM;
float averagew_ms, gust_ms;
float Output_WSpeed, Output_WGust, Output_RToday;

unsigned long last_UpdateDisplay;
unsigned long BackLight_Timer;
unsigned long Wireless_Data;

const int MaxPageNum = 511;	//512 pagine da 0 a 511
int ButtonFunction = 0;
int TemperatureInt;
int Output_T_Int, Output_T_Out;
int Soil_Data[4][2];
int Leaf_Data[4][2];

unsigned int Auriol_rainftiphour;
unsigned int Output_H_Int, Output_H_Out;
unsigned int Output_SRad, Output_UV;
unsigned int archHighSolRad;
unsigned int archHighRainRate;
unsigned int windd;
unsigned int YearNow_Eeprom;
unsigned int PageToWrite = 0;
unsigned int RAW_pressure_inHg;
unsigned int Dew_Point_F;
unsigned int COLLECTOR_TYPE[4] = { 300, 300, 300, 250 };
unsigned int YearNow;
unsigned int RainToday_tips;
unsigned int DaysWithoutRain;
unsigned int thisMonthRain_tips;
unsigned int thisYearRain_tips;
unsigned int RainDayBegin_tips;
unsigned int RainMonthBegin_tips;
unsigned int RainYearBegin_tips;

const byte ExtraSensor[5][2] = ExtraSensors_Source;
const byte SoilTemp_source[4][2] = SoilTempSensors_Source;
const byte SoilMoist_Source[4][2] = SoilMoistSensors_Source;
const byte LeafTemp_Source[4][2] = LeafTempSensors_Source;
const byte LeafWet_Source[4][2] = LeafWetSensors_Source;

const byte Display_Units = (Date_Time_Format << 6) | Temp_Display_Unit << 5 | ATM_Display_Unit << 4 | Rain_Display_Unit << 3 | Wind_Display_Unit;

byte MonthNow, DayNow, HourNow, MinuteNow, SecondNow;
byte FirstRunCheck;
byte FirstRunInit;
byte DayNow_Eeprom;
byte DayLastRain_tip;
byte MonthNow_Eeprom;
byte activeWindow = 99;
byte last_min = 99;
byte last_min1 = 99;
byte last_min3 = 99;
byte last_second0 = 99;
byte L_info = 1;
byte ExtraS_count = 0;
byte archHighUV = 0;
byte MainSensorsRX = 0;
byte GPO_state = 0;
byte Wireless_DeviceID = 0;
byte ArchNum;
byte RXverifyMask;

bool StartUp = true;
bool BackLight_State = true;
bool rolloverDone = false;

/***************************************************************

Arduino Internal EEPROM Data Storage Addresses

***************************************************************/

const byte RainGauge_Source_eeprom_adr = 100;
const byte TotalRain_tips_eeprom_adr = 110;
const byte RainDayBegin_tips_eeprom_adr = 120;
const byte RainMonthBegin_tips_eeprom_adr = 130;
const byte RainYearBegin_tips_eeprom_adr = 140;
const byte thisYearRain_tips_eeprom_adr = 150;
const byte DayLastRain_tips_eeprom_adr = 159;
const byte DayNow_eeprom_adr = 160;
const byte MonthNow_eeprom_adr = 170;
const byte YearNow_eeprom_adr = 180;
const byte DaysWithoutRain_eeprom_adr = 190;
const byte FirstRunCheck_eeprom_adr = 200;
const byte FirstRunInit_eeprom_adr = 201;
const byte SoftwareVersion_eeprom_adr = 202;
const byte SoftwareBuild_eeprom_adr = 203;

/***************************************************************

Crea Oggetti

***************************************************************/
#if InsideTH_Sensors == 0
DHTxx dht(9);				//DHT pin = 9
#endif
#if InsideTH_Sensor == 1
Adafruit_HTU21DF htu21d = Adafruit_HTU21DF();
#endif
#if InsideTH_Sensor == 3
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#endif
RTC_DS3231 RTC;
BMP085 bmp;
SerialCommand sCmd;
LoopPacket loopData;
ArchiveRec archive;
DataFlash dataflash;

void setup()
{
	Serial.begin(19200);

	// Receiver Setup

	DDRB = DDRB | B0;		// pinMode(RX_PIN, INPUT);
	vw_set_rx_pin(8);		// Receiver pin = 8
	vw_setup(1000);			//Bits per sec
	vw_rx_start();

	//-------------------------------------

	SetPortHIGH(PORTD, 3);

	SPI.begin();
	Wire.begin();

#if InsideTH_Sensor == 1
	htu21d.begin();
#endif

	bmp.begin();
	RTC.begin();

	lcd.begin();
	init_LCD();

#if RAIN_OutUnit == 3
	COLLECTOR_TYPE[2] = 250;
#endif
	if (FirstRunCheck != 0xAC) SystemInit1();
	SetPortHIGH(PORTD, 2);
	if (TestMode) RXverifyMask = B00000000;
	else RXverifyMask = B00000111;

}

void loop()
{
#if TH_OutUnit == 3 || WIND_OutUnit == 3 || RAIN_OutUnit == 3
	RX_WirelessSensors();
#endif

	if (vw_have_message()) RecieveDataRF();
	if ((MainSensorsRX & B00000111) == RXverifyMask && StartUp) {
		if (FirstRunInit != 0xAC) SystemInit2(YearNow, DayNow);
		StartUp = false;
		setPortLOW(PORTD, 2);
	}

#if Work_Mode == 1
	if (SecondNow % 10 == 0 && SecondNow != last_second0)
	{
		//easyweather_Output();
		last_second0 = SecondNow;
	}
#endif

	if (MinuteNow != last_min)
	{
		//DisplayClock_LCD();       // Update LCD Clock
		//read_InsideTH_Sensor();   // Read Inside Temp / Hum    IL PROBLEMA E' QUI DENTRO
		read_BMP();               // Read ATM

#if UseGPO == 1
		GPO_task();
#endif    
		last_min = MinuteNow;
	}

	if (BackLight_Timeout != 0 && (millis() - BackLight_Timer) > BackLight_Timeout * 60000)
	{
		BackLight_State = false;
		lcd.setBacklight(BackLight_State);
	}

	// ---- Update display every 5 seconds
	if (millis() - last_UpdateDisplay > 5000)
	{
		write_LCD();
		last_UpdateDisplay = millis();
	}
}
