#if TH_OutUnit == 3 || WIND_OutUnit == 3 || RAIN_OutUnit == 3
// --------------------------------------------------------------------------------------
//   Receive data from Wireless Sensors
// --------------------------------------------------------------------------------------
void RX_WirelessSensors()
{
	Wireless_Data = 0;
	switch (RCode())
	{
	case 0:
		return;

#if RAIN_OutUnit == 3
	case 1:
	{
		unsigned int AuriolTR_tips = 0;
		static unsigned int last_AuriolTR_tips;
		static unsigned long last_millis_0;

		for (byte i = 0; i < 16; i++)
			if (bitRead(Wireless_Data, 23 - i) == 1) AuriolTR_tips += 1 << i;

		unsigned int elapsed = (millis() - last_millis_0) / 1000;

		if (AuriolTR_tips != last_AuriolTR_tips)
		{
			if (elapsed >= 3600) Auriol_rainftiphour = 1;
			else Auriol_rainftiphour = (unsigned int)((((AuriolTR_tips - last_AuriolTR_tips) * 3600) / elapsed));

			if (last_AuriolTR_tips == 0) Auriol_rainftiphour = 0;   // Prevents high Rainfail rates on restarts
			last_millis_0 = millis();
		}

		if (((AuriolTR_tips == last_AuriolTR_tips) || (last_AuriolTR_tips == 0)) && (elapsed >= 60)) Auriol_rainftiphour = (unsigned int)(Auriol_rainftiphour / 3.0); // Progressive decresse of rainfall when rain stops
		last_AuriolTR_tips = AuriolTR_tips;

		Unit[3].Rainftipshour = Auriol_rainftiphour;
		Unit[3].TotalRain_tips = AuriolTR_tips;
		bitWrite(MainSensorsRX, 2, 1);

		//Serial.print(millis());Serial.print(" A TR_tips :"); Serial.println(Unit[3].TotalRain_tips);

		break;
	}
#endif


#if TH_OutUnit == 3
	case 2:
	{
		int AuriolOutTemp = 0;
		int AuriolOutHum = 0;
		byte Hum1 = 0;
		byte Hum2 = 0;

		//Serial.print("Wireless_DeviceID : "); Serial.println(Wireless_DeviceID, HEX);

		for (byte i = 0; i < 12; i++)
			if (bitRead(Wireless_Data, 27 - i) == 1) AuriolOutTemp += 1 << i;

		// Calculate negative temperature
		if ((AuriolOutTemp & 0x800) == 0x800) AuriolOutTemp = AuriolOutTemp | 0xF000;

		for (byte i = 0; i < 4; i++)
		{
			if (bitRead(Wireless_Data, 11 - i) == 1) Hum1 += 1 << i; // tenths
			if (bitRead(Wireless_Data, 15 - i) == 1) Hum2 += 1 << i; // ones
		}

		AuriolOutHum = ((Hum1 * 10) + Hum2) * 10;

		Unit[3].TemperaturaExt[0] = AuriolOutTemp;
		Unit[3].HumidadeExt[0] = AuriolOutHum;
		loopData.outsideTemperature = (AuriolOutTemp * 1.8) + 320;    // Convert ºC to ºF
		loopData.outsideHumidity = (AuriolOutHum + 5) * 0.1;       // Round the reading

		if (Temp_Display_Unit == 0)
			Output_T_Out = AuriolOutTemp;
		else
			Output_T_Out = loopData.outsideTemperature;
		Output_H_Out = AuriolOutHum;

		float Dew_Point = (Output_T_Out / 10.0) - ((100 - (Output_H_Out / 10.0)) / 5);  // Simpler calculation good for RH above 50%
		Dew_Point_F = (int)((Dew_Point * 1.8) + 32.5);  // Convert to Fahrenheit (Rounded Integer value)

		bitWrite(MainSensorsRX, 0, 1);

		break;
	}
#endif


#if WIND_OutUnit == 3
	case 3:
	{
		uint16_t AuriolWindAvg = 0;

		for (byte i = 0; i < 8; i++)
			if (bitRead(Wireless_Data, 15 - i) == 1) AuriolWindAvg += 1 << i;
		Unit[3].Wind_average = AuriolWindAvg * 200;

		if (bitRead(Wireless_Data, 31) == 0)            // Batterie Status Bit
		{
			loopData.transmitterBatteryStatus = 0;        // Good baterie
			loopData.consoleBatteryVoltage = 522;
		}
		else
		{
			loopData.transmitterBatteryStatus = 1;        // Weak Batterie < 2.6V
			loopData.consoleBatteryVoltage = 0;
		}
		break;
	}

	case 4:
	{
		uint16_t AuriolWindDir = 0;
		uint16_t AuriolWindGust = 0;
		static uint16_t lastgust;

		for (byte i = 0; i < 9; i++)
			if (bitRead(Wireless_Data, 21 - i) == 1) AuriolWindDir += 1 << i;
		for (byte i = 0; i < 8; i++)
			if (bitRead(Wireless_Data, 12 - i) == 1) AuriolWindGust += 1 << i;

		//Serial.print("AuriolWindGust0 : "); Serial.println((AuriolWindGust / 5.0) * 3.6, 1);

		// Wind gust spike removal
		if (AuriolWindGust >= 250) AuriolWindGust = lastgust;  // Some Auriol wind sensor, when have near strikes tends to report false 51m/s wind gusts
		lastgust = AuriolWindGust;

		Unit[3].Wind_dir = AuriolWindDir * 10;
		Unit[3].Wind_gust = AuriolWindGust * 200;              // gust in m/s

															   /*
															   //else if ((code.charAt(4)=='0')&&(code.charAt(9)=='1')&&(code.charAt(10)=='1')&&(code.charAt(12)=='1')&&(code.charAt(13)=='1')&&(code.charAt(14)=='1'))
															   else if (Wireless_DeviceID != BiosSensor_ID && bitRead(Wireless_Data, 30) == 1 && bitRead(Wireless_Data, 29) == 1 && bitRead(Wireless_Data, 27) == 1 && bitRead(Wireless_Data, 26) == 1 && bitRead(Wireless_Data, 25) == 1)
															   {
															   unsigned int AuriolWindDir  = 0;
															   unsigned int AuriolWindGust = 0;
															   static unsigned int lastgust;

															   for (byte i = 0; i < 9 ; i++)
															   if (bitRead(Wireless_Data, 24 - i) == 1) AuriolWindDir += 1 << i;
															   for (byte i = 0; i < 8 ; i++)
															   if (bitRead(Wireless_Data, 15 - i) == 1) AuriolWindGust += 1 << i;

															   //Serial.print("AuriolWindGust1 : "); Serial.println((AuriolWindGust / 5.0) * 3.6, 1);

															   // Wind gust spike removal
															   if (AuriolWindGust >= 250) AuriolWindGust = lastgust;  // Some Auriol wind sensor, when have near strikes tends to report false 51m/s wind gusts
															   lastgust = AuriolWindGust;

															   Unit[3].Wind_dir  = AuriolWindDir * 10;
															   Unit[3].Wind_gust = AuriolWindGust * 200;              // gust in m/s
															   }
															   */
															   // --------------------------------------------------------------------------------------
															   //   Decode the received data and return type of data (Temp, Wind, Rain, etc...
															   // --------------------------------------------------------------------------------------
	}
	//----------------------------------------------------------------------------
	}
}

byte RCode()
{
	//String code = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	String code = "xxxxxxxx";

	byte lastbit_RX = 7;
	long startMicros = micros(), endMicros;

	//if (digitalRead(RX_PIN)) return;
	//while(!digitalRead(RX_PIN))
	if (bitRead(PINB, 0)) return 0;
	while (!bitRead(PINB, 0))
	{
		if ((micros() - startMicros) > smax)
			return 0;
	}
	if ((micros() - startMicros) < smin)
		return 0;
	startMicros = micros();
	//while(digitalRead(RX_PIN))
	while (bitRead(PINB, 0))
	{
		if ((micros() - startMicros) > semax)
			return 0;
	}
	if ((micros() - startMicros) < semin)
		return 0;
	for (byte i = 0; i < nobits; i++)
	{
		startMicros = micros();
		//while(!digitalRead(RX_PIN))
		while (!bitRead(PINB, 0))
		{
			if ((micros() - startMicros) < 1);
		}
		endMicros = micros();
		if (((endMicros - startMicros) > lmin) && ((endMicros - startMicros) < lmax))
		{
			if (i > 7)
			{
				if (lastbit_RX + 1 != i) return 0;
				bitWrite(Wireless_Data, 39 - i, 0);
				lastbit_RX = i;
			}
			else
			{
				code.setCharAt(i, '0');
				bitWrite(Wireless_DeviceID, 7 - i, 0);
			}
		}
		else if (((endMicros - startMicros) > hmin) && ((endMicros - startMicros) < hmax))
		{
			if (i > 7)
			{
				if (lastbit_RX + 1 != i) return 0;
				bitWrite(Wireless_Data, 39 - i, 1);
				lastbit_RX = i;
			}
			else
			{
				code.setCharAt(i, '1');
				bitWrite(Wireless_DeviceID, 7 - i, 1);
			}
		}
		startMicros = micros();
		//while(digitalRead(RX_PIN))
		while (bitRead(PINB, 0))
		{
			if ((micros() - startMicros) > semax)
				return 0;
		}
		if ((micros() - startMicros) < semin)
			return 0;
	}


#if RAIN_OutUnit == 3
	if (bitRead(Wireless_Data, 30) == 1 && bitRead(Wireless_Data, 29) == 1 && bitRead(Wireless_Data, 28) == 0 && bitRead(Wireless_Data, 27) == 1 && bitRead(Wireless_Data, 26) == 1 && bitRead(Wireless_Data, 25) == 0 && bitRead(Wireless_Data, 24) == 0)
		return 1;  // Rain packet
#endif
#if TH_OutUnit == 3
	bool AuriolTempPacket = true;
	for (byte j = 0; j < 3; j++)   // Check Auriol Temp Packet validity
		if (code.charAt(j) == 'x')
		{
			AuriolTempPacket = false;  // Return false if invalid Packet
			break;
		}
	if ((AuriolTempPacket == true) && ((bitRead(Wireless_Data, 30) == 0 && bitRead(Wireless_Data, 29) == 0) || (bitRead(Wireless_Data, 30) == 1 && bitRead(Wireless_Data, 29) == 0) || (bitRead(Wireless_Data, 30) == 0 && bitRead(Wireless_Data, 29) == 1)))
		return 2;  // Temp / Hum packet
#endif
#if WIND_OutUnit == 3
	if (bitRead(Wireless_Data, 30) == 1 && bitRead(Wireless_Data, 29) == 1 && bitRead(Wireless_Data, 27) == 1 && bitRead(Wireless_Data, 26) == 0 && bitRead(Wireless_Data, 25) == 0)
		return 3;  //  Wind Average / Bat state packet
	else if (bitRead(Wireless_Data, 27) == 1 && bitRead(Wireless_Data, 26) == 1 && bitRead(Wireless_Data, 24) == 1 && bitRead(Wireless_Data, 23) == 1 && bitRead(Wireless_Data, 22) == 1)
		return 4;  // Wind Dir / Gust packet
#endif
	return 0;
}
#endif
#endif

