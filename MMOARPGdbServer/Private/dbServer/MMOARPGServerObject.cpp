#include "MMOARPGServerObject.h"
#include "SimpleMySQLibrary.h"
#include "MysqlConfig.h"
#include "Log\MMOARPGdbServerLog.h"
#include "Protocol/LoginProtocol.h"
#include "MMOARPGType.h"
#include "Global/SimpleNetGlobalInfo.h"
#include "../../SimpleHTTP/Source/SimpleHTTP/Public/SimpleHTTPManage.h"
#include "Protocol/HallProtocol.h"

void UMMOARPGServerObejct::Init()
{
	Super::Init();

	// 创建"读" 的数据库对象
	MysqlObjectRead = USimpleMySQLLibrary::CreateMysqlObject(nullptr,
		FSimpleMysqlConfig::Get()->GetInfo().User,
		FSimpleMysqlConfig::Get()->GetInfo().Host,
		FSimpleMysqlConfig::Get()->GetInfo().Pwd,
		FSimpleMysqlConfig::Get()->GetInfo().DB,
		FSimpleMysqlConfig::Get()->GetInfo().Port,
		FSimpleMysqlConfig::Get()->GetInfo().ClientFlags
	);

	// 创建"写" 的数据库对象.
	MysqlObjectWrite = USimpleMySQLLibrary::CreateMysqlObject(nullptr,
		FSimpleMysqlConfig::Get()->GetInfo().User,
		FSimpleMysqlConfig::Get()->GetInfo().Host,
		FSimpleMysqlConfig::Get()->GetInfo().Pwd,
		FSimpleMysqlConfig::Get()->GetInfo().DB,
		FSimpleMysqlConfig::Get()->GetInfo().Port,
		FSimpleMysqlConfig::Get()->GetInfo().ClientFlags
	);

	//仅测试代码.
//  	FString SQL = "SELECT * FROM wp_users WHERE ID = 1";
//  	TArray<FSimpleMysqlResult> Results;
//  	Get(SQL, Results);// 使用此语句查东西然后把结果存出来.
//  	for (auto& Tmp : Results) {
//  	
//  	}

}

void UMMOARPGServerObejct::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void UMMOARPGServerObejct::Close()
{
	Super::Close();

	// 服务器关闭的时候执行安全清除 数据库对象.
	if (MysqlObjectWrite != nullptr) {
		MysqlObjectWrite->ConditionalBeginDestroy();
		MysqlObjectWrite = nullptr;
	}
	if (MysqlObjectRead != nullptr) {
		MysqlObjectRead->ConditionalBeginDestroy();
		MysqlObjectRead = nullptr;
	}
}

void UMMOARPGServerObejct::RecvProtocol(uint32 InProtocol)
{
	Super::RecvProtocol(InProtocol);

	// 根据收到的来自客户端的协议号执行接收.
	switch (InProtocol) {
		case SP_LoginRequests:
		{
			FString AccountString;
			FString PasswordString;
			FSimpleAddrInfo AddrInfo;

			FString String = TEXT("[]");// 仅用作占位符.

			SIMPLE_PROTOCOLS_RECEIVE(SP_LoginRequests, AccountString, PasswordString, AddrInfo);

			// 设定一条在数据库里查找语句.
			FString SQL = FString::Printf(
				TEXT("SELECT ID,user_pass FROM wp_users WHERE user_login ='%s' or user_email='%s';"), *AccountString, *AccountString
			);

			TArray<FSimpleMysqlResult> Result;
			if (Get(SQL, Result)) { /* 从dbServer上拉数据.*/
				if (Result.Num() > 0) {
					for (auto& Tmp : Result) {
						int32 UserID = 0;
						if (FString* IDString = Tmp.Rows.Find(TEXT("ID"))) {// 查找字段为"ID"的那个字符串.
							UserID = FCString::Atoi(**IDString);
						}
						/// 用户加密密码以及Post操作.
						if (FString* UserPass = Tmp.Rows.Find(TEXT("user_pass"))) {// 查找字段为"user_pass"的那个字符串.
							// 此句来自网络上借鉴的php插件.
							// http://127.0.0.1/wp/wp-content/plugins/SimplePasswordVerification/SimplePasswordVerification.php?EncryptedPassword=$P$BJT4j.n/npkiQ8Kp.osbJX9xA7to5U/&Password=1232123&IP=12345&Port=5677&Channel=SAFASDASFWEQDFSDASDADWFASDASDQW&UserID=1
							FString Param = FString::Printf(
								TEXT("EncryptedPassword=%s&Password=%s&IP=%i&Port=%i&Channel=%s&UserID=%i")
								, **UserPass// 加密密码
								, *PasswordString// 实际输入的密码.
								, AddrInfo.Addr.IP// 登录服务器里的IP.
								, AddrInfo.Addr.Port
								, *AddrInfo.ChannelID.ToString()
								, UserID
							);

							/** Post 操作.*/
							FSimpleHTTPResponseDelegate Delegate;
							Delegate.SimpleCompleteDelegate.BindUObject(this, &UMMOARPGServerObejct::Callback_CheckPasswordResult);
							// 借助全局配置表. 获取公网IP.即载有db的那台机器wordpress的 192.168.2.30
							FString WpIP = FSimpleNetGlobalInfo::Get()->GetInfo().PublicIP;
							//
							SIMPLE_HTTP.PostRequest(
								*FString::Printf(TEXT("http://%s/wp/wp-content/plugins/SimplePasswordVerification/SimplePasswordVerification.php"), *WpIP)
								, *Param
								, Delegate
							);
						}
					}
				}
				else {
					ELoginType Type = ELoginType::LOGIN_ACCOUNT_WRONG;
					SIMPLE_PROTOCOLS_SEND(SP_LoginResponses, AddrInfo, Type, String);
				}
			}
			else {/* dbSevrer不存在.*/
				ELoginType Type = ELoginType::LOGIN_DB_SERVER_ERROR;
				SIMPLE_PROTOCOLS_SEND(SP_LoginResponses, AddrInfo, Type, String);
			}

			UE_LOG(LogMMOARPGdbServer, Display, TEXT("AccountString = %s, PasswordString = %s"), *AccountString, *PasswordString);
			break;
		}

		case SP_CharacterAppearanceRequests:// 收到协议: 来自网关转发的客户端玩家形象协议.
		{
			// 接收到来自网关转发的客户端玩家形象 Request协议.
			int32 UserID = INDEX_NONE;
			FSimpleAddrInfo AddrInfo;// 需要那个接收数据源的网关服务器地址.
			SIMPLE_PROTOCOLS_RECEIVE(SP_CharacterAppearanceRequests, UserID, AddrInfo);
			UE_LOG(LogMMOARPGdbServer, Display, TEXT("[SP_CharacterAppearanceResponses], db 收到了捏脸玩家形象请求."));

			if (UserID > 0.0f) {// ID在数据库里正常大于0.
				// 关联玩家形象的数据库数据,目前先写死,作假.
				FCharacterAppearances CharacterAppearances;
				CharacterAppearances.Add(FMMOARPGCharacterAppearance());
				FMMOARPGCharacterAppearance& InLastAppear = CharacterAppearances.Last();
				InLastAppear.Lv = 14;
				InLastAppear.Date = FDateTime::Now().ToString();// 真实世界的时间点.
				InLastAppear.Name = TEXT("之钠波");
				InLastAppear.SlotPosition = 1;
				

				// 把数据源压缩成JSON
				FString JsonString;
				NetDataAnalysis::CharacterAppearancesToString(CharacterAppearances, JsonString);

				// 发回去
				SIMPLE_PROTOCOLS_SEND(SP_CharacterAppearanceResponses, AddrInfo, JsonString);
				UE_LOG(LogMMOARPGdbServer, Display, TEXT("[SP_CharacterAppearanceResponses], dbServer已发送Response!!!!!"));
			}
			break;
		}
	}
}

bool UMMOARPGServerObejct::Post(const FString& InSQL)
{
	// 只需要负责把SQL数据  写入 服务器.
	if (!InSQL.IsEmpty()) {
		if (MysqlObjectWrite != nullptr) {
			// 利用 SQL对象查找错误消息
			FString ErrMsg;
			USimpleMySQLLibrary::QueryLink(MysqlObjectWrite, InSQL, ErrMsg);
			if (ErrMsg.IsEmpty()) {
				return true;
			}
			else {
				// 打印出错误消息.
				UE_LOG(LogMMOARPGdbServer, Error, TEXT("MMOARPGdbServer Error : Post msg [ %s]"), *ErrMsg);
			}
		}
	}
	return false;
}

bool UMMOARPGServerObejct::Get(const FString& InSQL, TArray<FSimpleMysqlResult>& Results)
{
	// 只需要负责把SQL数据  写入 服务器.
	if (!InSQL.IsEmpty()) {
		if (MysqlObjectRead != nullptr) {
			// 调试信息、错误消息之类.
			FSimpleMysqlDebugResult Debug;
			Debug.bPrintToLog = true;
			FString ErrMsg;

			USimpleMySQLLibrary::QueryLinkResult(MysqlObjectRead,
				InSQL,
				Results,
				ErrMsg,
				EMysqlQuerySaveType::STORE_RESULT,// 先下后查模式.
				Debug
			);
			if (ErrMsg.IsEmpty()) {
				return true;
			}
			else {
				// 打印出错误消息.
				UE_LOG(LogMMOARPGdbServer, Error, TEXT("MMOARPGdbServer Error : Get msg [ %s]"), *ErrMsg);
			}
		}
	}
	return false;
}

void UMMOARPGServerObejct::Callback_CheckPasswordResult(const FSimpleHttpRequest& InRequest, const FSimpleHttpResponse& InResponse, bool bLinkSuccessful)
{
	/* 开始解析数据*/
	if (bLinkSuccessful == true) {
		// 开始解析类似于 xx&ip&port&0 这种数据.
		TArray<FString> Values;
		InResponse.ResponseMessage.ParseIntoArray(Values, TEXT("&"));

		FSimpleAddrInfo AddrInfo;// 当前网关地址.
		uint32 UserID = 0;// 数据库里的真正用户ID.
		EPasswordVerification PV = EPasswordVerification::VERIFICATION_FAIL;// 数据库里密码验证的PV.
		if (Values.Num()) {
			if (Values.IsValidIndex(0)) {// 0号是用户ID.
				UserID = FCString::Atoi(*Values[0]);
			}
			if (Values.IsValidIndex(1)) {// 1号是地址ip.
				AddrInfo.Addr.IP = FCString::Atoi(*Values[1]);
			}
			if (Values.IsValidIndex(2)) {// 2号是端口.
				AddrInfo.Addr.Port = FCString::Atoi(*Values[2]);
			}
			if (Values.IsValidIndex(3)) {// 3号是Guid.
				FGuid::ParseExact(Values[3], EGuidFormats::Digits, AddrInfo.ChannelID);
			}
			if (Values.IsValidIndex(4)) {
				PV = (EPasswordVerification)FCString::Atoi(*Values[4]);
			}

			FString String = TEXT("[]");// 仅用作占位符.

			/* 追加判断PV成功.*/
			if (PV == VERIFICATION_SUCCESS) {
				if (UserID != 0) {// 不为0才有意义.
					// 设定一条检索的SQL语句.
					FMMOARPGUserData UserData;// 构建一个用户数据.
					UserData.ID = UserID;// 填值.
					FString SQL = FString::Printf(TEXT("SELECT user_login,user_email,user_url,display_name FROM wp_users WHERE ID=%i;"), UserID);// SQL查找语句.

					TArray<FSimpleMysqlResult> Result;
					if (Get(SQL, Result)) { /* 从dbServer上拉数据.*/
						if (Result.Num() > 0) {

							for (auto& Tmp : Result) {
								//
								if (FString* InUserLogin = Tmp.Rows.Find(TEXT("user_login"))) {
									UserData.Account = *InUserLogin;
								}
								//
								if (FString* InUserEmail = Tmp.Rows.Find(TEXT("user_email"))) {
									UserData.EMail = *InUserEmail;
								}
								// 								//
								// 								if (FString* InUserUrl = Tmp.Rows.Find(TEXT("user_url"))) {
								// 
								// 								}
																//
								if (FString* InDisplayName = Tmp.Rows.Find(TEXT("display_name"))) {
									UserData.Name = *InDisplayName;
								}
							}
						}
					}

					// 把有值的UserData 写入并暂存序列化为Json的格式.
					NetDataAnalysis::UserDataToString(UserData, String);

					// 回复给客户端,通知解析成功.
					ELoginType Type = ELoginType::LOGIN_SUCCESS;
					SIMPLE_PROTOCOLS_SEND(SP_LoginResponses, AddrInfo, Type, String);
				}
			}
			/* 追加判断PV失败*/
			else {
				// 发送给客户端,说明是密码输错了.
				ELoginType Type = ELoginType::LOGIN_PASSWORD_WRONG;
				SIMPLE_PROTOCOLS_SEND(SP_LoginResponses, AddrInfo, Type, String);
			}
		}
	}
}
