#ifndef __DB_H__
#define __DB_H__
#include <sqlite3.h>
#include <vector>
#include "user.h"
void createTable(sqlite3 *ppdb);
void insertTable(sqlite3 *ppdb);
int callback(void *arg, int num, char **values, char **name);
void SelectData(sqlite3 *ppdb,std::vector<User>& facedb);
void loadFaceDb(std::vector<User>& facedb);
#endif