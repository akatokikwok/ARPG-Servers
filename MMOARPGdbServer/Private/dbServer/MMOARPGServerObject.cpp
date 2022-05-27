#include "MMOARPGServerObject.h"
#include "SimpleMySQLibrary.h"
#include "MysqlConfig.h"
#include "Log\MMOARPGdbServerLog.h"
#include "Protocol/LoginProtocol.h"
#include "Global/SimpleNetGlobalInfo.h"
#include "../../SimpleHTTP/Source/SimpleHTTP/Public/SimpleHTTPManage.h"
#include "Protocol/HallProtocol.h"
#include <Protocol/ServerProtocol.h>

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

	// 初始化一下 SQL语句: 它负责创建一张表`mmoarpg_characters_c.
	FString Create_mmoarpg_character_ca_SQL = TEXT("CREATE TABLE IF NOT EXISTS `mmoarpg_characters_ca`(\
		`id` INT UNSIGNED AUTO_INCREMENT,\
		`mmoarpg_name` VARCHAR(100) NOT NULL,\
		`mmoarpg_date` VARCHAR(100) NOT NULL,\
		`mmoarpg_slot` INT,\
		`Leg_Size` double(11,4) DEFAULT '0.00',\
		`Waist_Size` double(11,4) DEFAULT '0.00',\
		`Arm_Size` double(11,4) DEFAULT '0.00',\
		PRIMARY KEY(`id`)\
		) ENGINE = INNODB DEFAULT CHARSET = utf8mb4; ");
	if (!Post(Create_mmoarpg_character_ca_SQL)) {
		UE_LOG(LogMMOARPGdbServer, Error, TEXT("we create table mmoarpg_characters_ca failed."));// 如果Post失败就打印提示 创表失败.
	}

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

		/** 舞台人物造型请求. */
		case SP_CharacterAppearanceRequests:// 收到协议: 来自网关转发的客户端玩家形象协议.
		{
			// 接收到来自网关转发的客户端玩家形象 Request协议.
			int32 UserID = INDEX_NONE;
			FSimpleAddrInfo AddrInfo;// 需要那个接收数据源的网关服务器地址.
			SIMPLE_PROTOCOLS_RECEIVE(SP_CharacterAppearanceRequests, UserID, AddrInfo);

			/**  */
			if (UserID > 0.0f) {
				FCharacterAppearances CharacterAppearances;// CA存档集.
				FString IDs;// 待加工的角色ID集, 表现出如类似1,2,3,4的形式.

				/// 拿到元数据.
				{
					FString SQL = FString::Printf(TEXT("SELECT meta_value from wp_usermeta where user_id=%i and meta_key=\"character_ca\";"), UserID);// 查找语句.
					TArray<FSimpleMysqlResult> Result;
					if (Get(SQL, Result)) {
						if (Result.Num() > 0) {
							for (auto& Tmp_row : Result) {// 单行.
								if (FString* InMetaValue = Tmp_row.Rows.Find(TEXT("meta_value"))) {// 找到表wp_usermeta里指定ID的 元数据
									TArray<FString> Arrays_temp;
									InMetaValue->ParseIntoArray(Arrays_temp, TEXT("|"));// 拆解携带|号的字符串
									for (auto& TmpID : Arrays_temp) {
										IDs += TmpID + TEXT(",");// 拼接成新的字符串.
									}
									IDs.RemoveFromEnd(TEXT(","));
								}
							}
						}
					}
				}

				/// 拿到我们的角色数据.
				if (!IDs.IsEmpty()) {
					// 从表mmoarpg_characters_ca查找id属于IDs的所有行.
					FString SQL = FString::Printf(TEXT("SELECT * from mmoarpg_characters_ca where id in (%s);"), *IDs);
					// 
					TArray<FSimpleMysqlResult> Result;
					if (Get(SQL, Result)) {
						if (Result.Num() > 0) {
							for (auto& Tmp : Result) {// 每一行.

								/* 对 新增存档各属性部分执行设置. */
								CharacterAppearances.Add(FMMOARPGCharacterAppearance());
								FMMOARPGCharacterAppearance& InLast = CharacterAppearances.Last();
								if (FString* InName = Tmp.Rows.Find(TEXT("mmoarpg_name"))) {
									InLast.Name = *InName;
								}
								if (FString* InDate = Tmp.Rows.Find(TEXT("mmoarpg_date"))) {
									InLast.Date = *InDate;
								}
								if (FString* InSlot = Tmp.Rows.Find(TEXT("mmoarpg_slot"))) {
									InLast.SlotPosition = FCString::Atoi(**InSlot);
								}

								/* 舞台人物身材外观.*/
								if (FString* InLegSize = Tmp.Rows.Find(TEXT("leg_Size"))) {
									InLast.LegSize = FCString::Atof(**InLegSize);
								}
								if (FString* InWaistSize = Tmp.Rows.Find(TEXT("waist_size"))) {
									InLast.WaistSize = FCString::Atof(**InWaistSize);
								}
								if (FString* InArmSize = Tmp.Rows.Find(TEXT("arm_size"))) {
									InLast.ArmSize = FCString::Atof(**InArmSize);
								}
							}
						}
					}
				}

				// 把数据源压缩成JSON
				FString JsonString;
				NetDataAnalysis::CharacterAppearancesToString(CharacterAppearances, JsonString);

				// 发回去
				SIMPLE_PROTOCOLS_SEND(SP_CharacterAppearanceResponses, AddrInfo, JsonString);
				UE_LOG(LogMMOARPGdbServer, Display, TEXT("[SP_CharacterAppearanceResponses], dbServer已发送Response!!!!!"));
			}
			break;
		}

		/** 核验角色命名的请求: */
		case SP_CheckCharacterNameRequests:// 收到协议: 来自网关转发的 核验角色命名的请求.
		{
			/* 收到来自网关的数据请求. */
			int32 UserID = INDEX_NONE;// 用户ID
			FString CharacterName;// 键入的待核验名称.
			FSimpleAddrInfo AddrInfo;// 中转作用的网关地址.
			SIMPLE_PROTOCOLS_RECEIVE(SP_CheckCharacterNameRequests, UserID, CharacterName, AddrInfo);// 收到来自网关的数据请求.

			/* 核验键入的名字. */
			ECheckNameType CheckNameType = ECheckNameType::UNKNOWN_ERROR;
			if (UserID > 0.0f) {// ID在数据库里正常大于0.
				CheckNameType = CheckName(CharacterName);
			}

			// 处理完之后 把回复 发回至 Gate-dbClient
			SIMPLE_PROTOCOLS_SEND(SP_CheckCharacterNameResponses, CheckNameType, AddrInfo);
			// Print.
			UE_LOG(LogMMOARPGdbServer, Display, TEXT("[SP_CheckCharacterNameResponses], db-server-CheckCharacterName."));
			break;
		}

		/** 创建一个舞台角色的请求.*/
		case SP_CreateCharacterRequests:
		{
			/* 收到来自网关的数据请求. */
			int32 UserID = INDEX_NONE;// 用户ID
			FSimpleAddrInfo AddrInfo;// 中转作用的网关地址.
			FString JsonString_CA;// CA存档压缩成的json.
			SIMPLE_PROTOCOLS_RECEIVE(SP_CreateCharacterRequests, UserID, JsonString_CA, AddrInfo);

			if (UserID > 0.0f) {// ID在数据库里正常大于0.
				// 从json里解析出恰当的CA存档.
				FMMOARPGCharacterAppearance CA_receive;
				NetDataAnalysis::StringToCharacterAppearances(JsonString_CA, CA_receive);

				// 处理CA存档.
				if (CA_receive.SlotPosition != INDEX_NONE) {

					/// 0.核验键入的名字并返回1个检查结果.
					ECheckNameType NameType_check = CheckName(CA_receive.Name);
					bool bCreateCharacter = false;// 创建角色信号.


					/// 当且仅当 核验类型为新增的名字,原本数据库里不存在的.
					if (NameType_check == ECheckNameType::NAME_NOT_EXIST) {
						TArray<FString> CAIDs;// 所有扫到的用户ID.
						bool bMeta = false;// 是否存在元数据.

						/// 1.先拿到用户数据.
						{
							/** 使用SQL语句向db中 表wp_usermeta里meta_key="character_ca"的字段 . */
							FString SQL = FString::Printf(
								TEXT("SELECT meta_value FROM wp_usermeta WHERE user_id=%i and meta_key=\"character_ca\";"), UserID);
							/** 使用语句拉取db上的表数据. */
							TArray<FSimpleMysqlResult> Result;
							if (Get(SQL, Result) == true) {
								if (Result.Num() > 0) {/* 说明db上存在数据源.*/
									for (auto& Tmp : Result) {
										if (FString* InMetaValue = Tmp.Rows.Find(TEXT("meta_value"))) {
											InMetaValue->ParseIntoArray(CAIDs, TEXT("|"));// 把类似2|3|4这种拆出来 2 3 4,存进CAIDs.
										}
									}
									bMeta = true;
								}
								bCreateCharacter = true;// 确认创建信号.
							}
						}

						/// 2.插入数据
						if (bCreateCharacter == true)/* 仅当有创建信号. */
						{
							FString SQL = FString::Printf(TEXT("INSERT INTO mmoarpg_characters_ca(\
								mmoarpg_name,mmoarpg_date,mmoarpg_slot,leg_size,waist_size,arm_size) \
								VALUES(\"%s\",\"%s\",%i,%.2lf,%.2lf,%.2lf);"),// 使用长浮点型,增大带宽.
								*CA_receive.Name, *CA_receive.Date, CA_receive.SlotPosition,
								CA_receive.LegSize, CA_receive.WaistSize, CA_receive.ArmSize);

							// 向数据库提交这条插入命令如果成功.就把语句刷新为按名字查找.
							if (Post(SQL)) {
								SQL = FString::Printf(TEXT("SELECT id FROM mmoarpg_characters_ca WHERE mmoarpg_name=\"%s\";"), *CA_receive.Name);
								TArray<FSimpleMysqlResult> Result;
								if (Get(SQL, Result)) {
									if (Result.Num() > 0) {
										for (auto& Tmp : Result) {
											if (FString* InIDString = Tmp.Rows.Find(TEXT("id"))) {
												CAIDs.Add(*InIDString);
											}
										}
									}
								}
								else {
									bCreateCharacter = false;
								}
							}
							else {
								bCreateCharacter = false;
							}
						}

						/// 3.更新元数据
						if (bCreateCharacter == true)/* 仅当有创建信号. */
						{
							// 之前已经拿取到完整的 CAIDs, 故下一步执行拼接字符串.
							FString IDStirng;
							for (auto& Tmp : CAIDs) {
								IDStirng += Tmp + TEXT("|");
							}
							IDStirng.RemoveFromEnd(TEXT("|"));

							FString SQL;
							if (bMeta == true) {// 仅当存在元数据, 则使用 SQL语句:更新.
								SQL = FString::Printf(TEXT("UPDATE wp_usermeta \
									SET meta_value=\"%s\" WHERE meta_key=\"character_ca\" and user_id=%i;"),
									*IDStirng, UserID);
							}
							else {// 没有元数据. 则使用SQL语句: 插入.
								SQL = FString::Printf(TEXT("INSERT INTO wp_usermeta( \
									user_id,meta_key,meta_value) VALUES(%i,\"character_ca\",\"%s\")"),
									UserID, *IDStirng);
							}

							if (!Post(SQL)) {/* 提交SQL语句失败也刷新创建信号为假. */
								bCreateCharacter = false;
							}
						}

					}

					// 处理完之后 把Response 发回至 Gate-dbClient
					SIMPLE_PROTOCOLS_SEND(SP_CreateCharacterResponses, NameType_check, bCreateCharacter, JsonString_CA, AddrInfo);
					// Print.
					UE_LOG(LogMMOARPGdbServer, Display, TEXT("[SP_CreateCharacterResponses], db-server-CreateCharacter."));
				}
			}
			break;
		}

		/** 移除角色存档 */
		case SP_DeleteCharacterRequests:
		{
			// 拿到HallMain发过来的 用户ID、要删除的槽号.
			int32 UserID = INDEX_NONE;
			FSimpleAddrInfo AddrInfo;
			int32 SlotID = INDEX_NONE;
			SIMPLE_PROTOCOLS_RECEIVE(SP_DeleteCharacterRequests, UserID, SlotID, AddrInfo);

			if (UserID != INDEX_NONE && SlotID != INDEX_NONE) {
				//I 获取元数据
				FString SQL = FString::Printf(TEXT("SELECT meta_value FROM wp_usermeta WHERE meta_key=\"character_ca\" and User_id=%i;"), UserID);
				TArray<FSimpleMysqlResult> Result;
				FString IDs;
				if (Get(SQL, Result)) {
					if (Result.Num() > 0) {
						// 用字符串拼接 把元数据表里的诸如 38|39|40 替换为 38,39,40; 这些ID号指向者玩家存档数据表.
						for (auto& Tmp : Result) {
							if (FString* InMetaValue = Tmp.Rows.Find(TEXT("meta_value"))) {
								IDs = InMetaValue->Replace(TEXT("|"), TEXT(","));
							}
						}
					}
				}

				//II 获取准备移除的数据ID
				int32 SuccessDeleteCount = 0;

				SQL = FString::Printf(TEXT("SELECT id FROM mmoarpg_characters_ca WHERE id in (%s) and mmoarpg_slot=%i;"),
					*IDs, SlotID);

				FString RemoveID;
				Result.Empty();
				if (Get(SQL, Result)) {
					if (Result.Num() > 0) {
						for (auto& Tmp : Result) {
							if (FString* InIDValue = Tmp.Rows.Find(TEXT("id"))) {
								RemoveID = *InIDValue;
								SuccessDeleteCount++;
							}
						}
					}
				}

				//III 删除Slot对应的对象
				if (!IDs.IsEmpty()) {
					SQL = FString::Printf(TEXT("DELETE FROM mmoarpg_characters_ca WHERE ID in (%s) and mmoarpg_slot=%i;"), *IDs, SlotID);
					if (Post(SQL)) {
						SuccessDeleteCount++;// 成功删除的计数加一.
					}
				}

				//V 删除执行成功后还需要去 更新元数据表
				if (SuccessDeleteCount > 1) {
					TArray<FString> IDArray;
					IDs.ParseIntoArray(IDArray, TEXT(","));//  拆解诸如38,39,40提出三个数字.

					// 真正的从所有ID里 移除符合条件的ID.
					IDArray.Remove(RemoveID);

					// 再把所有ID重新拼接一次,拼接成符合诸如 38|39|40的形式, 重新添加至元数据表.
					IDs.Empty();
					for (auto& Tmp : IDArray) {
						IDs += (Tmp + TEXT("|"));
					}
					IDs.RemoveFromEnd("|");

					SQL = FString::Printf(TEXT("UPDATE wp_usermeta \
							SET meta_value=\"%s\" WHERE meta_key=\"character_ca\" and user_id=%i;"),
						*IDs, UserID);

					if (Post(SQL)) {
						SuccessDeleteCount++;
					}
				}

				SIMPLE_PROTOCOLS_SEND(SP_DeleteCharacterResponses, UserID, SlotID, SuccessDeleteCount, AddrInfo);
			}
			break;
		}

		/** 玩家注册 */
		case SP_PlayerRegistInfoRequests:
		{
			//
			int32 UserID = INDEX_NONE;
			int32 SlotID = INDEX_NONE;
			FSimpleAddrInfo GateAddrInfo;
			FSimpleAddrInfo CenterAddrInfo;
			SIMPLE_PROTOCOLS_RECEIVE(SP_PlayerRegistInfoRequests, UserID, SlotID, GateAddrInfo, CenterAddrInfo);

			//
			if (UserID != INDEX_NONE && SlotID != INDEX_NONE)
			{
// 				FString UserInfoJson;
// 				FString SlotInfoJson;
// 				if (GetUserInfo(UserID, UserInfoJson) && GetSlotCAInfo(UserID, SlotID, SlotInfoJson)) {
// 					SIMPLE_PROTOCOLS_SEND(SP_PlayerRegistInfoResponses, UserInfoJson, SlotInfoJson, GateAddrInfo, CenterAddrInfo);
// 				}
// 				else {
// 					UserInfoJson = TEXT("[]");
// 					SlotInfoJson = TEXT("[]");
// 					SIMPLE_PROTOCOLS_SEND(SP_PlayerRegistInfoResponses, UserInfoJson, SlotInfoJson, GateAddrInfo, CenterAddrInfo);
// 				}
			}
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

/** 给定键入的名字 并核验它的检查类型. */
ECheckNameType UMMOARPGServerObejct::CheckName(const FString& InName)
{
	ECheckNameType CheckNameType = ECheckNameType::UNKNOWN_ERROR;
	if (!InName.IsEmpty()) {
		/** 使用SQL语句向db查询这个键入的待核验名字. */
		FString SQL = FString::Printf(TEXT("SELECT id FROM mmoarpg_characters_ca WHERE mmoarpg_name = \"%s\";"), *InName);
		/** 使用语句拉取db上的表数据. */
		TArray<FSimpleMysqlResult> Result;
		if (Get(SQL, Result)) {
			if (Result.Num() > 0) {/* 说明db上存在名字.*/
				CheckNameType = ECheckNameType::NAME_EXIST;
			}
			else {/* 说明db上不存在名字.*/
				CheckNameType = ECheckNameType::NAME_NOT_EXIST;
			}
		}
		else {/* 说明服务器出问题.*/
			CheckNameType = ECheckNameType::SERVER_NOT_EXIST;
		}
	}
	return CheckNameType;
}
