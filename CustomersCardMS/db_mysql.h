#pragma once
#include <mysql.h>

#define DB_HOST			_T("localhost")
#define DB_NAME			_T("cardb")
#define DB_USER			_T("baolan123")
#define DB_PASSWORD		_T("baolan123")
#define DB_CHARSER		_T("gbk")
#define DB_PORT			3306

#define DB_LINK_MAX		50
#define DB_LINK_MIN		10

bool db_init();

// 创建数据库 cardb
bool db_create(const TCHAR* host, const TCHAR* use, const TCHAR* password, unsigned int port);

// 连接数据库 cardb
MYSQL* db_connect(const TCHAR* host, const TCHAR* user, const TCHAR* password, const TCHAR* db, unsigned int port);

// 从连接池中分配可用的连接
MYSQL* db_alloc();

// 将连接返回到连接池中
void db_free(MYSQL* pMysql);