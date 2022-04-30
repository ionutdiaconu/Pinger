#include "DBManager.h"

#include <iostream>
#include <pqxx/pqxx> 

#include <ctime>

using std::cout;
using std::endl;
using std::cerr;
using std::string;

using pqxx::connection;
using pqxx::work;



//std::string FormatTime(std::chrono::system_clock::time_point tp) {
//	std::stringstream ss;
//	auto t = std::chrono::system_clock::to_time_t(tp);
//	auto tp2 = std::chrono::system_clock::from_time_t(t);
//	if (tp2 > tp)
//		t = std::chrono::system_clock::to_time_t(tp - std::chrono::seconds(1));
//	ss << std::put_time(std::localtime_s(&t), "%Y-%m-%d %T")
//		<< "." << std::setfill('0') << std::setw(3)
//		<< (std::chrono::duration_cast<std::chrono::milliseconds>(
//			tp.time_since_epoch()).count() % 1000);
//	return ss.str();
//}

std::string CurrentTimeStr() {
	//return FormatTime(std::chrono::system_clock::now());

	time_t now = time(0);

	return std::to_string(now);
}

void DBManager::insert(string host, string cost)
{
	string sql;

	try {
		connection C("dbname = pinger user = pinger password = pinger \
      hostaddr = 192.168.1.80 port = 5432");
		if (C.is_open()) {
			cout << "Opened database successfully: " << C.dbname() << endl;
		}
		else {
			cout << "Can't open database" << endl;
			//return 1;
		}

		/* Create SQL statement */
		sql = "INSERT INTO ping_events (host, datetime, cost) VALUES ('" + host + "', '" + CurrentTimeStr() + "', '" + cost + "');";

		/* Create a transactional object. */
		work W(C);


		/* Execute SQL query */
		W.exec(sql);
		W.commit();
		cout << "Records created successfully" << endl;
		C.close();
	}
	catch (const std::exception& e) {
		cerr << e.what() << std::endl;
		//return 1;
	}

}