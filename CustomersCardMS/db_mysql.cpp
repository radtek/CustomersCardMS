#include "stdafx.h"
#include "db_mysql.h"
#include <vector>

#pragma comment(lib, "libmysql.lib")

std::vector<MYSQL*>		v_db;
CRITICAL_SECTION		cs_db;
HANDLE					hDbWait = NULL;
int g_nCurrentLinkNumber = 0;

bool db_init()
{
	v_db.clear();
	InitializeCriticalSection(&cs_db);
//	hDbWait = CreateEvent(NULL, TRUE, FALSE, NULL);

	for (int i = 0; i < DB_LINK_MIN; i++)
	{
		MYSQL* pMysql = db_connect(DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_PORT);
		if (NULL != pMysql)
		{
			EnterCriticalSection(&cs_db);
			v_db.push_back(pMysql);
			++g_nCurrentLinkNumber;
			LeaveCriticalSection(&cs_db);
		}
	}

	return true;
}

bool db_create(const TCHAR* host, const TCHAR* use, const TCHAR* password, unsigned int port)
{
	return true;
}

MYSQL* db_connect(const TCHAR* host, const TCHAR* user, const TCHAR* password, const TCHAR* db, unsigned int port)
{
	MYSQL* pMysql = mysql_init((MYSQL*)NULL);
	if (NULL == pMysql)
	{
		_tprintf_s(_T("%s__mysql_ini失败\n"), __FUNCTION__);
		return pMysql;
	}

	if (!mysql_real_connect(pMysql, host, user, password, db, port, NULL, 0))
	{
		_tprintf_s(_T("%s__mysql_real_connect失败：coder=%d\nmsg=%s\r\n\r\n"), __FUNCTION__, mysql_errno(pMysql), mysql_error(pMysql));
		mysql_close(pMysql);
		pMysql = NULL;
		return pMysql;
	}

	if (!mysql_set_character_set(pMysql, "gbk"))
	{
		_tprintf_s(_T("%s__mysql_set_character_set失败：coder=%d\nmsg=%s\r\n\r\n"), __FUNCTION__, mysql_errno(pMysql), mysql_error(pMysql));
		mysql_close(pMysql);
		pMysql = NULL;
		return pMysql;
	}
	return pMysql;
}

MYSQL* db_alloc()
{
	while (true)
	{
		MYSQL* pMysql = NULL;
		EnterCriticalSection(&cs_db);
		if (v_db.empty())
		{
			if (g_nCurrentLinkNumber < DB_LINK_MAX)
			{
				pMysql = db_connect(DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_PORT);
				if (NULL == pMysql)
				{
					LeaveCriticalSection(&cs_db);
					return pMysql;
				}
				++g_nCurrentLinkNumber;
				LeaveCriticalSection(&cs_db);
			}
			else
			{
				//ResetEvent(hDbWait);
				//LeaveCriticalSection(&cs_db);
				//WaitForSingleObject(hDbWait, INFINITE);
				//continue;
				LeaveCriticalSection(&cs_db);
				_tprintf_s(_T("***数据库连接已经被使用完***\n"));
			}
		}
		else
		{
			pMysql = v_db.back();
			v_db.pop_back();
			LeaveCriticalSection(&cs_db);
			if (0 != mysql_ping(pMysql))
			{
				mysql_close(pMysql);
				pMysql = db_connect(DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_PORT);
				if (NULL == pMysql)
				{
					EnterCriticalSection(&cs_db);
					--g_nCurrentLinkNumber;
					LeaveCriticalSection(&cs_db);
				}
			}
		}

		return pMysql;
	}
}

void db_free(MYSQL* pMysql)
{
	EnterCriticalSection(&cs_db);
	v_db.push_back(pMysql);
	LeaveCriticalSection(&cs_db);
//	SetEvent(hDbWait);
}