#include "DBManager.h"

#include <iostream>
#include <pqxx/pqxx> 

using std::cout;
using std::endl;
using std::cerr;
using std::string;

using  pqxx::connection;
using pqxx::work;

void DBManager::dbtest() {
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
        C.close();
    }
    catch (const std::exception& e) {
        cerr << e.what() << std::endl;
       // return 1;
    }
}


void DBManager::insert(string host, string time)
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
        sql = "INSERT INTO ping_events (host,time) VALUES ('" + host + "', '" + time + "');";

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