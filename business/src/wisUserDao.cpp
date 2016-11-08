#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>



#include "wisUserDao.h"
#include "libSql.h"
#include "CNVDataAccess.h"
#include "WisBindDao.h"

std::string WisUserDao::defaultPassword = "R800JL.Ke8MBo";

bool WisUserDao::checkUser(const std::string &user)
{
	TRACE_IN();
	if(user.empty()) return false;
	
	CNVDataAccess *access = (CNVDataAccess *)DbaModule_GetNVDataAccess();
	if(NULL == access)
	{
		return false;
	}
	
	char sql[200] = {0};
	snprintf(sql,sizeof(sql),"select * from wis_user_tbl where `name`='%s'",user.c_str());
	if(0 != access->ExecuteNoThrow(sql))
	{
		LOG_INFO("excute sql(%s) query failed. ",sql);
		DbaModule_ReleaseNVDataAccess(access);
		return false;
	}
	if(access->RowsAffected() >0)
	{	
		DbaModule_ReleaseNVDataAccess(access);
		return true;
	}
	DbaModule_ReleaseNVDataAccess(access);
	TRACE_OUT();
	return false;	
}

bool WisUserDao::checkUserAndPassword(const std::string &user,const std::string &password){
	TRACE_IN();
	if(user.empty()) return false;
	
	CNVDataAccess *access = (CNVDataAccess *)DbaModule_GetNVDataAccess();
	if(NULL == access)
	{
		return false;
	}
	
	char sql[200] = {0};
	snprintf(sql,sizeof(sql),"select * from wis_user_tbl where `name`='%s' and `password`='%s'",user.c_str(),password.c_str());
	if(0 != access->ExecuteNoThrow(sql))
	{
		LOG_INFO("excute sql(%s) query failed. ",sql);
		DbaModule_ReleaseNVDataAccess(access);
		return false;
	}
	if(access->RowsAffected() >0)
	{	
		DbaModule_ReleaseNVDataAccess(access);
		return true;
	}
	DbaModule_ReleaseNVDataAccess(access);
	TRACE_OUT();
	return false;	
}

bool WisUserDao::login(const std::string &user,const std::string& passwd)
{
	TRACE_IN();
	if(user.empty()) return false;
	
	CNVDataAccess *access = (CNVDataAccess *)DbaModule_GetNVDataAccess();
	if(NULL == access)
	{
		LOG_ERROR("get database access failed");
		return false;
	}

	
	char sql[200] = {0};
	snprintf(sql,sizeof(sql),"select * from wis_user_tbl where `name`='%s' and `password`='%s'",user.c_str(),passwd.c_str());
	if(access->ExecuteNoThrow(sql) < 1)
	{
		LOG_ERROR("excute sql(%s) query failed. ",sql);
		return false;
	}
	if(access->IsResultSet())
	{
		access->FreeResult();
	}
		
	snprintf(sql,sizeof(sql),"update wis_user_tbl set `login_time`=CURRENT_TIMESTAMP,\
	`login_cnt`=`login_cnt`+1,`status`=1 where `name`='%s'",user.c_str());
	
	if(-1 == access->ExecuteNonQuery(sql))
	{
		LOG_ERROR("excute sql(%s) query failed. ",sql);
		DbaModule_ReleaseNVDataAccess(access);
		return false;
	}

	DbaModule_ReleaseNVDataAccess(access);
	TRACE_OUT();
	return true;
}


bool WisUserDao::logout(const std::string &user,const std::string &passwd)
{
	if(user.empty()) return false;
	
	CNVDataAccess *access = (CNVDataAccess *)DbaModule_GetNVDataAccess();
	if(NULL == access)
	{
		return false;
	}
	
	char sql[200] = {0};
	snprintf(sql,sizeof(sql),"update wis_user_tbl set `status`=0 where `name`='%s'",user.c_str());

	if(-1 == access->ExecuteNonQuery(sql))
	{
		DbaModule_ReleaseNVDataAccess(access);
		return false;
	}

	DbaModule_ReleaseNVDataAccess(access);
	TRACE_OUT();
	return false;
}

bool WisUserDao::regist(const std::string &user,const std::string &passwd)
{
	TRACE_IN();
	if(user.empty() || passwd.empty()) return false;
	
	CNVDataAccess *access = (CNVDataAccess *)DbaModule_GetNVDataAccess();
	if(NULL == access)
	{
		LOG_ERROR("get database access failed");
		return false;
	}
	
	char sql[200] = {0};
	snprintf(sql,sizeof(sql),"insert into wis_user_tbl(`name`,`password`,`type`,`permission`,\
	`reg_time`,`login_cnt`,`bind_status`) values('%s','%s', 0, 0, CURRENT_TIMESTAMP,0,0)",user.c_str(), passwd.c_str());
	if(-1 == access->ExecuteNonQuery(sql))
	{
		DbaModule_ReleaseNVDataAccess(access);
		return false;
	}

	DbaModule_ReleaseNVDataAccess(access);
	TRACE_OUT();
	return true;
	
}

bool WisUserDao::logoutAll()
{
	CNVDataAccess *access = (CNVDataAccess *)DbaModule_GetNVDataAccess();
	if(NULL == access)
	{
		LOG_ERROR("get database access failed");
		return false;
	}
	const char *sql = "update wis_user_tbl set `status`=0";
	if(-1 == access->ExecuteNonQuery(sql))
	{
		LOG_INFO("excute sql(%s)  failed",sql);
		DbaModule_ReleaseNVDataAccess(access);
		return false;
	}
	DbaModule_ReleaseNVDataAccess(access);
	return true;
	
}

bool WisUserDao::userGetDevice(const char *uuid)
{
	TRACE_IN();
	std::map<std::string,WisDeviceInfo> mapDevice;
	int count = WisBindDao::getBindedDevices(std::string(uuid),mapDevice);
	int size = sizeof(int) + count*sizeof(WisDeviceInfo);

	WisUserDeviceList *deviceList = (WisUserDeviceList*)malloc(size);

	LOG_INFO("use(uuid = %s) get %d device and server will send response(size = %d)",uuid,count,size);
	if(deviceList){
		deviceList->nums = count;
		int index = 0;
		std::map<std::string,WisDeviceInfo>::iterator iter = mapDevice.begin();
		for(;iter != mapDevice.end();iter++,index++){
			memcpy((char *)deviceList + sizeof(int) + index*sizeof(WisDeviceInfo),&(iter->second),sizeof(WisDeviceInfo));
			}
		}

	GetUniteDataModuleInstance()->SendData(uuid,WIS_CMD_USER_GET_DEVICES,(char *)deviceList,size,TCP_SERVER_MODE);
	SafeDeleteArray(deviceList);
	TRACE_OUT();
}

void WisUserDao::handleUserRegist(BUS_ADDRESS & busAddress,const char * pdata)
{
	TRACE_IN();
	if(NULL == pdata){
		LOG_ERROR("illegal parameter");
		return;
	}	
	
	WisUserRegistInfo *registInfo = (WisUserRegistInfo*)pdata;
	
	if(!checkMail(string(registInfo->uuid,UUID_LEN))){
		LOG_ERROR("%s attempt to regist with a illegal mail address",string(registInfo->uuid,UUID_LEN).c_str());
		sendUserResponse(busAddress,WIS_CMD_USER_REGIST,-3);
		return;
	}
	
    if(checkUser(string(registInfo->uuid,UUID_LEN))){
		LOG_INFO("%s is already registed",string(registInfo->uuid,UUID_LEN).c_str());
		sendUserResponse(busAddress,WIS_CMD_USER_REGIST,-2);
		return;
    }
		
	char sql[200] = {0};
	snprintf(sql,sizeof(sql),"insert into wis_user_tbl(`name`,`password`,`type`,`permission`,\
	`reg_time`,`login_cnt`,`bind_status`) values('%s','%s', 0, 0, CURRENT_TIMESTAMP,0,0)",std::string(registInfo->uuid,UUID_LEN).c_str(), std::string(registInfo->password,PASSWORD_LEN).c_str());
		
	CNVDataAccess *access = (CNVDataAccess *)DbaModule_GetNVDataAccess();
	if(NULL == access)
	{
		LOG_ERROR("get database access failed");
		sendUserResponse(busAddress,WIS_CMD_USER_REGIST,-1);
		return;
	}
    if(-1 == access->ExecuteNonQuery(sql))
	{
		DbaModule_ReleaseNVDataAccess(access);
		sendUserResponse(busAddress,WIS_CMD_USER_REGIST,-1);
		return ;
	}
    sendUserResponse(busAddress,WIS_CMD_USER_REGIST,0);
	DbaModule_ReleaseNVDataAccess(access);
	TRACE_OUT();
}

void WisUserDao::handleUserModifyPassword(std::string &uuid,BUS_ADDRESS &busAddress,const char *pdata)
{
	TRACE_IN();
	if(NULL == pdata){
		LOG_ERROR("illegal parameter");
		return;
	}
	WisUserModifyPassword *modifyInfo  = (WisUserModifyPassword*)pdata;

    CNVDataAccess *access = (CNVDataAccess *)DbaModule_GetNVDataAccess();
	if(NULL == access)
	{
		LOG_ERROR("get database access failed");
		sendUserResponse(busAddress,WIS_CMD_USER_MODIFY_PASSWORD,-1);
		return;
	}
	char sql[200] = {0};
	snprintf(sql,sizeof(sql),"update wis_user_tbl set `password`='%s' where `name`='%s'",string(modifyInfo->newPassword).c_str(),uuid.c_str());
	if(-1 == access->ExecuteNonQuery(sql))
	{
		LOG_INFO("excute sql(%s)  failed",sql);
		sendUserResponse(busAddress,WIS_CMD_USER_MODIFY_PASSWORD,-1);
		DbaModule_ReleaseNVDataAccess(access);
		return ;
	}	
	sendUserResponse(busAddress,WIS_CMD_USER_MODIFY_PASSWORD,0);
	DbaModule_ReleaseNVDataAccess(access);
}

void WisUserDao::handleUserResetPassword(BUS_ADDRESS &busAddress,const char *pdata)
{
	TRACE_IN();
	if(NULL == pdata){
		LOG_ERROR("illegal parameter");
		return;
	}

	WisUserResetPassword*resetInfo = (WisUserResetPassword*)pdata;	
    if(!checkUser(string(resetInfo->uuid,UUID_LEN))){
		LOG_INFO("user(uuid = %s) want to reset his password,but he is not registed yet",string(resetInfo->uuid,UUID_LEN).c_str());
		sendUserResponse(busAddress,WIS_CMD_USER_RESET_PASSWORD,-2);
		return;
    }	
	if(sendResetPasswordMailTo(string(resetInfo->uuid,UUID_LEN)))
		sendUserResponse(busAddress,WIS_CMD_USER_RESET_PASSWORD,0);
	else
		sendUserResponse(busAddress,WIS_CMD_USER_RESET_PASSWORD,-1);
}

string WisUserDao::makeupURL(const std::string& uuid)
{
	return "wisHome password manager";
}

bool WisUserDao::sendUserResponse(BUS_ADDRESS &busAddress,int cmd,const int ret)
{
	return GetUniteDataModuleInstance()->SendData(busAddress,cmd,(char *)&ret,sizeof(int),TCP_SERVER_MODE);
}


bool WisUserDao::sendResetPasswordMailTo(const std::string & uuid)
{
	string url = makeupURL(string(uuid,UUID_LEN));
	string content = string("dear sir:<br/> &nbsp &nbsp &nbsp &nbsp &nbsp  please click this link <a href=\"http://192.168.2.70/test.php\">") + url + string(" </a>to reset your password"); 	

	Mail mail;
	std::set<std::string> to,cc,bcc,file;
	to.insert(uuid);
	mail.setTargetAndFile(to,cc,bcc,file);
	if(!mail.sendMail(content))
		return false;
	else
		return true;
}

bool WisUserDao::sendGreetMailTo(const std::string & uuid)
{
	string content = string("Dear sir:<br/> &nbsp &nbsp &nbsp &nbsp &nbsp  thanks for register wishome. ");	
    std::set<std::string> to,cc,bcc,file;
	to.insert(uuid);
	Mail mail;
    mail.setTargetAndFile(to,cc,bcc,file);
    if(!mail.sendMail(content))
		return false;
	else
		return true;	
}

bool WisUserDao::checkMail(const std::string &mail)
{
	int n = mail.find('@',0);
	if(n == string::npos)
		return false;
	std::string _smtpServer = "smtp." + mail.substr(n + 1,mail.length());

	hostent *ph = gethostbyname(_smtpServer.c_str());
    if( ph == NULL ) {
        return false;
    }
    
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(25);    //port of SMTP 
    memcpy(&sin.sin_addr, (in_addr*)ph->h_addr_list[0], ph->h_length);
   
	unsigned long ul = 1;  
	int s = socket(PF_INET, SOCK_STREAM, 0);
	//fcntl(s, F_SETFL, O_NONBLOCK);
    if(connect(s, (sockaddr*)&sin, sizeof(sin)) ) 
       return false;
	close(s);
	//SafeFree(ph);
   	return true;
}





