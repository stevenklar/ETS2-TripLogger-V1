/*
ETS2TripLogger 1.x DLL - TripLogger for Eurotruck Simulator 2 (DLL Version).
Copyright (C) 2016 NV1S1ON

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://opensource.org/licenses/GPL-3.0>.

You can contact me on:
ETS2MP.COM username NV1S1ON
Facebook ETS2TripLogger

------------------------------------------------------------------------------------------------------------------
ETS2 TripLogger V1.x
Connection to MySQL is made directly.  You need to add a reference to the MySQL connector libmysql.dll (download from MySQL website) and place either in same directory or system folder.  
You can comment out all the MySQL as it was still in testing stage, trips will be saved to a CSV file.
You will also need to reference the ETS2 SDK.

Database Name = TripLog
Database Fields = profile_id, profile_name, F_Company, F_City, T_Company, T_City, Cargo, Weight, KM, Fuel, Truck_Make, Truck_Model, trip_days, trip_hours, trip_mins, wear_engine, wear_chassis, wear_trans, wear_trailer

Database Name = Users
Database Fields = Username, Password (SHA1)

Local username and password stored in Software\\NV1S1ON\\ETS2TripLogger\\Settings\\Password and Software\\NV1S1ON\\ETS2TripLogger\\Settings\\Username

If any modification please commit to the GitHub repository.
*/
#define WINVER 0x0500 // Windows XP +
#define _WIN32_WINNT 0x0500 // Windows XP +
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sstream>
#include <iostream>
#include <string>
#include "ETS2Logger.h"
#include "SHA1.h" // CSHA1 Class

// Eurotruck SDK - Current SDK 1.5
#include "scssdk_telemetry.h"
#include "eurotrucks2/scssdk_eut2.h"
#include "eurotrucks2/scssdk_telemetry_eut2.h"

// Convert Floats and Ints to Strings for MySQL
std::stringstream ss_cargoMass, ss_distanceODO, ss_distanceFuel, ss_days, ss_hours, ss_mins, ss_driverName, ss_driverPassword, ss_engine, ss_chassis, ss_trans, ss_trailer;

#pragma unmanaged // Disabled Compiler Warning 1

// Game log file
scs_log_t game_log = NULL;

std::string job_csv_file; // Holder for CSV trip log

#define UNUSED(x)

//const std::string dataServer = "DATABASE HOST";
//const std::string dataUser = "DATABASE USERNAME";
//const std::string dataPass = "DATABASE PASSWORD";
//const std::string dataBase = "DATABASE NAMAE";

unsigned long last_update = 0;

bool last_engine_enabled; // Last Engine State
bool trailer_odo_set = 0; // Last state of odometer set
float trailer_connection_odo = 0; // Current Odometer when trailer is connected
float trailer_distance_odo = 0; // Distance trailer has done
float trailer_fuel_connection_set = 0; // Fuel level when trailer is connected
float trailer_fuel = 0; // Used fuel when trailer is attached
float trailer_fuel_save = 0; // Saves fuel when engine is off and / or when fueling up
float trailer_fuel_litres_100 = 0; // L/100km

long trailer_connected_time; // Set time trailer was connected
int trailer_connected_days = 0; // Days
int trailer_connected_hours = 0; // Hours
int trailer_connected_mins = 0; // Mins

float wear_engine_set = 0; // Set Engine Damage at trailer connect
float wear_trans_set = 0; // Set Trans Damage at trailer connect
float wear_chassis_set = 0; // Set Chassis Damage at trailer connect
float wear_trailer_set = 0; // Set Trailer Damage at trailer connect

float engine_damage = 0;
float chassis_damage = 0;
float trans_damage = 0;
float trailer_damage = 0;

float current_engine_damage = 0;
float current_chassis_damage = 0;
float current_trans_damage = 0;
float current_trailer_damage = 0;

// Website Login Credentials
std::string driver_username;
std::string driver_password;
std::string driver_password_hashed;

bool game_paused = true;

// Telemetry Truck Data
struct telemetry_state_truck
{
	float fuel; 	// Current Fuel Level of truck, Liters
	bool  engine_enabled; // Engine Running - 0 = Off, 1 = On
	bool  parking_brake;
	float odometer; // Kilometres
	float cargo_mass; // Cargo Mass
	std::string cargo; // Cargo ID when on Job, used for signalling when to save after completed job.  Should return NULL when no job
	std::string source_city; // Source City
	std::string source_company; // Source Company
	std::string destination_city; // Destination City
	std::string destination_company; // Destination Company

	bool connected; // Trailer Connected

	float wear_engine; // Engine Damage
	float wear_transmission; // Transmission Damage
	float wear_chassis; // Chassis Damage
	float wear_trailer_chassis; // Trailer Damage

	std::string brand; // Make of Truck
	std::string name; // Model of Truck

	long game_time; // Game Time in Minutes


} telemetry;

std::string HashString(std::string Input) // Hashing for User Passwords SHA1
{
	std::string Password = Input;
	CSHA1 sha1;
	std::string strReport;

	sha1.Update((UINT_8*)Password.c_str(), Password.size() * sizeof(TCHAR));
	sha1.Final();
	sha1.ReportHashStl(strReport, CSHA1::REPORT_HEX_SHORT);

	return strReport;
}

SCSAPI_VOID telemetry_frame_end(const scs_event_t /*event*/, const void *const /*event_info*/, const scs_context_t /*context*/)
{
	const unsigned long now = GetTickCount();
	const unsigned long diff = now - last_update;
	if (diff < 50)
		return;

	last_update = now;

	// Set start odemeter when trailer was connect
	if (telemetry.connected == 1 && trailer_odo_set == 0) // Trailer Connected, Odo Not Set
	{
		// Reset Values
		wear_engine_set = 0;
		wear_chassis_set = 0;
		wear_trans_set = 0;
		wear_trailer_set = 0;
		engine_damage = 0;
		chassis_damage = 0;
		trans_damage = 0;
		trailer_damage = 0;
		current_engine_damage = 0;
		current_chassis_damage = 0;
		current_trans_damage = 0;
		current_trailer_damage = 0;

		trailer_fuel_save = 0; // Clear fuel save
		trailer_fuel = 0;
		trailer_distance_odo = 0;
		trailer_connection_odo = 0;

		trailer_connected_time = 0; // Clear Trailer Connected Time
		trailer_connected_days = 0; // Clear Trailer Connected Days
		trailer_connected_hours = 0; // Clear Trailer Connected Hours
		trailer_connected_mins = 0; // Clear Trailer Connected Mins

		// Settings
		trailer_connection_odo = telemetry.odometer; // Set Odometer
		trailer_fuel_connection_set = telemetry.fuel; // Set Current Fuel
		trailer_connected_time = telemetry.game_time; // Set connection time of trailer

		// Set Wear Levels
		wear_engine_set = telemetry.wear_engine;
		wear_trans_set = telemetry.wear_transmission;
		wear_chassis_set = telemetry.wear_chassis;
		wear_trailer_set = telemetry.wear_trailer_chassis;

		trailer_odo_set = 1; // Odometer has been set
	}

	if (telemetry.connected == 1 && trailer_odo_set == 1 && telemetry.engine_enabled == 0) // Trailer Connected, Odo Set, Engine Not Running - Refueling, Garage, Service etc
	{
		trailer_fuel_save = trailer_fuel;
		trailer_fuel_connection_set = telemetry.fuel;
	}

	if (telemetry.connected == 1 && trailer_odo_set == 1 && telemetry.engine_enabled == 1) // Trailer Connected, Odo Set, Engine Running
	{
		trailer_distance_odo = telemetry.odometer - trailer_connection_odo;
		trailer_fuel = (trailer_fuel_connection_set - telemetry.fuel) + trailer_fuel_save;

		current_engine_damage = telemetry.wear_engine;
		current_chassis_damage = telemetry.wear_chassis;
		current_trans_damage = telemetry.wear_transmission;
		current_trailer_damage = telemetry.wear_trailer_chassis;
	}

	if (telemetry.connected == 0 && trailer_odo_set == 1 && telemetry.parking_brake == true) // Save Data - Only run if trailer is disconnected odometer is set
	{
		loadDeliveryTime(); // Calculate Trip Time in Days, Hours, Mins

		engine_damage = current_engine_damage - wear_engine_set;
		chassis_damage = current_chassis_damage - wear_chassis_set;
		trans_damage = current_trans_damage - wear_trans_set;
		trailer_damage = current_trailer_damage - wear_trailer_set;

		// MySQL Connection 
		mysql_init(&mysql);
		conn = mysql_real_connect(&mysql, dataServer.c_str(), dataUser.c_str(), dataPass.c_str(), dataBase.c_str(), 0, NULL, 0); // Database connection string
		mysql_set_character_set(&mysql, "UTF8"); // Set Character to UTF8, needed for special characters

		// Check if connection to server could be established
		if (conn == NULL)
		{
			// No Connection coul dbe established
			game_log(SCS_LOG_TYPE_error, mysql_error(&mysql));

			saveTripFile(); // Save Trip Information if Error has accured
			game_log(SCS_LOG_TYPE_error, "ETS2TripLogger: Could not connect Online.  Trip Saved to File");
		}
		else
		{
			// SQL Query to check if user is a member using username and Hashed Password and if so, get ID and continue
			std::string loginQuery = "select id from users WHERE username = '" + driver_username + "' AND password = '" + driver_password_hashed + "' LIMIT 1";

			int queryStateLogin; // Login State

			queryStateLogin = mysql_query(conn, loginQuery.c_str()); // Try the Query

			if (queryStateLogin != 0) // Error with Query
			{
				saveTripFile(); // Save Trip Information if Error has accured
				game_log(SCS_LOG_TYPE_error, "ETS2TripLogger: There was a problem with getting user details.  Trip Saved to File");
			}

			MYSQL_RES * resultsetLogin;
			MYSQL_ROW row;

			resultsetLogin = mysql_store_result(conn);
			int numOfUser = mysql_num_rows(resultsetLogin);

			row = mysql_fetch_row(resultsetLogin);

			// Now free the result - needs to be done fore another Query is peformed
			mysql_free_result(resultsetLogin);

			/////////////////////////////////////////////////////////////////////////////

			if (numOfUser != 0) // If there is a row, then there is a matching profile so continue 
			{
				std::string driver_ID = row[0];

				game_log(SCS_LOG_TYPE_message, "ETS2TripLogger: Connected Online, Saving Trip Information");
				int queryState_save;

				//////////////FOR DATABASE INSERT STATEMENT////////////////////////////////

				// Cargo Mass
				ss_cargoMass << telemetry.cargo_mass; // Add value to Stream
				std::string cargoMass = ss_cargoMass.str(); // Output to String Variable
				ss_cargoMass.str("");// Clear StringStream

				// Distance Travelled ODO
				ss_distanceODO << trailer_distance_odo; // Add value to Stream
				std::string distanceODO = ss_distanceODO.str(); // Output to String Variable
				ss_distanceODO.str("");// Clear StringStream

				// Trip Fuel Used
				ss_distanceFuel << trailer_fuel; // Add value to Stream
				std::string distanceFuel = ss_distanceFuel.str(); // Output to String Variable
				ss_distanceFuel.str("");// Clear StringStream

				// Days Trailer Connected
				ss_days << trailer_connected_days; // Add value to Stream
				std::string trip_days = ss_days.str(); // Output to String Variable
				ss_days.str("");// Clear StringStream

				// Hours Trailer Connected
				ss_hours << trailer_connected_hours; // Add value to Stream
				std::string trip_hours = ss_hours.str(); // Output to String Variable
				ss_hours.str("");// Clear StringStream

				// Mins Trailer Connected
				ss_mins << trailer_connected_mins; // Add value to Stream
				std::string trip_mins = ss_mins.str(); // Output to String Variable
				ss_mins.str(""); // Clear StringStream

				//////DAMAGES//////
				// Engine Damage
				ss_engine << engine_damage; // Add value to Stream
				std::string e_engine = ss_engine.str(); // Output to String Variable
				ss_engine.str(""); // Clear StringStream

				// Chassis Damage
				ss_chassis << chassis_damage; // Add value to Stream
				std::string e_chassis = ss_chassis.str(); // Output to String Variable
				ss_chassis.str(""); // Clear StringStream

				// Trans Damage
				ss_trans << trans_damage; // Add value to Stream
				std::string e_trans = ss_trans.str(); // Output to String Variable
				ss_trans.str(""); // Clear StringStream

				// Trailer Damage
				ss_trailer << trailer_damage; // Add value to Stream
				std::string e_trailer = ss_trailer.str(); // Output to String Variable
				ss_trailer.str(""); // Clear StringStream

				std::string AddTrip = "INSERT INTO TripLog (profile_id, profile_name, F_Company, F_City, T_Company, T_City, Cargo, Weight, KM, Fuel, Truck_Make, Truck_Model, trip_days, trip_hours, trip_mins, wear_engine, wear_chassis, wear_trans, wear_trailer) VALUES ( '" + driver_ID + "', '" + driver_username + "','" + telemetry.source_company + "', '" + telemetry.source_city + "', '" + telemetry.destination_company + "', '" + telemetry.destination_city + "', '" + telemetry.cargo + "', '" + cargoMass + "', '" + distanceODO + "', '" + distanceFuel + "', '" + telemetry.brand + "', '" + telemetry.name + "', '" + trip_days + "', '" + trip_hours + "', '" + trip_mins + "', '" + e_engine + "', '" + e_chassis + "', '" + e_trans + "', '" + e_trailer + "')";

				queryState_save = mysql_query(conn, AddTrip.c_str());

				if (queryState_save != 0)
				{
					game_log(SCS_LOG_TYPE_error, mysql_error(&mysql));

					saveTripFile(); // Save Trip Information if Error has accured
					game_log(SCS_LOG_TYPE_error, "ETS2TripLogger: Could not upload trip Online.  Trip Saved to File");
				}
				else
				{
					game_log(SCS_LOG_TYPE_message, "ETS2TripLogger: Trip Saved Online");
				}

				// here, query succeeded, so we can proceed
				MYSQL_RES * resultset;
				resultset = mysql_store_result(conn);

				// Now free the result - needs to be done fore another Query is peformed
				mysql_free_result(resultset);
			}
			else
			{
				game_log(SCS_LOG_TYPE_error, "ETS2TripLogger: Could not find your details.  Make sure Username & Password are correct");
				saveTripFile(); // Save Trip Information if Error has accured
			}
		}
		mysql_close(conn); // Close Connection

		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Reset variables ready for next load
		trailer_odo_set = 0; // Clear trailer odometer set
	}

}
/**
* @param location The location of the registry key.
* @param name the name of the registry key, for example "Installed Path"
* @return the value of the key or an empty string if an error occured.
*/
std::string getRegKey(const std::string& location, const std::string& name){
	HKEY key;
	TCHAR value[1024];
	DWORD bufLen = 1024 * sizeof(TCHAR);
	long ret;
	ret = RegOpenKeyExA(HKEY_CURRENT_USER, location.c_str(), 0, KEY_QUERY_VALUE, &key);
	if (ret != ERROR_SUCCESS){
		return std::string();
	}
	ret = RegQueryValueExA(key, name.c_str(), 0, 0, (LPBYTE)value, &bufLen);
	RegCloseKey(key);
	if ((ret != ERROR_SUCCESS) || (bufLen > 1024 * sizeof(TCHAR))){
		return std::string();
	}
	std::string stringValue = std::string(value, (size_t)bufLen - 1);
	size_t i = stringValue.length();
	while (i > 0 && stringValue[i - 1] == '\0'){
		--i;
	}
	return stringValue.substr(0, i);
}

void saveTripFile() // Save trip information to file
{
	// Save trip to file if no connection to database is established
	FILE * pFileLog1; // Holder for Trip Log File

	// CSV Format
	pFileLog1 = fopen(job_csv_file.c_str(), "a + "); // File name, Open or Create file and append
	fprintf(pFileLog1, "%s,%s,%s,%s,%s,% 0.1f,% 0.1f,% 0.1f,%s,%s,%u,%u,%u \n", telemetry.source_company.c_str(), telemetry.source_city.c_str(), telemetry.destination_company.c_str(), telemetry.destination_city.c_str(), telemetry.cargo.c_str(), telemetry.cargo_mass, trailer_distance_odo, trailer_fuel, telemetry.brand.c_str(), telemetry.name.c_str(), trailer_connected_days, trailer_connected_hours, trailer_connected_mins); // Distance and Fuel to file
	fclose(pFileLog1); // Close CSV Trip Log File

	game_log(SCS_LOG_TYPE_message, "ETS2TripLogger: Trip Information Saved to file"); // Message to Console in game
}

void loadDeliveryTime() // Caluclate Days, Hours, Mins from game minutes
{
	long input_seconds = (telemetry.game_time - trailer_connected_time) * 60;
	trailer_connected_days = input_seconds / 60 / 60 / 24; // Days
	trailer_connected_hours = (input_seconds / 60 / 60) % 24; // Hours
	trailer_connected_mins = (input_seconds / 60) % 60; // Mins
}

SCSAPI_VOID telemetry_pause(const scs_event_t event, const void *const UNUSED(event_info), const scs_context_t UNUSED(context))
{
	game_paused = (event == SCS_TELEMETRY_EVENT_paused);

}

SCSAPI_VOID telemetry_store_float(const scs_string_t /*name*/, const scs_u32_t /*index*/, const scs_value_t *const value, const scs_context_t context)
{
	assert(value);
	assert(value->type == SCS_VALUE_TYPE_float);
	assert(context);
	*static_cast<float *>(context) = value->value_float.value;
}

SCSAPI_VOID telemetry_store_bool(const scs_string_t /*name*/, const scs_u32_t /*index*/, const scs_value_t *const value, const scs_context_t context)
{
	// Results in a default value of false if telemetry value is NULL
	if (value)
	{
		if (value->value_bool.value == 0)
		{
			*static_cast<bool *>(context) = false;
		}
		else
		{
			*static_cast<bool *>(context) = true;
		}
	}
	else
	{
		*static_cast<bool *>(context) = false;
	}
}

SCSAPI_VOID telemetry_store_u64(const scs_string_t /*name*/, const scs_u32_t /*index*/, const scs_value_t *const value, const scs_context_t context)
{
	assert(value);
	assert(value->type == SCS_VALUE_TYPE_u64);
	assert(context);
	*static_cast<int *>(context) = value->value_u64.value;
}

SCSAPI_VOID telemetry_store_s32(const scs_string_t /*name*/, const scs_u32_t /*index*/, const scs_value_t *const value, const scs_context_t context)
{
	assert(value);
	assert(value->type == SCS_VALUE_TYPE_s32);
	assert(context);
	*static_cast<int *>(context) = value->value_s32.value;
}

SCSAPI_VOID telemetry_store_u32(const scs_string_t /*name*/, const scs_u32_t /*index*/, const scs_value_t *const value, const scs_context_t context)
{
	assert(value);
	assert(value->type == SCS_VALUE_TYPE_u32);
	assert(context);
	*static_cast<unsigned int *>(context) = value->value_u32.value;
}

SCSAPI_VOID telemetry_configuration(const scs_event_t /*event*/, const void *const event_info, const scs_context_t /*context*/)
{
	const struct scs_telemetry_configuration_t *const info = static_cast<const scs_telemetry_configuration_t *>(event_info);

	for (const scs_named_value_t *current = info->attributes; current->name; ++current)
	{

#define GET_CONFIG(PROPERTY, TYPE) \
		if (strcmp(SCS_TELEMETRY_CONFIG_ATTRIBUTE_ ## PROPERTY, current->name) == 0) \
		{ telemetry.PROPERTY = current->value.value_ ## TYPE.value; }

		GET_CONFIG(cargo, string); // Type of Cargo
		GET_CONFIG(source_city, string); // Source City
		GET_CONFIG(source_company, string); // Source Company
		GET_CONFIG(destination_city, string); // Destination City
		GET_CONFIG(destination_company, string); // Destination Company
		GET_CONFIG(cargo_mass, float); // Cargo mass in Kilograms
		GET_CONFIG(brand, string); // Truck Make
		GET_CONFIG(name, string); // Truck Model
	}
}

SCSAPI_RESULT scs_telemetry_init(const scs_u32_t version, const scs_telemetry_init_params_t *const params)
{
	if (version != SCS_TELEMETRY_VERSION_1_00) {
		return SCS_RESULT_unsupported;
	}

	const scs_telemetry_init_params_v100_t *const version_params = static_cast<const scs_telemetry_init_params_v100_t *>(params);
	game_log = version_params->common.log;
	game_log(SCS_LOG_TYPE_message, "ETS2TripLogger v0.1.3 Initialising");

	// Get User Values from Registry
	driver_username = getRegKey("Software\\NV1S1ON\\ETS2TripLogger\\Settings", "UserName");
	driver_password = getRegKey("Software\\NV1S1ON\\ETS2TripLogger\\Settings", "Password");
	driver_password_hashed = HashString(driver_password.c_str()); // Hashed Password 

	job_csv_file = (getRegKey("Software\\NV1S1ON\\ETS2TripLogger\\Settings", "InstallDir") + "\\Logs\\ETS2TripLog.ett"); // Trip Log CSV

	// Register for in game events
	bool registered =
		(version_params->register_for_event(SCS_TELEMETRY_EVENT_frame_end, telemetry_frame_end, NULL) == SCS_RESULT_ok) &&
		(version_params->register_for_event(SCS_TELEMETRY_EVENT_paused, telemetry_pause, NULL) == SCS_RESULT_ok) &&
		(version_params->register_for_event(SCS_TELEMETRY_EVENT_started, telemetry_pause, NULL) == SCS_RESULT_ok);
	(version_params->register_for_event(SCS_TELEMETRY_EVENT_configuration, telemetry_configuration, NULL) == SCS_RESULT_ok);

	// Register for truck channels
#define REG_CHAN(CHANNEL, TYPE) \
	registered &= (version_params->register_for_channel(\
	SCS_TELEMETRY_TRUCK_CHANNEL_ ## CHANNEL, SCS_U32_NIL, SCS_VALUE_TYPE_ ## TYPE, \
	SCS_TELEMETRY_CHANNEL_FLAG_none, telemetry_store_ ## TYPE, &telemetry.CHANNEL) == SCS_RESULT_ok)

	REG_CHAN(parking_brake, bool);
	REG_CHAN(fuel, float);
	REG_CHAN(engine_enabled, bool);
	REG_CHAN(odometer, float);

	// Register for Trailer and Game Time Channels
	registered &= (version_params->register_for_channel(SCS_TELEMETRY_TRAILER_CHANNEL_connected, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_no_value, telemetry_store_bool, &telemetry.connected) == SCS_RESULT_ok);
	registered &= (version_params->register_for_channel(SCS_TELEMETRY_CHANNEL_game_time, SCS_U32_NIL, SCS_VALUE_TYPE_u32, SCS_TELEMETRY_CHANNEL_FLAG_no_value, telemetry_store_u32, &telemetry.game_time) == SCS_RESULT_ok);
	// Register Wear Channels
	registered &= (version_params->register_for_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wear_engine, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, telemetry_store_float, &telemetry.wear_engine) == SCS_RESULT_ok);
	registered &= (version_params->register_for_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wear_transmission, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, telemetry_store_float, &telemetry.wear_transmission) == SCS_RESULT_ok);
	registered &= (version_params->register_for_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wear_chassis, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, telemetry_store_float, &telemetry.wear_chassis) == SCS_RESULT_ok);
	registered &= (version_params->register_for_channel(SCS_TELEMETRY_TRAILER_CHANNEL_wear_chassis, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, telemetry_store_float, &telemetry.wear_trailer_chassis) == SCS_RESULT_ok);

	// Register 
	if (!registered)
	{
		game_log(SCS_LOG_TYPE_error, "ETS2TripLogger Unable to register channels");
		return SCS_RESULT_generic_error;
	}

	memset(&telemetry, 0, sizeof(telemetry));

	return SCS_RESULT_ok;
}

SCSAPI_VOID scs_telemetry_shutdown(void)
{
	game_log(SCS_LOG_TYPE_message, "ETS2TripLogger Shutdown"); // ETS2Arduino Shutdown in SCS Log
	game_log = NULL;
}

// Detatch ETS2Arduino DLL
BOOL APIENTRY DllMain(HMODULE /*module*/, DWORD reason_for_call, LPVOID /*reseved*/)
{
	if (reason_for_call == DLL_PROCESS_DETACH)
	{
		//
	}
	return TRUE;
}