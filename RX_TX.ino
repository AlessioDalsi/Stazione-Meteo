// --------------------------------------------------------------------------------------
//   Receive and Parse Data
// --------------------------------------------------------------------------------------
void RecieveDataRF()
{
	char RX_Station_ID[4];
	long RX_Data[6];                                      // define RX data elements array (must be long type)
	byte ReceivedStation_ID, inByte;

	memset(RX_DataPacket, 0, sizeof(RX_DataPacket));      // first clean DataPacket array
	uint8_t buflen = 32;
	uint8_t buf[32];                                  // incoming message can't have more than 32 bytes

	if (vw_get_message(buf, &buflen))
	{
		for (byte i = 0; i < buflen; i++)
		{
			inByte = (char)buf[i];
			RX_DataPacket[i] = inByte;
			if (i < 3) RX_Station_ID[i] = inByte;            // Store the first 3 bytes, for latter check Sender Station_ID
			if (i == 2)
			{
				RX_Station_ID[3] = 0x00; // appends a null char at the end
				ReceivedStation_ID = atoi(RX_Station_ID);
				if (Station_ID != ReceivedStation_ID) break;   // if received Station_ID != user configurable Station_ID, return and ignore data
			}
		}
	}

	if (Station_ID != ReceivedStation_ID) return;        // if received Station_ID != user configurable Station_ID, return and ignore data

														 //for (byte i=0; i<32; i++) Serial.print(RX_DataPacket[i]);
														 //Serial.println(" ");

														 // ------ Call function to parse received DataPacket to individual RX_Data elements ---------
	memset(RX_Data, 0, sizeof(RX_Data));      // first clean RX_Data array
	getValues(RX_DataPacket, RX_Data, 6);

	//byte ReceivedStation_ID = RX_Data[0];
	byte PacketID = RX_Data[1];
	if (PacketID > 63) return;

	byte UnitID = PacketID >> 4;           // PacketID High nibble     * PacketID = (UnitID << 4) | SensorID)
	byte SensorID = PacketID & B00001111;  // PacketID Low nible

#if TH_OutUnit != 3
	if (SensorID == 0)
	{
		//-------------- Process data from SensorID 0 --------------------------------------
		Unit[UnitID].TemperaturaExt[0] = RX_Data[2] / 10;                      // Temperature from outside sensor 0 in Tenths of Degree
		Unit[UnitID].HumidadeExt[0] = RX_Data[3] / 10;                      // Humidity from outside sensor 0 in Tenths of %
		Unit[UnitID].TemperaturaExt[1] = RX_Data[4] / 10;                      // Temperature from outside sensor 1 in Tenths of Degree
		Unit[UnitID].HumidadeExt[1] = RX_Data[5] / 10;                      // Humidity from outside sensor 1 in Tenths of %

																			//Serial.print("Unit ID "); Serial.print(UnitID); Serial.print(" S0 Temp: "); Serial.print(Unit[UnitID].TemperaturaExt[0]); Serial.print(" Hum: "); Serial.println(Unit[UnitID].HumidadeExt[0]);
																			//Serial.print("Unit ID "); Serial.print(UnitID); Serial.print(" S1 Temp: "); Serial.print(Unit[UnitID].TemperaturaExt[1]); Serial.print(" Hum: "); Serial.println(Unit[UnitID].HumidadeExt[1]);

		loopData.outsideTemperature = (Unit[TH_OutUnit].TemperaturaExt[MainOutTH_Sensor] * 1.8) + 320;    // Convert ºC to ºF
		loopData.outsideHumidity = (Unit[TH_OutUnit].HumidadeExt[MainOutTH_Sensor] + 5) * 0.1;         // Round the reading

		if (Temp_Display_Unit == 0)
			Output_T_Out = Unit[TH_OutUnit].TemperaturaExt[MainOutTH_Sensor];
		else
			Output_T_Out = loopData.outsideTemperature;
		Output_H_Out = Unit[TH_OutUnit].HumidadeExt[MainOutTH_Sensor];

#if RAIN_OutUnit != 3
		float Dew_Point = (Unit[TH_OutUnit].TemperaturaExt[MainOutTH_Sensor] / 10.0) - ((100 - (Output_H_Out / 10.0)) / 5);  // Simpler calculation good for RH above 50%
																															 //float Dew_Point = ((pow(Output_H_Out / 1000.0, 0.125) * (112 + (0.09 * Unit[TH_OutUnit].TemperaturaExt[MainOutTH_Sensor]))) + (0.01 * Unit[TH_OutUnit].TemperaturaExt[MainOutTH_Sensor]) - 112); // Dew Point in Cº
		Dew_Point_F = (int)((Dew_Point * 1.8) + 32.5);  // Convert to Fahrenheit (Rounded Integer value)
#endif

		if (UnitID == TH_OutUnit)
		{
			bitWrite(MainSensorsRX, 0, 1);

		}
		// ----------- Process Extra Sensors ID0 ----------------------------------------------------
		if (ExtraSensor[ExtraS_count][0] != 9)
		{
			loopData.extraTemperatures[ExtraS_count] = (Unit[ExtraSensor[ExtraS_count][0]].TemperaturaExt[ExtraSensor[ExtraS_count][1]] * 0.18) + 122.5;
			loopData.extraHumidities[ExtraS_count] = (Unit[ExtraSensor[ExtraS_count][0]].HumidadeExt[ExtraSensor[ExtraS_count][1]] + 5) * 0.1;

		}
		ExtraS_count += 1;
		if (ExtraS_count > 4) ExtraS_count = 0;
	} // -------- End processing Sensors ID0 ------------------------------------
#endif

#if WIND_OutUnit != 3

#if TH_OutUnit == 3
	if (SensorID == 2 && UnitID == WIND_OutUnit)
#else
	else if (SensorID == 2 && UnitID == WIND_OutUnit)
#endif
	{
		unsigned long gust;
		gust = RX_Data[3];
		if (gust <= 67000)                                    // - First step Spike Removal - Process if Gust <= 241 Km/h
		{
#if WindSpikeControl == 1
			static unsigned long last_gust;
			unsigned int spike_limit;
			if (last_gust < 11111) spike_limit = 8333;   // < 40 Km/h -> Spike Limit 30 Km/h
			else if (last_gust < 18055) spike_limit = 15277;  // < 65 Km/h -> Spike Limit 55 Km/h
			else spike_limit = 50000;                          // > 65 Km/h -> Spike Limit 180 Km/h
			if (gust > last_gust + spike_limit) gust = last_gust;
			last_gust = gust;
#endif

			Unit[UnitID].Wind_average = RX_Data[2];
			Unit[UnitID].Wind_gust = gust;
			Unit[UnitID].Wind_dir = RX_Data[4];

#if Work_Mode == 0
#if VP2_WindRes == 0
			loopData.tenMinAvgWindSpeed = Unit[WIND_OutUnit].Wind_average * 0.002237;                       // m/s * 1000 converted to mhp
			loopData.windSpeed = Unit[WIND_OutUnit].Wind_gust * 0.002237;                          // m/s * 1000 converted to mhp
			Output_WSpeed = loopData.tenMinAvgWindSpeed * mph_To_WindDisplayUnit[Wind_Display_Unit];
			Output_WGust = loopData.windSpeed * mph_To_WindDisplayUnit[Wind_Display_Unit];
#else
			loopData.tenMinAvgWindSpeed = Unit[UnitID].Wind_average / (100.0 * VP2_WindRes);                // value is m/s *10 or *20 depending on VP2_WindRes (Cumulus multiplier 0.224 or 0.448)
			loopData.windSpeed = Unit[UnitID].Wind_gust / (100.0 * VP2_WindRes);                   // value is m/s *10 or *20 depending on VP2_WindRes (Cumulus multiplier 0.224 or 0.448)
			Output_WSpeed = (loopData.tenMinAvgWindSpeed / 10.0) * VP2_WindRes * ms_To_WindDisplayUnit[Wind_Display_Unit];
			Output_WGust = (loopData.windSpeed / 10.0) * VP2_WindRes * ms_To_WindDisplayUnit[Wind_Display_Unit];
#endif
			loopData.windDirection = Unit[WIND_OutUnit].Wind_dir / 10;
#else
			Output_WSpeed = (Unit[WIND_OutUnit].Wind_average / 1000.0) * ms_To_WindDisplayUnit[Wind_Display_Unit];     // Wind Speed in m/s * 1000 converted to Km/h
			Output_WGust = (Unit[WIND_OutUnit].Wind_gust / 1000.0) * ms_To_WindDisplayUnit[Wind_Display_Unit];        // Wind Gust in m/s * 1000 converted to Km/h
#endif

			bitWrite(MainSensorsRX, 1, 1);

		}
	}
#endif


#if RAIN_OutUnit != 3

#if WIND_OutUnit == 3
	if (SensorID == 3)
#else
	else if (SensorID == 3)
#endif
	{
		COLLECTOR_TYPE[UnitID] = RX_Data[2];             // Collector tip value * 1000
		Unit[UnitID].Rainftipshour = RX_Data[3];
		Unit[UnitID].TotalRain_tips = RX_Data[4];
		//Serial.print("TotalRain_tips :"); Serial.println(Unit[UnitID].TotalRain_tips);
		bitWrite(MainSensorsRX, 2, 1);


	}
#endif


#if (SRUV_OutUnit !=9)

#if (WIND_OutUnit == 3)
	if (SensorID == 4 && UnitID == SRUV_OutUnit)
#else
	else if (SensorID == 4 && UnitID == SRUV_OutUnit)
#endif
	{
		// RX_Data[2] ---- Solar Data from TX Unit
		// RX_Data[3] ---- UV Data from TX Unit
		// RX_Data[4] ---- Not Used

		// ---- Solar Radiation
#if (Solar_Sensor == 0)
		Unit[UnitID].SolarRad = 0;
#endif  
#if (Solar_Sensor == 1)
		// --- WeatherDuino Solar Cell Pyranometer (Solar / UV interface R2= 2700 Ohm)
		if (RX_Data[2] < 4) RX_Data[2] = 0;
		Unit[UnitID].SolarRad = map(RX_Data[2], 0, 1023, 0, 1400);               // Formula outputs an integer, between 0 and 1400 W/m2
#endif
#if (Solar_Sensor == 2)
																				 // Apogee SP-110: Self-Powered Pyranometer (Solar / UV interface R2= 4700 Ohm)
		Unit[UnitID].SolarRad = map(RX_Data[2], 0, 1023, 0, 1600);
#endif 

		// ---- UV Index
		float IndiceUV = 0;

#if (UV_Sensor == 1)
		// --- UVM-30A
		IndiceUV = ((RX_Data[3] * 12.0 * (100.0 / UV_Transmission)) / 1024.0);
#endif  
#if (UV_Sensor == 2)
		// Reserved
#endif 
#if (UV_Sensor == 3)
		// --- Reyax UV-01
		if (RX_Data[3] <= 35) RX_Data[3] = 0;
		IndiceUV = ((RX_Data[3] * 12.0) / 1024.0);
#endif    

		Unit[UnitID].UV = (unsigned int)(IndiceUV * 1000.0);

		loopData.solarRadiation = Unit[SRUV_OutUnit].SolarRad;
		loopData.uV = Unit[SRUV_OutUnit].UV / 100;

		if (loopData.solarRadiation > archHighSolRad) archHighSolRad = loopData.solarRadiation;
		if (loopData.uV > archHighUV) archHighUV = loopData.uV;  // Cumulus MX gets the Hightest UV value over the archive period from data ogger

		if (Solar_Sensor != 0) Output_SRad = Unit[SRUV_OutUnit].SolarRad;
		else Output_SRad = 0xFFFF;
		if (UV_Sensor != 0) Output_UV = Unit[SRUV_OutUnit].UV;
		else Output_UV = 0xFFFF;
		bitWrite(MainSensorsRX, 3, 1);

#if Relay_Data == 1
		sendData(Relay_ID, 134, (long)(Output_SRad), (long)(Output_UV), 0, 0);
#endif
	}
#endif

#if WIND_OutUnit != 3  // Auriol case
	else if (SensorID == 5)
	{
		Unit[UnitID].BatVolt = (int)(((RX_Data[2] * 20.23) / 1024) * 100.0);
		Unit[UnitID].SysTemp = (int)((((RX_Data[3] * 1.1) / 1024) - 0.5) * 10000.0);     // For TMP36
																						 //Unit[UnitID].SysTemp    = ((RX_Data[3] * 1.1) / 1024) * 10000;                    // For LM35 (do not read negative temperatures)
		Unit[UnitID].RSfan = RX_Data[4];

		if (Unit[0].BatVolt > 12)
			loopData.transmitterBatteryStatus = 0;
		else
			loopData.transmitterBatteryStatus = 1;

		loopData.consoleBatteryVoltage = Unit[0].BatVolt * 1.709;
		if (UnitID < 2)
		{
			loopData.extraTemperatures[5 + UnitID] = (Unit[UnitID].SysTemp * 0.018) + 122.5;
			loopData.extraHumidities[5 + UnitID] = 0;
		}

	}
#endif

#if SoilLeaf_Interface == 1
	if (SensorID == 7)
	{
		static bool flag1 = true;
		for (byte i = 0; i < 4; i++)
		{
			if (SoilTemp_Source[i][0] == UnitID)
			{
				Soil_Data[i][0] = RX_Data[SoilTemp_Source[i][1] + 2] / 10;
				loopData.soilTemperatures[i] = (Soil_Data[i][0] * 0.18) + 122.5;
			}
			if (LeafTemp_Source[i][0] == UnitID)
			{
				Leaf_Data[i][0] = RX_Data[LeafTemp_Source[i][1] + 2] / 10;
				loopData.leafTemperatures[i] = (Leaf_Data[i][0] * 0.18) + 122.5;
			}
		}

#if Relay_Data == 1
		if (flag1) sendData(Relay_ID, 150, Soil_Data[0][0], Soil_Data[1][0], Soil_Data[2][0], Soil_Data[3][0]);
		else sendData(Relay_ID, 151, Leaf_Data[0][0], Leaf_Data[1][0], Leaf_Data[2][0], Leaf_Data[3][0]);
		flag1 = !flag1;
#endif
	}
	else if (SensorID == 9)
	{
		static bool flag2 = true;
		unsigned int miliVolts;
		for (byte i = 0; i < 4; i++)
		{
			if (SoilMoist_Source[i][0] == UnitID)
			{
#if VWC_CalcMethod == 1
				miliVolts = RX_Data[SoilMoist_Source[i][1] + 2];
				if (miliVolts < 50)   Soil_Data[i][1] = 0;
				else if (miliVolts < 1100) Soil_Data[i][1] = (int)((miliVolts / 1000.0) * 10.0 - 1);
				else if (miliVolts < 1300) Soil_Data[i][1] = (int)((miliVolts / 1000.0) * 25.0 - 17.5);
				else if (miliVolts < 1820) Soil_Data[i][1] = (int)((miliVolts / 1000.0) * 48.08 - 47.5);
				else if (miliVolts < 2200) Soil_Data[i][1] = (int)((miliVolts / 1000.0) * 26.32 - 7.89);
				else Soil_Data[i][1] = (int)((miliVolts / 1000.0) * 57.4 - 73.7); // Determined experimentally
#endif
#if VWC_CalcMethod == 2
				Soil_Data[i][1] = map(RX_Data[SoilMoist_Source[i][1] + 2], AnalogSensor_MinVoltOut[SoilMoist_Source[i][1]], AnalogSensor_MaxVoltOut[SoilMoist_Source[i][1]], 0, 100);
#endif
				if (Soil_Data[i][1] >= 0 && Soil_Data[i][1] <= 100)
					loopData.soilMoistures[i] = Soil_Data[i][1];  // Outputs data in VWC (Volumetric Water Content)
			}

			if (LeafWet_Source[i][0] == UnitID)
			{
				Leaf_Data[i][1] = (int)(map(RX_Data[LeafWet_Source[i][1] + 2], AnalogSensor_MinVoltOut[LeafWet_Source[i][1]], AnalogSensor_MaxVoltOut[LeafWet_Source[i][1]], 0, 15));
				if (Leaf_Data[i][1] >= 0 && Leaf_Data[i][1] <= 15)
					loopData.leafWetnesses[i] = Leaf_Data[i][1];
			}
		}

#if Relay_Data == 1
		if (flag2) sendData(Relay_ID, 160, loopData.soilMoistures[0], loopData.soilMoistures[1], loopData.soilMoistures[2], loopData.soilMoistures[3]);
		else sendData(Relay_ID, 161, loopData.leafWetnesses[0], loopData.leafWetnesses[1], loopData.leafWetnesses[2], loopData.leafWetnesses[3]);
		flag2 = !flag2;
#endif
	}
#endif
}      // End of ReceiveDataRF() function


	   // --------------------------------------------------------------------------------------
	   //   Function to parse the given string and populate the values integer array
	   //   maxelements is the number of elements in the given values array
	   //   Note: this version assumes that the string is properly formed (i.e. contains only integers seperated by commas)
	   // --------------------------------------------------------------------------------------
void getValues(char * str, long * values, int maxelements)
{
	char *result = NULL;
	int index = 0;
	result = strtok(str, ",");
	while ((result != NULL) && (index < maxelements))
	{
		values[index++] = strtoul(result, NULL, 10);
		result = strtok(NULL, ",");
	}
}

