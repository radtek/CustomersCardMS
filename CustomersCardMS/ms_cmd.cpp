#include "stdafx.h"
#include "ms_cmd.h"
#include "ms_data.h"
#include "ms_memory.h"

void Client_CmdCompletionFailed(void* bobj_, void* _bobj)
{
	BUFFER_OBJ* pBObj = (BUFFER_OBJ*)_bobj;
	RECV_SOCK* pRSock = (RECV_SOCK*)pBObj->pRelateSock;

	if (0 == InterlockedDecrement(&pRSock->nRef))
	{
		CMCloseSocket(pRSock);
		freeRSock(pRSock);
	}
	freeBObj(pBObj);
}

void Client_CmdCompletionSuccess(DWORD dwTranstion, void* bobj_, void* _bobj)
{
	BUFFER_OBJ* bobj = (BUFFER_OBJ*)_bobj;
//	try
//	{
//		msgpack::unpacker unpack_;
//		msgpack::object_handle result_;
//		unpack_.reserve_buffer(bobj->dwRecvedCount - 2);
//		memcpy_s(unpack_.buffer(), bobj->dwRecvedCount - 2, bobj->data, bobj->dwRecvedCount - 2);
//		unpack_.buffer_consumed(bobj->dwRecvedCount - 2);
//		unpack_.next(result_);
//		if (msgpack::type::ARRAY != result_.get().type)
//			goto error;
//
//		msgpack::object* pArray = result_.get().via.array.ptr;
//		bobj->nCmd = (pArray++)->as<int>();
//		switch (bobj->nCmd)
//		{
//		case USER_DATA:
//		{
//			cmd_user(pArray, bobj);
//			return;
//		}
//		break;
//		case KH_DATA:
//		{
//			cmd_kh(pArray, bobj);
//			return;
//		}
//		break;
//		case KHJL_DATA:
//		{
//			cmd_khjl(pArray, bobj);
//			return;
//		}
//		break;
//		case EXCEL_LOAD:
//		{
//			cmd_load(pArray, bobj);
//			return;
//		}
//		case SIM_DATA:
//		{
//			cmd_sim(pArray, bobj);
//			return;
//		}
//		break;
//		case SSDQ_DATA:
//		{
//			cmd_ssdq(pArray, bobj);
//			return;
//		}
//		break;
//		case LLTC_DATA:
//		{
//			cmd_lltc(pArray, bobj);
//			return;
//		}
//		break;
//		case LLC_DATA:
//		{
//			cmd_llc(pArray, bobj);
//			return;
//		}
//		break;
//		case DXZH_DATA:
//		{
//			cmd_dxzh(pArray, bobj);
//			return;
//		}
//		break;
//		case BEAT_DATA:
//		{
//			cmd_beat(pArray, bobj);
//			return;
//		}
//		break;
//		case DISABLE_NUMBER_DATA:
//		{
//			cmd_disnumber(pArray, bobj);
//			return;
//		}
//		break;
//		case CARD_STATUS_DATA:
//		{
//			cmd_cardstatus(pArray, bobj);
//			return;
//		}
//		break;
//		default:
//			break;
//		}
//	}
//	catch (msgpack::type_error e)
//	{
//		_tprintf(_T("msgpack::type_error\n"));
//	}
//	catch (msgpack::unpack_error e)
//	{
//		_tprintf(_T("msgpack::unpack_error\n"));
//	}
//	catch (const std::exception&)
//	{
//
//	}
//
//error:
//	msgpack::sbuffer sbuf;
//	msgpack::packer<msgpack::sbuffer> msgPack(&sbuf);
//	sbuf.write("\xfb\xfc", 6);
//	msgPack.pack_array(3);
//	msgPack.pack(0xbb);
//	msgPack.pack(0xbb);
//	msgPack.pack(0);
//
//	DealTail(sbuf, bobj);
}