#include "DBManager.h"

#include <iostream>
#include <pqxx/pqxx> 


using std::cout;
using std::endl;
using std::cerr;
using std::string;

using pqxx::connection;
using pqxx::work;



std::string getCurrentTime() {

	auto now = std::chrono::system_clock::now();
	std::time_t time = std::chrono::system_clock::to_time_t(now);

	char s[50];

	if (ctime_s(s, 50, &time))
	{
		throw std::runtime_error("Error converting time");
	}
	
	return s;
}



DBManager::DBManager() : 
	con("dbname = pinger user = pinger password = pinger hostaddr = 192.168.1.80 port = 5432")
{
	if (con.is_open()) 
	{
			cout << "Opened database successfully: " << con.dbname() << endl;
	}
	else 
	{	
		throw std::runtime_error("Can't open database");
	}
}

DBManager::~DBManager()
{
	con.close();
}


void DBManager::insert(string host, string cost)
{
	string sql;

	try {
		
		sql = "INSERT INTO ping_events (host, datetime, cost) VALUES ('" + host + "', '" + getCurrentTime() + "', '" + cost + "');";

		work work(con);

		work.exec(sql);
		work.commit();
	}
	catch (const std::exception& e) {
		cerr << e.what() << std::endl;
	}
}


