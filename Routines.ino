// --------------------------------------------------------------------------------------
//   Initialization routine 1
// --------------------------------------------------------------------------------------
void SystemInit1()
{

	FirstRunCheck = 0xAC;
	EEPROM.write(FirstRunCheck_eeprom_adr, 0xAC);
}

// --------------------------------------------------------------------------------------
//   Initialization routine 2
// --------------------------------------------------------------------------------------
void SystemInit2(int ano, byte dia)
{
	eepromWriteInt(YearNow_eeprom_adr, ano);
	EEPROM.write(DayNow_eeprom_adr, dia);
	unsigned int initRain = Unit[RAIN_OutUnit].TotalRain_tips;
	eepromWriteInt(RainDayBegin_tips_eeprom_adr, initRain);
	eepromWriteInt(RainMonthBegin_tips_eeprom_adr, initRain);
	eepromWriteInt(RainYearBegin_tips_eeprom_adr, initRain);
	eepromWriteInt(thisYearRain_tips_eeprom_adr, 0);
	eepromWriteInt(TotalRain_tips_eeprom_adr, initRain);
	FirstRunInit = 0xAC;
	FirstRunCheck = 0xAC;
	EEPROM.write(FirstRunCheck_eeprom_adr, 0xAC);
	EEPROM.write(FirstRunInit_eeprom_adr, 0xAC);
}


// --------------------------------------------------------------------------------------
//   Rain routines
// --------------------------------------------------------------------------------------
void Rain_updates(unsigned int ano, byte mes, byte dia, byte horas, byte minutos)
{
	static unsigned int last10min_TotalRain_tips;

#if RAIN_OutUnit == 3
	// --- Resets eeprom total rain tips counter when bateries are changed in Auriol pluviometer ----
	if (RainYearBegin_tips != 0 && Unit[RAIN_OutUnit].TotalRain_tips == 0)
	{
		//Serial.println("Reset_RainData_eeprom");
		for (byte i = 110; i < 154; i++) EEPROM.write(i, 0);
		RainDayBegin_tips = 0;
	}
#endif

	if (ano != YearNow_Eeprom)
	{
		eepromWriteInt(YearNow_eeprom_adr, ano);
		YearNow_Eeprom = ano;
		eepromWriteInt(RainYearBegin_tips_eeprom_adr, Unit[RAIN_OutUnit].TotalRain_tips);
		RainYearBegin_tips = Unit[RAIN_OutUnit].TotalRain_tips;
	}

	if (mes != MonthNow_Eeprom)
	{
		EEPROM.write(MonthNow_eeprom_adr, mes);
		MonthNow_Eeprom = mes;
		eepromWriteInt(RainMonthBegin_tips_eeprom_adr, Unit[RAIN_OutUnit].TotalRain_tips);
		RainMonthBegin_tips = Unit[RAIN_OutUnit].TotalRain_tips;
	}

	if (dia != DayLastRain_tip && !rolloverDone)   // Prevents false rain readings when the RX unit is powered down in a rainy day and reconnected in a different day
	{
		RainDayBegin_tips = Unit[RAIN_OutUnit].TotalRain_tips;
		eepromWriteInt(RainDayBegin_tips_eeprom_adr, RainDayBegin_tips);
		//eepromWriteInt(DaysWithoutRain_eeprom_adr, -1);
		rolloverDone = true;
	}

	RainToday_tips = Unit[RAIN_OutUnit].TotalRain_tips - RainDayBegin_tips;

	Output_RToday = RainToday_tips * (COLLECTOR_TYPE[RAIN_OutUnit] / 1000.0);
	if (RainToday_tips > 0) DaysWithoutRain = 0;

	if (horas == MeteoDay_HStart && dia != DayNow_Eeprom)
	{
		if (eepromReadInt(RainDayBegin_tips_eeprom_adr) == Unit[RAIN_OutUnit].TotalRain_tips) DaysWithoutRain += 1;
		eepromWriteInt(DaysWithoutRain_eeprom_adr, DaysWithoutRain);
		EEPROM.write(DayNow_eeprom_adr, dia);
		DayNow_Eeprom = dia;
		RainDayBegin_tips = Unit[RAIN_OutUnit].TotalRain_tips;
		eepromWriteInt(RainDayBegin_tips_eeprom_adr, RainDayBegin_tips);
		rolloverDone = true;
		//Serial.println(F("Day rollover done"));
	}

	thisMonthRain_tips = Unit[RAIN_OutUnit].TotalRain_tips - RainMonthBegin_tips;
	thisYearRain_tips = Unit[RAIN_OutUnit].TotalRain_tips - RainYearBegin_tips;

	if (minutos % 10 == 0 && (Unit[RAIN_OutUnit].TotalRain_tips > last10min_TotalRain_tips))
	{
		eepromWriteInt(TotalRain_tips_eeprom_adr, Unit[RAIN_OutUnit].TotalRain_tips);
		eepromWriteInt(thisYearRain_tips_eeprom_adr, thisYearRain_tips);
		EEPROM.write(DayLastRain_tips_eeprom_adr, dia);
		//Serial.println("Chuva 10min");
		last10min_TotalRain_tips = Unit[RAIN_OutUnit].TotalRain_tips;
	}

	loopData.rainRate = Unit[RAIN_OutUnit].Rainftipshour;           // Rainfall in tips per hour
	loopData.dayRain = RainToday_tips;                             // Total rain tips today
	loopData.monthRain = thisMonthRain_tips;                         // Total rain tips this month
	loopData.yearRain = thisYearRain_tips;                          // Total rain tips this year

	if (loopData.rainRate > archHighRainRate) archHighRainRate = loopData.rainRate;
}


// --------------------------------------------------------------------------------------
//   Read barometric Sensor BMP085
// --------------------------------------------------------------------------------------
void read_BMP()
{
	float RAW_pressure;
	//TempIntBMP = bmp.readTemperature();
	RAW_pressure = bmp.readPressure();
	SeaLevel_ATM = ((RAW_pressure / pow((1.0 - ((float)(ELEVATION)) / 44330.0), 5.255)) / 100.0) + SeaLevel_ATM_offset; // return the equivalent pressure (mb) at sea level.
	loopData.barometer = (unsigned int)(SeaLevel_ATM * 29.52998);

#if RAIN_OutUnit != 3
	RAW_pressure_inHg = (unsigned int)(RAW_pressure * 0.29529);   // Pressure in mBar* 100 converted to inHg * 1000
#endif
}


// --------------------------------------------------------------------------------------
//   Read inside temperature / humidity sensor
// --------------------------------------------------------------------------------------
void read_InsideTH_Sensor()
{
#if InsideTH_Sensor == 0
	int ti, hi;
	dht.reading(ti, hi);
	TemperaturaInt = ti + TemperaturaInt_offset;
	Output_H_Int = hi;
	loopData.insideTemperature = (TemperaturaInt * 1.8) + 320;
	loopData.insideHumidity = (Output_H_Int + 5) * 0.1;              // Round the reading

	if (Temp_Display_Unit == 0)
		Output_T_Int = TemperaturaInt;
	else
		Output_T_Int = loopData.insideTemperature;
#endif

#if InsideTH_Sensor == 1
	TemperatureInt = (htu21d.readTemperature() * 10.0) + TemperaturaInt_offset;
	Output_H_Int = htu21d.readHumidity() * 10.0;
	loopData.insideTemperature = (TemperatureInt * 1.8) + 320;
	loopData.insideHumidity = (Output_H_Int + 5) * 0.1;              // Round the reading

	if (Temp_Display_Unit == 0)
		Output_T_Int = TemperatureInt;
	else
		Output_T_Int = loopData.insideTemperature;
#endif

#if InsideTH_Sensor == 2
	TemperaturaInt = (SHT2x.GetTemperature() * 10.0) + TemperaturaInt_offset;
	Output_H_Int = SHT2x.GetHumidity() * 10.0;
	loopData.insideTemperature = (TemperaturaInt * 1.8) + 320;
	loopData.insideHumidity = (Output_H_Int + 5) * 0.1;              // Round the reading

	if (Temp_Display_Unit == 0)
		Output_T_Int = TemperaturaInt;
	else
		Output_T_Int = loopData.insideTemperature;
#endif

#if InsideTH_Sensor == 3
	TemperaturaInt = (sht31.readTemperature() * 10.0) + TemperaturaInt_offset;
	Output_H_Int = sht31.readHumidity() * 10.0;
	loopData.insideTemperature = (TemperaturaInt * 1.8) + 320;
	loopData.insideHumidity = (Output_H_Int + 5) * 0.1;              // Round the reading

	if (Temp_Display_Unit == 0)
		Output_T_Int = TemperaturaInt;
	else
		Output_T_Int = loopData.insideTemperature;
#endif
}



// --------------------------------------------------------------------------------------
//   Read / Write a Int to internal Arduino eeprom
// --------------------------------------------------------------------------------------
int eepromReadInt(int address)
{
	int value = 0x0000;
	value = value | EEPROM.read(address) << 8;
	value = value | EEPROM.read(address + 1);
	return value;
}

void eepromWriteInt(int address, int value)
{
	EEPROM.write(address, (value >> 8) & 0xFF);
	EEPROM.write(address + 1, value & 0xFF);
}




// --------------------------------------------------------------------------------------
//   Erase eeproms
// --------------------------------------------------------------------------------------
void erase_eeproms()
{
	for (byte i = 90; i < 215; i++) EEPROM.write(i, 0);
}


