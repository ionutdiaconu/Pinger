#pragma once

#include <iostream>
#include <pqxx/pqxx> 

class DBManager
{

private:
	pqxx::connection con;

public:

	DBManager();
	~DBManager();

	void insert(std::string host, std::string time);
	std::vector<std::string> getPingDates();

};

