/*	
  ============================================================================
  Name        : demo2.cpp
  Author      : wuj 
  Version     : the second demo for oci programme
  Copyright   : nstv
  Description : connnect oracle for dbuser1/1@orcl
				start transaction
				exec sql select employee_id, first_name from employees where employee_id<102
				end transaction(commmit/rollback)
				disconnect oracle
  ============================================================================
*/
/**
vim /usr/lib/oracle/11.2/client64/network/admin/tnsnames.ora

ORCL =                                                                                                                       
  (DESCRIPTION =
    (ADDRESS = (PROTOCOL = TCP)(HOST = 192.168.4.84)(PORT = 1521))
    (CONNECT_DATA =
      (SERVER = DEDICATED)
      (SERVICE_NAME = orcl)
    )
  )
  CDCASDB =
  (DESCRIPTION =
    (ADDRESS = (PROTOCOL = TCP)(HOST = 192.168.4.90)(PORT = 1521))
    (CONNECT_DATA =
      (SERVER = DEDICATED)
      (SERVICE_NAME = cdcasdb)
    )
  )
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <oci.h>//oci头文件
#include "NstvException.h"


using namespace std;
static OCIEnv		*p_env;//oci环境句柄
static OCIServer	*p_server;//oci服务器句柄
static OCIError 	*p_err;//oci错误句柄
static OCISession	*p_session;//oci用户会话句柄
static OCISvcCtx	*p_svc;//oci上下文句柄
static OCITrans 	*p_trans;//oci事务句柄
static OCIStmt		*p_stmt1;//oci语句句柄 select 
static OCIStmt		*p_stmt2;//oci语句句柄 insert
static OCIStmt		*p_stmt3;//oci语句句柄 delete
static OCIDefine	*p_dfn1 = (OCIDefine *)0;//oci输出变量
static OCIDefine	*p_dfn2 = (OCIDefine *)0;
static OCIBind		*p_bnd1 = (OCIBind *)0;//oci绑定句柄，用于绑定SQL语句里的变量(占位符)
static OCIBind		*p_bnd2 = (OCIBind *)0;
static OCIBind		*p_bnd3 = (OCIBind *)0;

string sid = "orcl";
string user = "dbuser1";
string passwd = "1";

#define WUJDEBUG printf

void printOCIErr(dvoid *p_err, const char *funcName, int rc){
	char errbuf[100]; //存储错误信息
	int	errcode; //错误号
	OCIErrorGet((dvoid *)p_err, (ub4)1, (text *)NULL, &errcode, (OraText*)errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR);
	WUJDEBUG("函数%s 返回值：%d: 错误号: %d, 错误信息: %s\n", funcName, rc, errcode, errbuf);
}

int main()
{
	int		p_x;//employee_id = p_x
	int		p_id;
	const char	*p_name = "wuj";
	char	*res_name;//存储SQL查询语句后的结果
	int		*res_id;
	int 	rc;	//接收OCI-api返回值
	char	mysql_select[100];//储存SQL语句
	char	mysql_insert[100];
	char	mysql_delete[100];
	ub2 	stmt_type1;//获取SQL语句的类型 select
	ub2 	stmt_type2;//insert
	ub2 	stmt_type3;//delete

	res_id = (int *)malloc(4);
	memset(res_id, 0, 4);
	
	res_name = (char *)malloc(20);
	memset(res_name, 0, 20);

	/*创建oci环境句柄*/
	rc = OCIEnvCreate((OCIEnv **)&p_env, OCI_THREADED|OCI_OBJECT,(dvoid *)0,
			(dvoid * (*)(dvoid *, size_t))0,
			(dvoid * (*)(dvoid *, dvoid *, size_t))0,
			(void (*)(dvoid *, dvoid *))0,
			(size_t)0, (dvoid **)0);
	WUJDEBUG("oci环境句柄创建: OCIEncCreate()rc=%d\n", rc);
	if(rc != 0){
		goto OCIErr;
	}
	
	/*创建oci错误句柄*/
	if (!NstvException::checkErr(p_err, OCIHandleAlloc((dvoid *)p_env, (dvoid **)&p_err, OCI_HTYPE_ERROR, (size_t)0, (dvoid **)0)))
		throw NstvException();
	WUJDEBUG("oci错误句柄创建: OCIHandleAlloc()\n");
	
	/*创建oci服务器句柄*/
	rc = OCIHandleAlloc((dvoid *)p_env, (dvoid **)&p_server, OCI_HTYPE_SERVER, (size_t)0, (dvoid **)0);
	WUJDEBUG("oci服务器句柄创建: OCIHandleAlloc()rc=%d\n", rc);
	if(rc != 0){
		goto OCIErr;
	}
	
	/*创建一个oracl连接路径, 多用户方式连接*/
	if (!NstvException::checkErr(p_err, OCIServerAttach(p_server, p_err, (text *)sid.c_str(), strlen(sid.c_str()), OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci创建连接路径: OCIServerAttach()\n");

	/*创建oci上下文句柄, 并且为其服务器句柄设置属性*/
	rc = OCIHandleAlloc((dvoid *)p_env, (dvoid **)&p_svc, OCI_HTYPE_SVCCTX, (size_t)0, (dvoid **)0);
	WUJDEBUG("oci上下文句柄创建: OCIHandleAlloc()rc=%d\n", rc);
	if(rc != 0){
		goto OCIErr;
	}
	if (!NstvException::checkErr(p_err, OCIAttrSet((dvoid *)p_svc, OCI_HTYPE_SVCCTX, (dvoid *)p_server, (ub4)0, OCI_ATTR_SERVER, (OCIError *)p_err)))
		throw NstvException();
	WUJDEBUG("oci为上下文句柄设置服务器属性: OCIHandleAlloc()\n");
	
	/*创建oci用户会话句柄, 并且为会话句柄设置登录名及密码*/
	rc = OCIHandleAlloc((dvoid *)p_env, (dvoid **)&p_session, (ub4)OCI_HTYPE_SESSION, (size_t)0, (dvoid **)0);
	WUJDEBUG("oci创建用户会话句柄: OCIHandleAlloc()rc=%d\n", rc);
	if(rc != 0){
		goto OCIErr;
	}
	if (!NstvException::checkErr(p_err, OCIAttrSet((dvoid *)p_session, (ub4)OCI_HTYPE_SESSION, (dvoid *)user.c_str(), (ub4)strlen(user.c_str()), (ub4)OCI_ATTR_USERNAME, p_err)))
		throw NstvException();
	WUJDEBUG("oci为会话句柄设置用户: OCIHandleAlloc()\n");
	if (!NstvException::checkErr(p_err, OCIAttrSet((dvoid *)p_session, (ub4)OCI_HTYPE_SESSION, (dvoid *)passwd.c_str(), (ub4)strlen(passwd.c_str()), (ub4)OCI_ATTR_PASSWORD, p_err)))
		throw NstvException();
	WUJDEBUG("oci为会话句柄设置密码: OCIAttrSet()\n");
	
	/*验证用户并且为用户建立会话连接*/
	if (!NstvException::checkErr(p_err, OCISessionBegin(p_svc, p_err, p_session, OCI_CRED_RDBMS, (ub4)OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci验证用户并且建立会话连接: OCISessionBegin()\n");
	
	/*为oci上下文句柄设置会话认证环境属性*/
	if (!NstvException::checkErr(p_err, OCIAttrSet((dvoid *)p_svc, (ub4) OCI_HTYPE_SVCCTX, (dvoid *)p_session, (ub4)0, (ub4)OCI_ATTR_SESSION, p_err)))
		throw NstvException();
	WUJDEBUG("oci上下文句柄设置会话认证环境属性: OCIAttrSet()\n");
	
	/*创建oci语句句柄*/
	rc = OCIHandleAlloc((dvoid *)p_env, (dvoid **)&p_stmt1, OCI_HTYPE_STMT, (size_t)0, (dvoid **)0);
	WUJDEBUG("oci创建语句句柄1: OCIHandleAlloc()rc=%d\n", rc);
	if(rc != 0){
		goto OCIErr;
	}
	rc = OCIHandleAlloc((dvoid *)p_env, (dvoid **)&p_stmt2, OCI_HTYPE_STMT, (size_t)0, (dvoid **)0);
	WUJDEBUG("oci创建语句句柄2: OCIHandleAlloc()rc=%d\n", rc);
	if(rc != 0){
		goto OCIErr;
	}

	rc = OCIHandleAlloc((dvoid *)p_env, (dvoid **)&p_stmt3, OCI_HTYPE_STMT, (size_t)0, (dvoid **)0);
	WUJDEBUG("oci创建语句句柄2: OCIHandleAlloc()rc=%d\n", rc);
	if(rc != 0){
		goto OCIErr;
	}
	
	/*准备SQL语句*/
	strcpy(mysql_select, "select employee_id, first_name from employees where employee_id<:x");//冒号x是变量(占位符)
	if (!NstvException::checkErr(p_err, OCIStmtPrepare(p_stmt1, p_err, (text *)mysql_select, (ub4)strlen(mysql_select), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci准备SQL语句1: OCIStmtPrepare()\n");
	strcpy(mysql_insert, "insert into employees(employee_id, first_name) values(:id, :name)");
	
	if (!NstvException::checkErr(p_err, OCIStmtPrepare(p_stmt2, p_err, (text *)mysql_insert, (ub4)strlen(mysql_insert), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci准备SQL语句2: OCIStmtPrepare()\n");

	strcpy(mysql_delete, "delete employees where employee_id = 1 and first_name = 'wuj'");
	if (!NstvException::checkErr(p_err, OCIStmtPrepare(p_stmt3, p_err, (text *)mysql_delete, (ub4)strlen(mysql_delete), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci准备SQL语句3: OCIStmtPrepare()\n");

	/*绑定变量x、id、name的值*/
	p_x = 102;
	if (!NstvException::checkErr(p_err, OCIBindByName(p_stmt1, &p_bnd1, p_err, (text *)":x", -1, 
		(dvoid *)&p_x, sizeof(int), SQLT_INT, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)0, (ub4 *)0, OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci绑定语句占位符1: OCIBindByName()\n");

	p_id = 1;
	if (!NstvException::checkErr(p_err, OCIBindByName(p_stmt2, &p_bnd2, p_err, (text *)":id", -1, 
			(dvoid *)&p_id, sizeof(int), SQLT_INT, (dvoid *)0, (ub2 *)0, (ub2 *)0, (ub4)0, (ub4 *)0, OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci绑定语句占位符2: OCIBindByName()\n");

	if (!NstvException::checkErr(p_err, OCIBindByName(p_stmt2, &p_bnd3, p_err, (text *)":name", strlen(":name"), 
			(dvoid *)p_name, strlen(p_name)+1, SQLT_STR, (dvoid *)0,           
			(ub2 *)0, (ub2 *)0, (ub4)0, (ub4 *)0, OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci绑定语句占位符3: OCIBindByName()\n");	
	
	/*准备语句执行结果缓冲区*/
	if (!NstvException::checkErr(p_err, OCIDefineByPos(p_stmt1, &p_dfn1, p_err, 1, (dvoid *)res_id, sizeof(int), SQLT_INT, 
			(dvoid *)0, (ub2 *)0, (ub2 *)0, OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci定义结果缓冲区1: OCIDefineByPos()\n");
	if (!NstvException::checkErr(p_err, OCIDefineByPos(p_stmt1, &p_dfn2, p_err, 2, (dvoid *)res_name, (sword)20, SQLT_STR, 
			(dvoid *)0, (ub2 *)0, (ub2 *)0, OCI_DEFAULT)))
		throw NstvException();
	WUJDEBUG("oci定义结果缓冲区2: OCIDefineByPos()\n");
	
	/*获得SQL语句的类型, 判断会话执行的SQL语句是什么类型的 OCI_STMT_XXX, 如 OCI_STMT_SELECT、OCI_STMT_UPDATE、OCI_STMT_DROP等*/
	if (!NstvException::checkErr(p_err, OCIAttrGet ((dvoid *)p_stmt1, (ub4)OCI_HTYPE_STMT, (dvoid *)&stmt_type1, 
		(ub4 *)0, (ub4)OCI_ATTR_STMT_TYPE, p_err)))
		throw NstvException();
	WUJDEBUG("oci获得SQL语句的类型1: OCIAttrGet() 类型为: p_stmt1=%d\n", stmt_type1);

	if (!NstvException::checkErr(p_err, OCIAttrGet ((dvoid *)p_stmt2, (ub4)OCI_HTYPE_STMT, (dvoid *)&stmt_type2, 
		(ub4 *)0, (ub4)OCI_ATTR_STMT_TYPE, p_err)))
		throw NstvException();
	WUJDEBUG("oci获得SQL语句的类型2: OCIAttrGet() 类型为: p_stmt2=%d\n", stmt_type2);
	
	if (!NstvException::checkErr(p_err, OCIAttrGet ((dvoid *)p_stmt3, (ub4)OCI_HTYPE_STMT, (dvoid *)&stmt_type3, 
		(ub4 *)0, (ub4)OCI_ATTR_STMT_TYPE, p_err)))
		throw NstvException();
	WUJDEBUG("oci获得SQL语句的类型3: OCIAttrGet() 类型为: p_stmt3=%d\n", stmt_type3);
	
	/*执行SQL语句*/
#if 1

	rc = OCIHandleAlloc((dvoid *)p_env, (dvoid **)&p_trans, OCI_HTYPE_TRANS, 0, 0);
	WUJDEBUG("oci创建事务句柄: OCIHandleAlloc()rc=%d\n", rc);
	if(rc != 0){
		printOCIErr(p_err, "OCIHandleAlloc", rc);
		goto OCIErr;
	}
	if (!NstvException::checkErr(p_err, OCIAttrSet((dvoid *)p_svc, OCI_HTYPE_SVCCTX, (dvoid *)p_trans, 0, OCI_ATTR_TRANS, p_err)))
		throw NstvException();
	WUJDEBUG("oci设置事务句柄属性: OCIHandleAlloc()\n");

	if (!NstvException::checkErr(p_err, OCITransStart (p_svc, p_err, 3, OCI_TRANS_NEW))){
		throw NstvException();
	}
    else if(stmt_type2 == OCI_STMT_INSERT){
		WUJDEBUG("oci开启一个事务: OCITransStart()\n");
		WUJDEBUG("oci执行SQL语句2: OCIStmtExecute()\n");
		if (!NstvException::checkErr(p_err, OCIStmtExecute(p_svc, p_stmt2, p_err, 
			(ub4)1, (ub4)0, (CONST OCISnapshot *)NULL, (OCISnapshot *)NULL, OCI_DEFAULT))){
			NstvException::checkErr(p_err, OCITransRollback(p_svc, p_err, (ub4) 0));
			throw NstvException();
		}else{//如果没有发生异常，则else中将事务提交
			WUJDEBUG("oci提交一个事务: OCITransStart()\n");
			NstvException::checkErr(p_err, OCITransCommit(p_svc, p_err, (ub4) 0));
		}
	}
	
#endif
#if 0
	if(stmt_type3 == OCI_STMT_DELETE){
		if (!NstvException::checkErr(p_err, OCIStmtExecute(p_svc, p_stmt3, p_err, (ub4)1, (ub4)0, 
			(CONST OCISnapshot *)NULL, (OCISnapshot *)NULL, OCI_DEFAULT)))
			throw NstvException();
		WUJDEBUG("oci执行SQL语句3: OCIStmtExecute()\n");
	}
#endif	
	if(stmt_type1 == OCI_STMT_SELECT){
		if (!NstvException::checkErr(p_err, OCIStmtExecute(p_svc, p_stmt1, p_err, (ub4)1, (ub4)0, 
			(CONST OCISnapshot *)NULL, (OCISnapshot *)NULL, OCI_DEFAULT)))
			throw NstvException();
		WUJDEBUG("oci执行SQL语句1: OCIStmtExecute()\n");
	}
	
	
	/*打印结果*/
	while (rc != OCI_NO_DATA)
	{         
		WUJDEBUG("%d, %s\n", *res_id, res_name);
		rc = OCIStmtFetch2(p_stmt1, p_err, 1, OCI_FETCH_NEXT, 1, OCI_DEFAULT);
		if(rc != 100 && rc !=0)
		{
			printOCIErr(p_err, "OCIStmtFetch2", rc);
		}
	}
	rc = 0;
	
	/*断开连接*/

	if (!NstvException::checkErr(p_err, OCISessionEnd(p_svc, p_err, p_session, (ub4)OCI_DEFAULT)))
		throw NstvException();
	printf("oci结束会话: CILogoff()\n");
	
	if (!NstvException::checkErr(p_err, OCIServerDetach(p_server, p_err, OCI_DEFAULT)))
		throw NstvException();
	printf("oci删除对数据源的访问: CILogoff()\n");
	
OCIErr:
	if(p_env != NULL){
		rc = OCIHandleFree((dvoid *)p_env, OCI_HTYPE_ENV);
		p_env = NULL;
	}
	if(p_server != NULL){
		rc = OCIHandleFree((dvoid *)p_server, OCI_HTYPE_SERVER);
		p_server = NULL;
	}
	if(p_err != NULL){
		rc = OCIHandleFree((dvoid *)p_err, OCI_HTYPE_ERROR);
		p_err = NULL;
	}
	if(p_session != NULL){
		rc = OCIHandleFree((dvoid *)p_session, OCI_HTYPE_SESSION);
		p_session = NULL;
	}
	if(p_svc != NULL){
		rc = OCIHandleFree((dvoid *)p_svc, OCI_HTYPE_SVCCTX);
		p_svc = NULL;
	}
	if(p_trans != NULL){
		rc = OCIHandleFree((dvoid *)p_trans, OCI_HTYPE_TRANS);
		p_trans = NULL;
	}
	if(p_stmt1 != NULL){
		rc = OCIHandleFree((dvoid *)p_stmt1, OCI_HTYPE_STMT);
		p_stmt1 = NULL;
	}
	if(p_stmt2 != NULL){
		rc = OCIHandleFree((dvoid *)p_stmt2, OCI_HTYPE_STMT);
		p_stmt2 = NULL;
	}
	if(p_stmt3 != NULL){
		rc = OCIHandleFree((dvoid *)p_stmt3, OCI_HTYPE_STMT);
		p_stmt3 = NULL;
	}
	if(p_dfn1 != NULL){
		rc = OCIHandleFree((dvoid *)p_dfn1, OCI_HTYPE_DEFINE);
		p_dfn1 = NULL;
	}
	if(p_dfn2 != NULL){
		rc = OCIHandleFree((dvoid *)p_dfn2, OCI_HTYPE_DEFINE); 
		p_dfn2 = NULL;
	}
	if(p_bnd1 != NULL){
		rc = OCIHandleFree((dvoid *)p_bnd1, OCI_HTYPE_BIND); 
		p_bnd1 = NULL;
	}
	if(p_bnd2 != NULL){
		rc = OCIHandleFree((dvoid *)p_bnd2, OCI_HTYPE_BIND); 
		p_bnd2 = NULL;
	}
	if(p_bnd3 != NULL){
		rc = OCIHandleFree((dvoid *)p_bnd3, OCI_HTYPE_BIND); 
		p_bnd3 = NULL;
	}
	
	free(res_id);
	res_id = NULL;
	free(res_name);
	res_name = NULL;
	
	return 0;
}

