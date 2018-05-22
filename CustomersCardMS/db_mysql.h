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

// �������ݿ� cardb
bool db_create(const TCHAR* host, const TCHAR* use, const TCHAR* password, unsigned int port);

// �������ݿ� cardb
MYSQL* db_connect(const TCHAR* host, const TCHAR* user, const TCHAR* password, const TCHAR* db, unsigned int port);

// �����ӳ��з�����õ�����
MYSQL* db_alloc();

// �����ӷ��ص����ӳ���
void db_free(MYSQL* pMysql);