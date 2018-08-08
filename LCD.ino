
#if DisplayType == 0

void write_LCD() {
	/*
	if (FirstRunInit != 0xAC) {
		lcd.setCursor(4, 2);
		lcd.print(F("Initializing"));
		lcd.setCursor(2, 3);
		lcd.print(F("Auto-Reboot soon!"));
		lcd.setCursor(17, 1);
		lcd.print(bitRead(MainSensorsRX, 0));
		lcd.print(bitRead(MainSensorsRX, 1));
		lcd.print(bitRead(MainSensorsRX, 2));
		return;
	}
	else if (StartUp == true) {
		lcd.setCursor(0, 2);
		lcd.print(F("Don't start weather"));
		lcd.setCursor(3, 3);
		lcd.print(F("software yet! "));
		lcd.print(bitRead(MainSensorsRX, 0));
		lcd.print(bitRead(MainSensorsRX, 1));
		lcd.print(bitRead(MainSensorsRX, 2));
		return;
	}
	lcd.setCursor(0, 3);
	for (byte i = 0; i < 20; i++) lcd.print(F(" "));
	*/
	switch (L_info) {
	case 1: {
//
//		lcd.setCursor(0, 1);
//		lcd.print(F("In :  "));
//		if (Output_T_Int >= 100) lcd.setCursor(5, 1);
//		else if (Output_T_Int < 0) lcd.setCursor(4, 1);
//		else lcd.setCursor(6, 1);
//		lcd.print(Output_T_Int / 10.0, 1);
//		lcd.write(B11011111);			//Simbolo dei gradi
//#if Temp_Display_Unit == 0
//		lcd.print(F("C "));
//#else
//		lcd.print(F("F "));
//#endif
//		lcd.setCursor(13, 1);
//		lcd.print(Output_H_Int / 10.0, 1);
//		lcd.print(F(" %H"));
//
//		lcd.setCursor(0, 2);
//		lcd.print(F("Out:   "));

		if (Output_T_Out >= 100) lcd.setCursor(5, 1);
		else if (Output_T_Out < 0) lcd.setCursor(4, 1);
		else lcd.setCursor(6, 1);
		lcd.print(Output_T_Out / 10.0, 1);
		lcd.write(B11011111);   // Degree Symbol
#if Temp_Display_Unit == 0
		lcd.print(F("C  "));
#else
		lcd.print(F("F  "));
#endif
		lcd.setCursor(13, 1);
		lcd.print(Output_H_Out / 10.0, 1);
		lcd.print(F(" %H"));

		lcd.setCursor(1, 2);
		lcd.print(F("ATM: "));
#if ATM_Display_Unit == 0
		lcd.print(SeaLevel_ATM, 1);
		lcd.print(F(" mB"));
#else
		lcd.print(SeaLevel_ATM * 0.0295, 2);
		lcd.print(F(" inHg"));
#endif
		L_info = 2;
		break;
	}
	case 2:
	{
		lcd.setCursor(1, 3);
		lcd.print(F("Wind: "));
		if (Output_WSpeed != 0)
		{
			lcd.print(Output_WSpeed, 1);
#if Wind_Display_Unit == 0
			lcd.print(F(" Kmh "));
#endif
#if Wind_Display_Unit == 1
			lcd.print(F(" mph "));
#endif
#if Wind_Display_Unit == 2
			lcd.print(F(" m/s "));
#endif
#if Wind_Display_Unit == 3
			lcd.print(F(" Kts "));
#endif
			lcd.print(vaneDirectionsText[(Unit[WIND_OutUnit].Wind_dir % 3600) / 225]);
		}
		else lcd.print(F("Calm"));
		L_info = 3;
		break;
	}
	case 3:
	{
		lcd.setCursor(1, 3);
		if (Output_RToday != 0)
		{
			lcd.print(F("Rain: "));
			lcd.print(Output_RToday, 2);
			lcd.print(F(" mm"));
		}
		else
		{
			lcd.print(F("No rain: "));
			lcd.print(DaysWithoutRain);
			lcd.print(F(" Days"));
		}
		if (SRUV_OutUnit != 9 && Solar_Sensor == 1 && Output_SRad != 0) L_info = 4;
		else if (SRUV_OutUnit != 9 && UV_Sensor == 1 && Output_UV != 0) L_info = 5;
		else L_info = 1;
		break;
	}

#if SRUV_OutUnit != 9
	case 4:
	{
		lcd.setCursor(0, 3);
		lcd.print(F("Solar Rad: "));
		lcd.print(Output_SRad);
		lcd.print(F(" Wm2"));
		if (SRUV_OutUnit != 9 && UV_Sensor == 1 && Output_UV != 0) L_info = 5;
		else L_info = 1;
		break;
	}
	case 5:
	{
		lcd.setCursor(2, 3);
		lcd.print(F("UV Index:  "));
		lcd.print(Output_UV / 1000.0, 1);
		L_info = 1;
		break;
	}
#endif
	}
}


// --------------------------------------------------------------------------------------
//   Write SysInfo to LCD
// --------------------------------------------------------------------------------------
#if TH_OutUnit != 3
void SysInfoDisplayLCD()
{
	lcd.clear();
	lcd.print(F("TX_0 Info"));
	lcd.setCursor(0, 1);
	lcd.print(F("Battery : "));
	lcd.print(Unit[0].BatVolt / 100.0, 2);
	lcd.print(F(" V"));
	lcd.setCursor(0, 2);
	lcd.print(F("Case T. : "));
#if Temp_Display_Unit == 0
	lcd.print(Unit[0].SysTemp / 100.0, 2);
	lcd.write(B11011111);   // Degree Symbol
	lcd.print(F("C"));
#else
	lcd.print((Unit[0].SysTemp * 0.018) + 32.0, 2);
	lcd.write(B11011111);   // Degree Symbol
	lcd.print(F("F"));
#endif
	lcd.setCursor(0, 3);
	lcd.print(F("RS Fan  : "));
	if (Unit[0].RSfan == 1) lcd.print(F("ON"));
	else lcd.print(F("OFF"));
	delay(4000);
	lcd.clear();
	L_info = 1;
	last_min = 99;
}
#endif


// --------------------------------------------------------------------------------------
//   Write date and time to LCD
// --------------------------------------------------------------------------------------
/*void DisplayClock_LCD()
{
	char lcd_horas[21];
#if Date_Time_Format == 0
	sprintf(lcd_horas, " %02d-%02d-%d   %02d:%02d ", DayNow, MonthNow, YearNow, HourNow, MinuteNow);
#endif
#if Date_Time_Format == 1
	sprintf(lcd_horas, " %d/%02d/%02d   %02d:%02d ", YearNow, MonthNow, DayNow, HourNow, MiniteNow);
#endif
	lcd.setCursor(0, 0);
	lcd.print(lcd_horas);
}*/


// --------------------------------------------------------------------------------------
//   Write init display to LCD
// --------------------------------------------------------------------------------------
void init_LCD()
{
	lcd.backlight();         // lcd backlight on

							 // create custum character
	lcd.createChar(0, heart);
	//lcd.createChar(1, clock);

	lcd.setCursor(2, 0);      // Start at character 3 on line 0
	lcd.print(F("WeatherDuino Pro2"));
	delay(1000);
	lcd.setCursor(5, 3);
	lcd.print(Software_Ver);
	delay(1500);
	lcd.setCursor(5, 1);
	lcd.print(F("by Werk_AG"));
	delay(1000);
	lcd.setCursor(4, 2);
	for (byte i = 0; i < 12; i++)
	{
		lcd.write(0);
		delay(127);
	}
	delay(1000);
	lcd.clear();
}

#endif

