ETS2 TripLogger V1.x
Connection to MySQL is made directly.  You need to add a reference to the MySQL connector libmysql.dll (download from MySQL website).
You will also need to reference the ETS2 SDK.

Database Name = TripLog
Database Fields = profile_id, profile_name, F_Company, F_City, T_Company, T_City, Cargo, Weight, KM, Fuel, Truck_Make, Truck_Model, trip_days, trip_hours, trip_mins, wear_engine, wear_chassis, wear_trans, wear_trailer

Database Name = Users
Database Fields = Username, Password (SHA1)

Local username and password stored in Software\\NV1S1ON\\ETS2TripLogger\\Settings\\Password and Software\\NV1S1ON\\ETS2TripLogger\\Settings\\Username

If any modification please commit to the GitHub repository.