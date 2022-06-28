#include "MMOARPGServerObject.h"
#include "SimpleMySQLibrary.h"
#include "MysqlConfig.h"
#include "Log\MMOARPGdbServerLog.h"
#include "Protocol/LoginProtocol.h"
#include "Global/SimpleNetGlobalInfo.h"
#include "../../SimpleHTTP/Source/SimpleHTTP/Public/SimpleHTTPManage.h"
#include "Protocol/HallProtocol.h"
#include <Protocol/ServerProtocol.h>
#include "Protocol/GameProtocol.h"
#include "MMOARPGType.h"
#include "Misc/FileHelper.h"
#include "Json.h"
#include "MMOARPGTagList.h"// 这个是引擎插件里的

#if PLATFORM_WINDOWS
#pragma optimize("",off) 
#endif

TMap<int32, FMMOARPGCharacterAttribute> UMMOARPGServerObejct::MMOARPGCharacterAttribute;// 人物属性缓存池 类外初始化.

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

	// 初始化一张用于玩家属性集的表
	FString Create_mmoarpg_character_a_SQL =
		TEXT("CREATE TABLE IF NOT EXISTS `mmoarpg_characters_a`(\
		`id` INT UNSIGNED AUTO_INCREMENT,\
		`user_id` INT UNSIGNED DEFAULT '0',\
		`character_id` INT UNSIGNED DEFAULT '0',\
		`mmoarpg_slot` INT UNSIGNED DEFAULT '0',\
		`Level_Base` double(11,4) DEFAULT '0.00',\
		`Level_Current` double(11,4) DEFAULT '0.00',\
		`Health_Base` double(11,4) DEFAULT '0.00',\
		`Health_Current` double(11,4) DEFAULT '0.00',\
		`MaxHealth_Base` double(11,4) DEFAULT '0.00',\
		`MaxHealth_Current` double(11,4) DEFAULT '0.00',\
		`Mana_Base` double(11,4) DEFAULT '0.00',\
		`Mana_Current` double(11,4) DEFAULT '0.00',\
		`MaxMana_Base` double(11,4) DEFAULT '0.00',\
		`MaxMana_Current` double(11,4) DEFAULT '0.00',\
		`PhysicsAttack_Base` double(11,4) DEFAULT '0.00',\
		`PhysicsAttack_Current` double(11,4) DEFAULT '0.00',\
		`MagicAttack_Base` double(11,4) DEFAULT '0.00',\
		`MagicAttack_Current` double(11,4) DEFAULT '0.00',\
		`PhysicsDefense_Base` double(11,4) DEFAULT '0.00',\
		`PhysicsDefense_Current` double(11,4) DEFAULT '0.00',\
		`MagicDefense_Base` double(11,4) DEFAULT '0.00',\
		`MagicDefense_Current` double(11,4) DEFAULT '0.00',\
		`AttackRange_Base` double(11,4) DEFAULT '0.00',\
		`AttackRange_Current` double(11,4) DEFAULT '0.00',\
		`ComboAttack` VARCHAR(256) NOT NULL,\
		`Skill` VARCHAR(256) NOT NULL,\
		`Limbs` VARCHAR(256) NOT NULL,\
		 PRIMARY KEY(`id`)\
		) ENGINE = INNODB DEFAULT CHARSET = utf8; ");
	if (!Post(Create_mmoarpg_character_a_SQL)) {
		UE_LOG(LogMMOARPGdbServer, Error, TEXT("create table mmoarpg_characters_a (GASAttributeSet) failed."));
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
		/** 登录请求. */
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
							Delegate.SimpleCompleteDelegate.BindUObject(this, &UMMOARPGServerObejct::CheckPasswordResult_callback);
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

		/** 注册用户请求. --来自LoginServer */
		case SP_RegisterRequests:
		{
			FString RegisterString;
			FSimpleAddrInfo AddrInfo;// 网关地址, 来自LoginServer发送过来.
			SIMPLE_PROTOCOLS_RECEIVE(SP_RegisterRequests, RegisterString, AddrInfo)

			ERegistrationType RegistrationType = ERegistrationType::SERVER_BUG_WRONG;
			if (!RegisterString.IsEmpty()) {
				//名字验证
				TArray<FString> Value;
				RegisterString.ParseIntoArray(Value, TEXT("&"));
				if (Value.IsValidIndex(1) && Value.IsValidIndex(2)) {
					FString AccountString = Value[2];
					FString EmailString = Value[1];
					if (EmailString.RemoveFromStart(TEXT("Email=")) && AccountString.RemoveFromStart(TEXT("Account="))) {
						// 账户在服务器是不是有重复的
						FString SQL = FString::Printf(TEXT("SELECT ID FROM wp_users WHERE user_login = \"%s\" or user_email= \"%s\";"),
							*AccountString,
							*EmailString);

						TArray<FSimpleMysqlResult> Result;
						if (Get(SQL, Result)) {
							if (Result.Num() > 0) {
								RegistrationType = ERegistrationType::ACCOUNT_AND_EMAIL_REPETITION_ERROR;
								SIMPLE_PROTOCOLS_SEND(SP_RegisterResponses, AddrInfo, RegistrationType);
							}
							else {
								FString Parma = FString::Printf(TEXT("%s&IP=%i&Port=%i&Channel=%s&UserID=%i"),
									*RegisterString,
									AddrInfo.Addr.IP,
									AddrInfo.Addr.Port,
									*AddrInfo.ChannelID.ToString());

								FSimpleHTTPResponseDelegate Delegate;
								Delegate.SimpleCompleteDelegate.BindUObject(this, &UMMOARPGServerObejct::CheckRegisterResult);

								FString WpIP = FSimpleNetGlobalInfo::Get()->GetInfo().PublicIP;

								SIMPLE_HTTP.PostRequest(
									*FString::Printf(TEXT("http://%s/wp/wp-content/plugins/SimpleUserRegistration/SimpleUserRegistration.php"),
									*WpIP),
									*Parma,
									Delegate);

							}
						}
					}
				}
			}

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
					TArray<FString> OutIDs;
					if (GetCharacterCAInfoByUserMeta(UserID, OutIDs) == true) {// 拆解出UserID的所有元数据的ID集.
						GetSerialString(TEXT(","), OutIDs, IDs);
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

					if (bCreateCharacter == true) {
						// 创建角色属性集
						if (FMMOARPGCharacterAttribute* InCharacterAttribute = MMOARPGCharacterAttribute.Find(1)) {// 若属性集缓存池里找到人物号为1的
							bCreateCharacter = CreateAndUpdateCharacterAttributeInfo(UserID, 1, *InCharacterAttribute, CA_receive.SlotPosition);
						}
						else {// 若 没有在属性集缓存池里找到人物号为1的
							bCreateCharacter = false;
							UE_LOG(LogMMOARPGdbServer, Display, TEXT("The data of player 1 cannot be found in the attribute table. Check whether the attribute table exists."));
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
			if (UserID != INDEX_NONE && SlotID != INDEX_NONE) {
				FString UserInfoJson;
				FString SlotCAInfoJson;
				if (GetUserInfo(UserID, UserInfoJson) && GetSlotCAInfo(UserID, SlotID, SlotCAInfoJson)) {// 两份Json都注入成功.

					SIMPLE_PROTOCOLS_SEND(SP_PlayerRegistInfoResponses, UserInfoJson, SlotCAInfoJson, GateAddrInfo, CenterAddrInfo);// 把处理后的用户信息JSON发出去.
				}
				else {// 两份JSON只要有一个注入失败.
					UserInfoJson = TEXT("[]");
					SlotCAInfoJson = TEXT("[]");
					SIMPLE_PROTOCOLS_SEND(SP_PlayerRegistInfoResponses, UserInfoJson, SlotCAInfoJson, GateAddrInfo, CenterAddrInfo);// 把处理后的用户信息JSON发出去.
				}
			}
		}

		/** 游戏协议: GAS人物属性集请求. */
		case SP_GetCharacterDataRequests:
		{
			FSimpleAddrInfo CenterAddrInfo;
			int32 UserID = INDEX_NONE;
			int32 CharacterID = INDEX_NONE;
			int32 MMOARPG_Slot = INDEX_NONE;
			SIMPLE_PROTOCOLS_RECEIVE(SP_GetCharacterDataRequests, UserID, CharacterID, MMOARPG_Slot, CenterAddrInfo);

			if (UserID != INDEX_NONE && CharacterID != INDEX_NONE && MMOARPG_Slot != INDEX_NONE) {
				/* 从DB上拉取数据, 以此构造1个GAS玩家属性集*/

				FMMOARPGCharacterAttribute CharacterAttribute;// 构造出的人物属性集.
				{
					if (!IsCharacterAttributeExit(UserID, CharacterID, MMOARPG_Slot)) {/* 不存在属性集就直接创建.*/
						if (FMMOARPGCharacterAttribute* InCharacterAttribute = MMOARPGCharacterAttribute.Find(CharacterID)) {
							CharacterAttribute = *InCharacterAttribute;// 拿到缓存池里的对应人物号的属性集.
						}

						if (CreateCharacterAttributeInfo(UserID, CharacterID, CharacterAttribute, MMOARPG_Slot)) {
							UE_LOG(LogMMOARPGdbServer, Display, TEXT("Data table [%i] created successfully"), CharacterID);
						}
						else {
							UE_LOG(LogMMOARPGdbServer, Error, TEXT("Failed to create data table[%i]."), CharacterID);
						}
					}
					else {
						FString SQL = FString::Printf(TEXT("SELECT * FROM `mmoarpg_characters_a` WHERE user_id = %i and character_id=%i and mmoarpg_slot=%i;"), UserID, CharacterID, MMOARPG_Slot);
						TArray<FSimpleMysqlResult> Result;
						if (Get(SQL, Result)) {
							if (Result.Num() > 0) {
								for (auto& Tmp : Result) {
									GetAttributeInfo(TEXT("Health"), CharacterAttribute.Health, Tmp.Rows);
									GetAttributeInfo(TEXT("MaxHealth"), CharacterAttribute.MaxHealth, Tmp.Rows);
									GetAttributeInfo(TEXT("Mana"), CharacterAttribute.Mana, Tmp.Rows);
									GetAttributeInfo(TEXT("MaxMana"), CharacterAttribute.MaxMana, Tmp.Rows);

									GetAttributeInfo(TEXT("PhysicsAttack"), CharacterAttribute.PhysicsAttack, Tmp.Rows);
									GetAttributeInfo(TEXT("MagicAttack"), CharacterAttribute.MagicAttack, Tmp.Rows);
									GetAttributeInfo(TEXT("PhysicsDefense"), CharacterAttribute.PhysicsDefense, Tmp.Rows);
									GetAttributeInfo(TEXT("MagicDefense"), CharacterAttribute.MagicDefense, Tmp.Rows);
									GetAttributeInfo(TEXT("AttackRange"), CharacterAttribute.AttackRange, Tmp.Rows);
									GetAttributeInfo(TEXT("Level"), CharacterAttribute.Level, Tmp.Rows);

									GetAttributeInfo(TEXT("ComboAttack"), CharacterAttribute.ComboAttack, Tmp.Rows);
									GetAttributeInfo(TEXT("Skill"), CharacterAttribute.Skill, Tmp.Rows);
									GetAttributeInfo(TEXT("Limbs"), CharacterAttribute.Limbs, Tmp.Rows);
								}
							}
						}
					}
				}

				/* GAS玩家属性集并将其压入JSON. */
				FString CharacterDataJsonString;
				NetDataAnalysis::MMOARPGCharacterAttributeToString(CharacterAttribute, CharacterDataJsonString);
				/* 压好的JSON-玩家属性集发出去,发给CS-dbclient.*/
				SIMPLE_PROTOCOLS_SEND(SP_GetCharacterDataResponses, UserID, CharacterID, CharacterDataJsonString, CenterAddrInfo);
			}
			break;
		}

		/** 刷新人物属性集 */
		case SP_UpdateCharacterDataRequests:
		{
			int32 UserID = INDEX_NONE;
			FString JsonString;
			int32 MMOARPG_Slot = INDEX_NONE;
			SIMPLE_PROTOCOLS_RECEIVE(SP_UpdateCharacterDataRequests, UserID, MMOARPG_Slot, JsonString);

			if (!JsonString.IsEmpty()) {
				TMap<int32, FMMOARPGCharacterAttribute> CharacterAttributes;
				if (NetDataAnalysis::StringToMMOARPGCharacterAttribute(JsonString, CharacterAttributes)) {/* 传入的玩家属性集JSON解析为<玩家-属性集>Map*/
					if (CharacterAttributes.Num() > 0) {
						bool UpdateDataSuccessfully = true;
						for (auto& Tmp : CharacterAttributes) {
							if (!CreateAndUpdateCharacterAttributeInfo(UserID, Tmp.Key, Tmp.Value, MMOARPG_Slot)) {
								UpdateDataSuccessfully = false;
								SIMPLE_PROTOCOLS_SEND(SP_UpdateCharacterDataResponses, UserID, Tmp.Key, UpdateDataSuccessfully);
							}
						}
						int32 CharacterID = 0;
						SIMPLE_PROTOCOLS_SEND(SP_UpdateCharacterDataResponses, UserID, CharacterID, UpdateDataSuccessfully);
					}
				}
			}
			break;
		}

		/**  */
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

void UMMOARPGServerObejct::CheckPasswordResult_callback(const FSimpleHttpRequest& InRequest, const FSimpleHttpResponse& InResponse, bool bLinkSuccessful)
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
					if (GetUserInfo(UserID, String) == true) {// 将指定用户ID的用户信息注入Json
						// 回复给客户端,通知解析成功.
						ELoginType Type = ELoginType::LOGIN_SUCCESS;
						SIMPLE_PROTOCOLS_SEND(SP_LoginResponses, AddrInfo, Type, String);
					}
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

void UMMOARPGServerObejct::CheckRegisterResult(const FSimpleHttpRequest& InRequest, const FSimpleHttpResponse& InResponse, bool bLinkSuccessful)
{
	//2130706433&51509&3B76DAFE4F31856008E3AB82AED291C3&0
	if (bLinkSuccessful) {
		//xx&IP&Port&0
		TArray<FString> Values;
		InResponse.ResponseMessage.ParseIntoArray(Values, TEXT("&"));

		FSimpleAddrInfo AddrInfo;
		uint32 UserID = 0;
		ERegistrationVerification RV = ERegistrationVerification::REGISTRATION_FAIL;
		if (Values.Num()) {
			if (Values.IsValidIndex(0)) {
				AddrInfo.Addr.IP = FCString::Atoi(*Values[0]);
			}
			if (Values.IsValidIndex(1)) {
				AddrInfo.Addr.Port = FCString::Atoi(*Values[1]);
			}
			if (Values.IsValidIndex(2)) {
				FGuid::ParseExact(Values[2], EGuidFormats::Digits, AddrInfo.ChannelID);
			}
			if (Values.IsValidIndex(3)) {
				RV = (ERegistrationVerification)FCString::Atoi(*Values[3]);
			}

			ERegistrationType RegistrationType = ERegistrationType::SERVER_BUG_WRONG;
			if (RV == REGISTRATION_SUCCESS) {
				RegistrationType = ERegistrationType::PLAYER_REGISTRATION_SUCCESS;
				SIMPLE_PROTOCOLS_SEND(SP_RegisterResponses, AddrInfo, RegistrationType);
			}
			else {
				SIMPLE_PROTOCOLS_SEND(SP_RegisterResponses, AddrInfo, RegistrationType);
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

/** 将指定ID的用户信息注入JSon */
bool UMMOARPGServerObejct::GetUserInfo(int32 InUserID, FString& OutJsonString)
{
	FMMOARPGUserData UserData;
	UserData.ID = InUserID;

	FString SQL = FString::Printf(TEXT("SELECT user_login,user_email,user_url,display_name FROM wp_users WHERE ID=%i;"), InUserID);
	TArray<FSimpleMysqlResult> Result;
	if (Get(SQL, Result)) {
		if (Result.Num() > 0) {
			for (auto& Tmp : Result) {
				if (FString* InUserLogin = Tmp.Rows.Find(TEXT("user_login"))) {
					UserData.Account = *InUserLogin;
				}
				if (FString* InUserEmail = Tmp.Rows.Find(TEXT("user_email"))) {
					UserData.EMail = *InUserEmail;
				}
				//if (FString* InUserUrl = Tmp.Rows.Find(TEXT("user_url")))
				//{
				//	UserData.EMail = *InUserEmail;
				//}
				if (FString* InDisplayName = Tmp.Rows.Find(TEXT("display_name"))) {
					UserData.Name = *InDisplayName;
				}
			}
		}
	}

	// 用户数据写入Json
	NetDataAnalysis::UserDataToString(UserData, OutJsonString);

	return OutJsonString.Len() > 0;
}

/** 将指定ID的Slot信息注入JSon */
bool UMMOARPGServerObejct::GetSlotCAInfo(int32 InUserID, int32 InSlotCAID, FString& OutJsonString)
{
	TArray<FString> IDs;
	if (GetCharacterCAInfoByUserMeta(InUserID, IDs) == true) {// 拆解出指定用户ID的 关联元数据 character_ca这个字段里存储的所有ID.
		FString IDString;// 是所有ID集
		GetSerialString(TEXT(","), IDs, IDString);// 将字符集输出成满足 s1, s2, s3, s4的形式.

		FString SQL = FString::Printf(TEXT("SELECT * from mmoarpg_characters_ca where id in(%s) and mmoarpg_slot=%i;"), *IDString, InSlotCAID);
		TArray<FSimpleMysqlResult> Result;
		if (Get(SQL, Result)) {
			if (Result.Num() > 0) {
				FMMOARPGCharacterAppearance CA;
				for (auto& Tmp : Result) {
					if (FString* InName = Tmp.Rows.Find(TEXT("mmoarpg_name"))) {
						CA.Name = *InName;
					}
					if (FString* InDate = Tmp.Rows.Find(TEXT("mmoarpg_date"))) {
						CA.Date = *InDate;
					}
					if (FString* InSlot = Tmp.Rows.Find(TEXT("mmoarpg_slot"))) {
						CA.SlotPosition = FCString::Atoi(**InSlot);
					}
					if (FString* InLegSize = Tmp.Rows.Find(TEXT("leg_Size"))) {
						CA.LegSize = FCString::Atof(**InLegSize);
					}
					if (FString* InWaistSize = Tmp.Rows.Find(TEXT("waist_size"))) {
						CA.WaistSize = FCString::Atof(**InWaistSize);
					}
					if (FString* InArmSize = Tmp.Rows.Find(TEXT("arm_size"))) {
						CA.ArmSize = FCString::Atof(**InArmSize);
					}
				}
				// 把CA存档压缩成Json包.
				NetDataAnalysis::CharacterAppearancesToString(CA, OutJsonString);

				return !OutJsonString.IsEmpty();
			}
		}
	}

	return false;
}

/** 拆解出指定用户ID的 关联元数据 character_ca这个字段里存储的所有ID. */
bool UMMOARPGServerObejct::GetCharacterCAInfoByUserMeta(int32 InUserID, TArray<FString>& OutIDs)
{
	// 先拿到元数据 character_ca这个字段里存储的所有ID.
	FString SQL = FString::Printf(TEXT("SELECT meta_value from wp_usermeta where user_id=%i and meta_key=\"character_ca\";"), InUserID);
	TArray<FSimpleMysqlResult> Result;
	if (Get(SQL, Result)) {
		if (Result.Num() > 0) {
			for (auto& Tmp : Result) {
				if (FString* InMetaValue = Tmp.Rows.Find(TEXT("meta_value"))) {
					InMetaValue->ParseIntoArray(OutIDs, TEXT("|"));// 按|号拆解出来.
				}
			}
		}
		return true;
	}

	return OutIDs.Num() > 0;
}

/** 将字符集输出成满足 s1, s2, s3, s4的形式 */
void UMMOARPGServerObejct::GetSerialString(TCHAR* InSplitPrefix, const TArray<FString>& InStrings, FString& OutString)
{
	for (auto& Tmp : InStrings) {
		OutString += Tmp + InSplitPrefix;
	}
	OutString.RemoveFromEnd(TEXT(","));
}

// 工具方法,拿取属性集,同名重载
void UMMOARPGServerObejct::GetAttributeInfo(const FString& InAttributeName, FMMOARPGAttributeData& OutAttributeData, const TMap<FString, FString>& InRow)
{
	if (const FString* InBase = InRow.Find(InAttributeName + TEXT("_Base"))) {
		OutAttributeData.BaseValue = FCString::Atof(**InBase);
	}

	if (const FString* InCurrent = InRow.Find(InAttributeName + TEXT("_Current"))) {
		OutAttributeData.CurrentValue = FCString::Atof(**InCurrent);
	}
}

// 工具方法,拿取属性集,同名重载
void UMMOARPGServerObejct::GetAttributeInfo(const FString& InAttributeName, TArray<FName>& OutAttributeData, const TMap<FString, FString>& InRowMap)
{
	if (const FString* InBase = InRowMap.Find(InAttributeName)) {
		NetDataAnalysis::AnalysisToArrayName(*InBase, OutAttributeData);
	}
}

// 工具方法; 判断角色属性集是否存在.
bool UMMOARPGServerObejct::IsCharacterAttributeExit(int32 InUserID, int32 InCharacterID, int32 MMOARPG_Slot)
{
	FString SQL = FString::Printf(TEXT("SELECT count(id) FROM `mmoarpg_characters_a` WHERE character_id=%i and user_id=%i and mmoarpg_slot=%i;"), InCharacterID, InUserID, MMOARPG_Slot);
	TArray<FSimpleMysqlResult> Result;

	if (Get(SQL, Result)) {
		if (Result.Num() > 0) {
			for (auto& Tmp : Result) {
				if (FString* Count_ID = Tmp.Rows.Find(TEXT("count(id)"))) {
					return (bool)FCString::Atoi(**Count_ID);
				}
			}
		}
	}

	return false;
}

// 工具方法; 创建并更新1个人物属性集.
bool UMMOARPGServerObejct::CreateAndUpdateCharacterAttributeInfo(int32 InUserID, int32 InCharacterID, const FMMOARPGCharacterAttribute& InAttributeData, int32 MMOARPG_Slot)
{
	// I.检查服务器是否已经存在了该对象
// 	bool bExistCharacterAttribute = IsCharacterAttributeExit(InUserID, InCharacterID, MMOARPG_Slot);

	// II.存在就更新
	if (IsCharacterAttributeExit(InUserID, InCharacterID, MMOARPG_Slot)) {
		return UpdateCharacterAttributeInfo(InUserID, InCharacterID, InAttributeData, MMOARPG_Slot);
	}
	// II.不存在就创建(插入)
	else {
		return CreateCharacterAttributeInfo(InUserID, InCharacterID, InAttributeData, MMOARPG_Slot);
	}
}

// 工具方法; 创建1个人物属性集.
bool UMMOARPGServerObejct::CreateCharacterAttributeInfo(int32 InUserID, int32 InCharacterID, const FMMOARPGCharacterAttribute& InAttributeData, int32 MMOARPG_Slot)
{
	FString	SQL = FString::Printf(TEXT(
		"INSERT INTO mmoarpg_characters_a (\
			 character_id,user_id,mmoarpg_slot,\
			 Level_Base,Level_Current,\
			 Health_Base,Health_Current,\
			 MaxHealth_Base,MaxHealth_Current,\
			 Mana_Base,Mana_Current,\
			 MaxMana_Base,MaxMana_Current,\
			 PhysicsAttack_Base,PhysicsAttack_Current,\
			 MagicAttack_Base,MagicAttack_Current,\
			 PhysicsDefense_Base,PhysicsDefense_Current,\
			 MagicDefense_Base,MagicDefense_Current,\
			 AttackRange_Base,AttackRange_Current,\
			 ComboAttack,Skill,Limbs) VALUES(\
			%i,%i,%i,\
			%.2lf,%.2lf,\
			%.2lf,%.2lf,\
			%.2lf,%.2lf,\
			%.2lf,%.2lf,\
			%.2lf,%.2lf,\
			%.2lf,%.2lf,\
			%.2lf,%.2lf,\
			%.2lf,%.2lf,\
			%.2lf,%.2lf,\
			%.2lf,%.2lf,\
			\"%s\",\"%s\",\"%s\");"),
		InCharacterID, InUserID, MMOARPG_Slot,
		InAttributeData.Level.BaseValue, InAttributeData.Level.CurrentValue,
		InAttributeData.Health.BaseValue, InAttributeData.Health.CurrentValue,
		InAttributeData.MaxHealth.BaseValue, InAttributeData.MaxHealth.CurrentValue,
		InAttributeData.Mana.BaseValue, InAttributeData.Mana.CurrentValue,
		InAttributeData.MaxMana.BaseValue, InAttributeData.MaxMana.CurrentValue,
		InAttributeData.PhysicsAttack.BaseValue, InAttributeData.PhysicsAttack.CurrentValue,
		InAttributeData.MagicAttack.BaseValue, InAttributeData.MagicAttack.CurrentValue,
		InAttributeData.PhysicsDefense.BaseValue, InAttributeData.PhysicsDefense.CurrentValue,
		InAttributeData.MagicDefense.BaseValue, InAttributeData.MagicDefense.CurrentValue,
		InAttributeData.AttackRange.BaseValue, InAttributeData.AttackRange.CurrentValue,
		*InAttributeData.ComboAttackToString(),
		*InAttributeData.SkillToString(),
		*InAttributeData.LimbsToString());

	if (Post(SQL)) {
		UE_LOG(LogMMOARPGdbServer, Display, TEXT("INSERT mmoarpg_characters_a true"));
		return true;
	}
	UE_LOG(LogMMOARPGdbServer, Display, TEXT("INSERT mmoarpg_characters_a false"));
	return false;
}

// 工具方法; 更新1个人物属性集.
bool UMMOARPGServerObejct::UpdateCharacterAttributeInfo(int32 InUserID, int32 InCharacterID, const FMMOARPGCharacterAttribute& InAttributeData, int32 MMOARPG_Slot)
{
	FString SQL = FString::Printf(
		TEXT("UPDATE mmoarpg_characters_a SET \
			Level_Base=%.2lf,\
			Level_Current=%.2lf,\
			Health_Base=%.2lf,\
			Health_Current=%.2lf,\
			MaxHealth_Base=%.2lf,\
			MaxHealth_Current=%.2lf,\
			Mana_Base=%.2lf,\
			Mana_Current=%.2lf,\
			MaxMana_Base=%.2lf,\
			MaxMana_Current=%.2lf,\
			PhysicsAttack_Base=%.2lf,\
			PhysicsAttack_Current=%.2lf,\
			MagicAttack_Base=%.2lf,\
			MagicAttack_Current=%.2lf,\
			PhysicsDefense_Base=%.2lf,\
			PhysicsDefense_Current=%.2lf,\
			MagicDefense_Base=%.2lf,\
			MagicDefense_Current=%.2lf,\
			AttackRange_Base=%.2lf,\
			AttackRange_Current=%.2lf,\
			ComboAttack=\"%s\",\
			Skill=\"%s\",\
			Limbs=\"%s\" \
			WHERE character_id=%i and user_id = %i and mmoarpg_slot=%i;"),
		InAttributeData.Level.BaseValue, InAttributeData.Level.CurrentValue,
		InAttributeData.Health.BaseValue, InAttributeData.Health.CurrentValue,
		InAttributeData.MaxHealth.BaseValue, InAttributeData.MaxHealth.CurrentValue,
		InAttributeData.Mana.BaseValue, InAttributeData.Mana.CurrentValue,
		InAttributeData.MaxMana.BaseValue, InAttributeData.MaxMana.CurrentValue,
		InAttributeData.PhysicsAttack.BaseValue, InAttributeData.PhysicsAttack.CurrentValue,
		InAttributeData.MagicAttack.BaseValue, InAttributeData.MagicAttack.CurrentValue,
		InAttributeData.PhysicsDefense.BaseValue, InAttributeData.PhysicsDefense.CurrentValue,
		InAttributeData.MagicDefense.BaseValue, InAttributeData.MagicDefense.CurrentValue,
		InAttributeData.AttackRange.BaseValue, InAttributeData.AttackRange.CurrentValue,
		*InAttributeData.ComboAttackToString(),
		*InAttributeData.SkillToString(),
		*InAttributeData.LimbsToString(),
		InCharacterID, InUserID, MMOARPG_Slot);

	if (Post(SQL)) {
		UE_LOG(LogMMOARPGdbServer, Display, TEXT("UPDATE mmoarpg_characters_a true"));
		return true;
	}
	UE_LOG(LogMMOARPGdbServer, Display, TEXT("UPDATE mmoarpg_characters_a false"));
	return false;
}

/** 初始化人物属性集(需要一个路径). */
bool UMMOARPGServerObejct::InitCharacterAttribute(const FString& InPath)
{
	// 此路径下的JSON文件不存在就报错.
	if (!IFileManager::Get().FileExists(*InPath)) {
		UE_LOG(LogMMOARPGdbServer, Error, TEXT("[%s]The file does not exist under this path."), *InPath);
		return false;
	}
	// 用来保存人物属性表的字符串.
	FString CharacterAttributeJson;

	// 若读取失败则报错.
	if (!FFileHelper::LoadFileToString(CharacterAttributeJson, *InPath)) {
		UE_LOG(LogMMOARPGdbServer, Error, TEXT("CharacterAttributeJson File read failed."));
		return false;
	}

	// 提前准备JSON读取器和readroot
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(CharacterAttributeJson);
	TArray<TSharedPtr<FJsonValue>> ReadRoot;

	/** 读取器反序列化 */
	if (FJsonSerializer::Deserialize(JsonReader, ReadRoot)) {
		
		/* Lambda--使用外部值注册1个mmo-attributedata */
		auto RegisterMMOARPGAttributeData = [](FMMOARPGAttributeData& InAttributeData, float InValue) {
			InAttributeData.BaseValue = InValue;
			InAttributeData.CurrentValue = InValue;
		};

		/* Lambda--解析属性集Json文件里各tag部分 */
		auto RegisterGameplayTag = [](const TArray<TSharedPtr<FJsonValue>>& GamePlayJsonTag, TArray<FName>& OutTag) ->void {
			TArray<FName> GamePlayTags;
			for (auto& Tmp : GamePlayJsonTag) {
				if (TSharedPtr<FJsonObject> InJsonObject = Tmp->AsObject()) {
					const TArray<TSharedPtr<FJsonValue>>& SubGamePlayJsonTag = InJsonObject->GetArrayField(TEXT("GameplayTags"));
					for (auto& SubTmp : SubGamePlayJsonTag) {
						if (TSharedPtr<FJsonObject> InSubJsonObject = SubTmp->AsObject()) {
							GamePlayTags.Add(*InSubJsonObject->GetStringField(TEXT("TagName")));

							AnalysisGamePlayTagsToArrayName(GamePlayTags, OutTag);
						}
					}
				}
			}

			if (!GamePlayTags.IsEmpty()) {
// 				AnalysisGamePlayTagsToArrayName(GamePlayTags, OutTag);
			}
		};

		// 保护性清零人物属性-缓存池
		MMOARPGCharacterAttribute.Empty();

		// 扫描Json数据源
		for (auto& Tmp : ReadRoot) {
			if (TSharedPtr<FJsonObject> InJsonObject = Tmp->AsObject()) {
				int32 ID = InJsonObject->GetIntegerField(TEXT("ID"));// 先拿一下人物号.
				MMOARPGCharacterAttribute.Add(ID, FMMOARPGCharacterAttribute());// 缓存池里先Add个ID-空属性集.
				FMMOARPGCharacterAttribute& InAttribute = MMOARPGCharacterAttribute[ID];// 再提出 刚才那个人物ID号的 缓存池里的属性集

				// 给提出来的属性集 各部分字段做值.
				RegisterMMOARPGAttributeData(InAttribute.Level, 1.f);
				RegisterMMOARPGAttributeData(InAttribute.Health, InJsonObject->GetNumberField(TEXT("Health")));
				RegisterMMOARPGAttributeData(InAttribute.MaxHealth, InJsonObject->GetNumberField(TEXT("Health")));
				RegisterMMOARPGAttributeData(InAttribute.Mana, InJsonObject->GetNumberField(TEXT("Mana")));
				RegisterMMOARPGAttributeData(InAttribute.MaxMana, InJsonObject->GetNumberField(TEXT("Mana")));
				RegisterMMOARPGAttributeData(InAttribute.PhysicsAttack, InJsonObject->GetNumberField(TEXT("PhysicsAttack")));
				RegisterMMOARPGAttributeData(InAttribute.MagicAttack, InJsonObject->GetNumberField(TEXT("MagicAttack")));
				RegisterMMOARPGAttributeData(InAttribute.PhysicsDefense, InJsonObject->GetNumberField(TEXT("PhysicsDefense")));
				RegisterMMOARPGAttributeData(InAttribute.MagicDefense, InJsonObject->GetNumberField(TEXT("MagicDefense")));
				RegisterMMOARPGAttributeData(InAttribute.AttackRange, InJsonObject->GetNumberField(TEXT("AttackRange")));
				RegisterGameplayTag(InJsonObject->GetArrayField(TEXT("ComboAttackTags")), InAttribute.ComboAttack);
				RegisterGameplayTag(InJsonObject->GetArrayField(TEXT("SkillTags")), InAttribute.Skill);
				RegisterGameplayTag(InJsonObject->GetArrayField(TEXT("LimbsTags")), InAttribute.Limbs);
			}
		}
		return true;
	}
	// 关联此Json文件的JSON读取器 若反序列化失败则报错.
	UE_LOG(LogMMOARPGdbServer, Error, TEXT("JSON deserialization failed."));

	return true;
}
