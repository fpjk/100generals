-- translation for YJCM Package

return {
	["YJCM"] = "一将成名",

	["#caozhi"] = "八斗之才",
	["caozhi"] = "曹植",
	["designer:caozhi"] = "Foxear",
	["illustrator:caozhi"] = "木美人",
	["luoying"] = "落英",
	[":luoying"] = "其他角色的牌因判定或弃置而置入弃牌堆时，你可以获得其中至少一张梅花牌。",
	["jiushi"] = "酒诗",
	[":jiushi"] = "若你的武将牌正面朝上，你可以将武将牌翻面，视为你使用了一张【酒】。每当你受到伤害扣减体力前，若武将牌背面朝上，你可以在伤害结算后将武将牌翻至正面朝上。",

	["#yujin"] = "魏武之刚",
	["yujin"] = "于禁",
	["illustrator:yujin"] = "Yi章",
	["jieyue"] = "节钺",
	[":jieyue"] = "结束阶段开始时，你可选择一名角色，请求其将一张牌置于你的武将牌上，称为 “钺”。若有“钺”， 你可将一张红色手牌当【闪】，黑色手牌当【无懈可击】使用或打出；准备阶段开始时， 你获得所有“钺”。",
	["@jieyue"] = "你可以发动“<font color=\"yellow\"><b>节钺</b></font>”",
	["@jieyue_put"] = "%src 对你发动了“<font color=\"yellow\"><b>节钺</b></font>”，请选择是否将一张牌置于其武将牌上",
	["jieyue_pile"] = "钺",
	
	["#zhonghui"] = "桀骜的野心家",
	["zhonghui"] = "钟会",
	["illustrator:zhonghui"] = "雪君S",
	["quanji"] = "权计",
	[":quanji"] = "当一名角色受到1点伤害后，若你与其的距离不大于1，其可以选择是否摸一张牌，然后将一张手牌置于你的武将牌上，称为“权”；若你未发动过“自立”，你的手牌上限＋X（X为“权”数）。",
	["quanji:quanji_invoke"] = "你要发动 %src 的技能“<font color=\"yellow\"><b>权计</b></font>”吗？",
	["quanji:quanji_draw"] = "请选择是否摸一张牌",
	["QuanjiPush"] = "请将一张手牌置于其武将牌上",
	["zili"] = "自立",
	[":zili"] = "觉醒技，准备阶段开始时，若“权”数不小于4，你减1点体力上限，然后你获得 “排异”（阶段技。你可将两张“权”置入弃牌堆并选择至多两名角色：其摸一张牌，然后若其手牌多于你，你对其造成1点伤害）。",
	["#ZiliWake"] = "%from 的“权”为 %arg 张，触发“%arg2”觉醒",
	["power"] = "权",
	["$ZiliAnimate"] = "image=image/animate/zili.png",
	["paiyi"] = "排异",
	[":paiyi"] = "阶段技。你可以将一张“权”置入弃牌堆并选择一名角色：若如此做，该角色摸两张牌：若其手牌多于你，该角色受到1点伤害。",
	
	["#fazheng"] = "蜀汉的辅翼",
	["fazheng"] = "法正",
	["designer:fazheng"] = "Michael_Lee",
	["illustrator:fazheng"] = "雷没才",
	["enyuan"] = "恩怨",
	[":enyuan"] = "锁定技。当你于回合外回复1点体力后，你令一名其他角色摸一张牌；锁定技。当你受到其他角色造成的伤害后，你令伤害来源或当前回合角色选择一项：1．将一张♥手牌交给你；2．失去1点体力。",
	["@enyuan-heart"] = "请给出一张红桃手牌",
	["@enyuan-en"] = "请选择一名其他角色令其摸一张牌",
	["@enyuan-yuan"] = "请选择“恩怨”的目标",
	["xuanhuo"] = "眩惑",
	[":xuanhuo"] = "阶段技。你可以将一张红桃手牌交给一名其他角色：若如此做，你获得该角色的一张牌，然后将此牌交给除该角色外的另一名角色。",
	["@xuanhuo-give"] = "你可以将此牌交给除 %src 外的一名角色",

	["#masu"] = "怀才自负",
	["masu"] = "马谡",
	["designer:masu"] = "点点",
	["illustrator:masu"] = "张帅",
	["xinzhan"] = "心战",
	[":xinzhan"] = "阶段技。你可以观看牌堆顶的三张牌，然后你可以展示并获得其中至少一张红桃牌，然后将其余的牌置于牌堆顶。",
	["huilei"] = "挥泪",
	[":huilei"] = "锁定技。你死亡时，你令杀死你的其他角色弃置其所有牌且不能获得奖励牌直至回合结束。",
	["#HuileiThrow"] = "%from 的“%arg”被触发，伤害来源 %to 弃置所有牌",

	["#lingtong"] = "豪情烈胆",
	["lingtong"] = "凌统",
	["illustrator:lingtong"] = "绵Myan",
	["xuanfeng"] = "旋风",
	[":xuanfeng"] = "每当你失去一次装备区的牌后，或弃牌阶段结束时若你于本阶段内弃置了至少两张你的牌，你可以弃置一名其他角色的一张牌，然后弃置一名其他角色的一张牌。",

	["#wuguotai"] = "武烈皇后",
	["wuguotai"] = "吴国太",
	["designer:wuguotai"] = "章鱼咬你哦",
	["illustrator:wuguotai"] = "zoo",
	["ganlu"] = "甘露",
	[":ganlu"] = "阶段技。你可以令装备区的牌数量差不超过你已损失体力值的两名角色交换他们装备区的装备牌。",
	["buyi"] = "补益",
	[":buyi"] = "每当一名角色进入濒死状态时，你可以展示该角色的一张手牌：若此牌为非基本牌，该角色弃置此牌，然后回复1点体力。",
	["#GanluSwap"] = "%from 交换了 %to 的装备",

	["#chengong"] = "刚直壮烈",
	["chengong"] = "陈宫",
	["designer:chengong"] = "Kaycent",
	["illustrator:chengong"] = "黑月乱",
	["mingce"] = "明策",
	[":mingce"] = "阶段技。你可以将一张装备牌或【杀】交给一名其他角色：若如此做，该角色可以视为对其攻击范围内由你选择的一名角色使用一张【杀】，否则其摸一张牌。",
	["zhichi"] = "智迟",
	[":zhichi"] = "锁定技。你的回合外，每当你受到伤害后，【杀】和非延时锦囊牌对你无效，直到回合结束。",
	["mingce:use"] = "对攻击范围内的一名角色使用一张【杀】",
	["mingce:draw"] = "摸一张牌",
	["#ZhichiDamaged"] = "%from 受到了伤害，本回合内【<font color=\"yellow\"><b>杀</b></font>】和非延时锦囊都将对其无效",
	["#ZhichiAvoid"] = "%from 的“%arg”被触发，【<font color=\"yellow\"><b>杀</b></font>】和非延时锦囊对其无效",
	
	["#xunyou"] = "曹魏的谋主",
	["xunyou"] = "荀攸",
	["designer:xunyou"] = "淬毒",
	["illustrator:xunyou"] = "魔鬼鱼",
	["qice"] = "奇策",
	[":qice"] = "阶段技。你可以将你的所有手牌（至少一张）当任意一张非延时锦囊牌使用。",
	["zhiyu"] = "智愚",
	[":zhiyu"] = "每当你受到伤害后，你可以摸一张牌：若如此做，你展示所有手牌。若你的手牌均为同一颜色，伤害来源弃置一张手牌。",

	["#wangyi"] = "决意的巾帼",
	["wangyi"] = "王异",
	["illustrator:wangyi"] = "Yi章",
	["zhenlie"] = "贞烈",
	[":zhenlie"] = "当你成为其他角色使用【杀】或非延时类锦囊牌的目标时，你可以取消自己并弃置其一张牌，然后失去1点体力。",
	["miji"] = "秘计",
	[":miji"] = "结束阶段开始时，若你已受伤，你可以摸至多X张牌，然后将至多等量的手牌任意分配给其他角色。若场上存活人数为5或更多，取消描述中的“至多”二字。（X为你已损失的体力值）",
	["miji_draw"] = "秘计摸牌数",

	["#liaohua"] = "历尽沧桑",
	["liaohua"] = "廖化",
	["designer:liaohua"] = "桃花僧",
	["illustrator:liaohua"] = "天空之城",
	["dangxian"] = "当先",
	[":dangxian"] = "准备阶段开始时，你可以弃置所有手牌（至少一张），视为对任意角色使用了一张【杀】。",
	["@dangxian"] = "你可以弃置所有手牌发动“当先”",
	["~dangxian"] = "选择【杀】的目标角色→点击确定",
	["fuli"] = "伏枥",
	[":fuli"] = "觉醒技。当你处于濒死状态时，你减1点体力上限，然后将体力值回复至2点，且“当先”描述中的“至少一张”修改为“无手牌则不弃”。",
	["$FuliAnimate"] = "image=image/animate/fuli.png",

	["#bulianshi"] = "无冕之后",
	["bulianshi"] = "步练师",
	["designer:bulianshi"] = "Anais",
	["illustrator:bulianshi"] = "勺子妞",
	["anxu"] = "安恤",
	[":anxu"] = "阶段技。你可选择两名手牌数不同的角色，令其中手牌多的选择是否令手牌少的角色观看其手牌并获得其中的一张。若其选择是，你摸一张牌。",
	["anxu-resp:resp"] = "你可以选择是否允许 %src 观看你的手牌并获得其中的一张",
	["zhuiyi"] = "追忆",
	[":zhuiyi"] = "你死亡时，你可以令一名其他角色（除杀死你的角色）摸三张牌并回复1点体力。",
	["zhuiyi-invoke"] = "你可以发动“追忆”<br/> <b>操作提示</b>: 选择一名其他角色→点击确定<br/>",
	["zhuiyi-invokex"] = "你可以发动“追忆”<br/> <b>操作提示</b>: 选择除 %src 外的一名其他角色→点击确定<br/>",

	["#handang"] = "石城侯",
	["handang"] = "韩当",
	["illustrator:handang"] = "DH",
	["gongqi"] = "弓骑",
	[":gongqi"] = "阶段技。你可以弃置一张牌：若如此做，本回合你的攻击范围无限；若此牌为装备牌，你可以弃置一名其他角色的一张牌。",
	["@gongqi-discard"] = "你可以弃置一名其他角色的一张牌",
	["jiefan"] = "解烦",
	[":jiefan"] = "限定技。出牌阶段，你可以选择一名角色，然后攻击范围内包含该角色的所有角色选择一项：弃置一张武器牌，或令该角色摸一张牌。",
	["@jiefan-discard"] = "请弃置一张武器牌，否则 %dest 摸一张牌",
	["$JiefanAnimate"] = "image=image/animate/jiefan.png",

	["#liubiao"] = "跨蹈汉南",
	["liubiao"] = "刘表",
	["designer:liubiao"] = "管乐",
	["illustrator:liubiao"] = "关东煮",
	["zishou"] = "自守",
	[":zishou"] = "摸牌阶段，你可多摸X张牌（X为势力数），若如此做，你不能使用除【桃】和【桃园结义】以外的牌直到回合结束。",
	["zongshi"] = "宗室",
	[":zongshi"] = "锁定技。你的手牌上限+X。（X为现存势力数）",
	
	["#guohuai"] = "垂问秦雍",
	["guohuai"] = "郭淮",
	["designer:guohuai"] = "雪•五月",
	["illustrator:guohuai"] = "DH",
	["jingce"] = "精策",
	[":jingce"] = "出牌阶段结束时，若你本回合已使用的牌数大于或等于你的体力值，你可以摸两张牌，且你此回合内手牌上限+1。",

	["#manchong"] = "政法兵谋",
	["manchong"] = "满宠",
	["designer:manchong"] = "方片杰克",
	["illustrator:manchong"] = "Aimer彩三",
	["lishou"] = "励守",
	[":lishou"] = "阶段技。你可以交给一名其他角色一张非基本手牌并对其造成1点伤害，然后其选择一项：获得一名除其以外角色的一张牌；或将武将牌恢复至游戏开始时的状态。",
	["lishou:obtain"] = "获得一名除其以外角色的一张牌",
	["lishou:reset"] = "重置并翻至正面朝上",

	["#guanping"] = "竭忠王",
	["guanping"] = "关平",
	["designer:guanping"] = "昂翼天使",
	["illustrator:guanping"] = "樱花闪乱",
	["longyin"] = "龙吟",
	[":longyin"] = "每当一名角色于出牌阶段内使用【杀】时，你可以弃置一张牌：若如此做，此【杀】不计入次数限制，若此【杀】为红色，你摸一张牌。",
	["@longyin"] = "你可以弃置一张牌发动“龙吟”",

	["#jianyong"] = "优游风议",
	["jianyong"] = "简雍",
	["designer:jianyong"] = "Nocihoo",
	["illustrator:jianyong"] = "Thinking",
	["qiaoshui"] = "巧说",
	[":qiaoshui"] = "出牌阶段开始时，你可以与一名其他角色拼点：若你赢，本回合你使用的下一张基本牌或非延时锦囊牌可以增加一个额外目标（无距离限制）或减少一名目标（若原有至少两名目标）；若你没赢，你使用锦囊牌不能选择其他角色为目标，直到回合结束。",
	["qiaoshui:add"] = "增加一名目标",
	["qiaoshui:remove"] = "减少一名目标",
	["@qiaoshui-card"] = "你可以发动“巧说”",
	["@qiaoshui-add"] = "请选择【%arg】的额外目标",
	["@qiaoshui-remove"] = "请选择【%arg】减少的目标",
	["~qiaoshui1"] = "选择一名其他角色→点击确定",
	["~qiaoshui"] = "选择【借刀杀人】的目标角色→选择【杀】的目标角色→点击确定",
	["zongshih"] = "纵适",
	[":zongshih"] = "每当你拼点赢，你可以获得对方的拼点牌。每当你拼点没赢，你可以获得你的拼点牌。",
	["#QiaoshuiAdd"] = "%from 发动了“%arg”为 %card 增加了额外目标 %to",
	["#QiaoshuiRemove"] = "%from 发动了“%arg”为 %card 减少了目标 %to",

	["#liufeng"] = "骑虎之殇",
	["liufeng"] = "刘封",
	["designer:liufeng"] = "香蒲神殇",
	["illustrator:liufeng"] = "Thinking",
	["xiansi"] = "陷嗣",
	[":xiansi"] = "出牌阶段开始时，你可将一张牌置于武将牌上，称为“逆”（此“逆”不随技能的失去而失去），然后弃置一至两名有牌的角色各一张牌。若如此做，当一名角色需要对你使用【杀】时，其可将一张“逆”置入弃牌堆，视为对你使用【杀】 （有距离限制）。",
	["@xiansi-card"] = "你可以发动“陷嗣”",
	["~xiansi"] = "选择 1-2 名角色→点击确定",
	["xiansi_slash"] = "陷嗣(杀)",
	["counter"] = "逆",

	["#yufan"] = "狂直之士",
	["yufan"] = "虞翻",
	["designer:yufan"] = "幻岛",
	["illustrator:yufan"] = "琛·美弟奇",
	["zongxuan"] = "纵玄",
	[":zongxuan"] = "当你的牌因弃置而置入弃牌堆时，你可以将其中至少一张牌依次置于牌堆顶。",
	["@zongxuan-put"] = "你可以发动“纵玄”",
	["~zongxuan"] = "选择任意数量的牌→点击确定（这些牌将以与你点击顺序相反的顺序置于牌堆顶）",
	["zhiyan"] = "直言",
	[":zhiyan"] = "结束阶段开始时，你可以令一名角色摸一张牌并展示之：若此牌为装备牌，该角色回复1点体力，然后使用之。",
	["zhiyan-invoke"] = "你可以发动“直言”<br/> <b>操作提示</b>: 选择一名角色→点击确定<br/>",

	["#fuhuanghou"] = "孤注一掷",
	["fuhuanghou"] = "伏皇后",
	["illustrator:fuhuanghou"] = "小莘",
	["zhuikong"] = "惴恐",
	[":zhuikong"] = "一名其他角色的回合开始时，若你已受伤且其有手牌，你可以弃一张牌令另一名有手牌的其他角色选择是否与其拼点，若其选择是：拼点赢的角色视为对拼点没赢的角色使用【杀】，且若当前回合角色赢，此【杀】可选择你为额外目标。",
	["@zhuikong"] = "你可以弃置一张牌发动“惴恐”<br/> <b>操作提示</b>: 选择除当前回合角色外的一名其他角色→点击确定<br/>",
	["zhuikong-pindian:pindian"] = "你可以选择是否与当前回合角色拼点",
	["zhuikong-extra:extra"] = "你可以选择伏皇后为【杀】的额外目标",
	["qiuyuan"] = "求援",
	[":qiuyuan"] = "当对你使用的【杀】生效前，你可以请求除使用者外一名有牌的其他角色选择是否交给你一张牌，若其选择是，当此【杀】被抵消时其摸两张牌；若其选择否，当此【杀】生效时其失去1点体力。",
	["qiuyuan-invoke"] = "你可以发动“求援”<br/> <b>操作提示</b>: 选择除此【杀】使用者外的一名其他角色→点击确定<br/>",
	["@qiuyuan-give"] = "请选择是否交给 %src 一张手牌",

	["#liru"] = "魔仕",
	["liru"] = "李儒",
	["illustrator:liru"] = "MSNZero",
	["juece"] = "绝策",
	[":juece"] = "结束阶段开始时，你可以对一名没有手牌的角色造成1点伤害。",
	["@juece"] = "你可以发动“绝策”<br/> <b>操作提示</b>: 选择一名没有手牌的角色→点击确定<br/>",
	["mieji"] = "灭计",
	[":mieji"] = "阶段技。你可以将一张黑色锦囊牌置于牌堆顶并选择一名有手牌的其他角色，该角色弃置一张锦囊牌，否则弃置两张非锦囊牌。",
	["@mieji-trick"] = "请弃置一张锦囊牌",
	["@mieji-nontrick"] = "请弃置两张非锦囊牌",
	["fencheng"] = "焚城",
	[":fencheng"] = "限定技。出牌阶段，你可以令X=0，然后所有其他角色选择一项：弃置至少X+1张牌，然后X变为弃牌数；或受到2点火焰伤害，然后X变为0。",
	["@fencheng"] = "请弃置至少 %arg 张牌（包括装备区的牌）",
	["$FenchengAnimate"] = "image=image/animate/fencheng.png",
	
	["#caozhen"] = "荷国天督",
	["caozhen"] = "曹真",
	["illustrator:caozhen"] = "Thinking",
	["designer:caozhen"] = "世外高v狼",
	["sidi"] = "司敌",
	[":sidi"] = "每当你使用或其他角色在你的回合内使用或打出【闪】时，你可以将牌堆顶的一张牌置于武将牌上。一名其他角色的出牌阶段开始时，你可以将一张“司敌牌”置入弃牌堆，然后该角色本回合使用【杀】的次数限制-1。",
	["sidi_remove:remove"] = "你可以将一张“司敌牌”置入弃牌堆令当前回合角色本回合使用【杀】的次数限制-1",

	["#guyong"] = "庙堂的玉磬",
	["guyong"] = "顾雍",
	["illustrator:guyong"] = "大佬荣",
	["designer:guyong"] = "睿笛终落",
	["shenxing"] = "慎行",
	[":shenxing"] = "出牌阶段，你可以弃置两张牌：若如此做，你摸一张牌。",
	["bingyi"] = "秉壹",
	[":bingyi"] = "结束阶段开始时，若你有手牌，你可以展示所有手牌：若均为同一颜色，你可以令至多X名角色各摸一张牌。（X为你的手牌数）",
	["@bingyi-card"] = "你可以展示所有手牌发动“秉壹”",
	["~bingyi"] = "若手牌均为同一颜色，选择至多X名角色→点击确定；否则直接点击确定",

	["#wuyi"] = "建兴鞍辔",
	["wuyi"] = "吴懿",
	["illustrator:wuyi"] = "蚂蚁君",
	["desinger:wuyi"] = "沸治克里夫",
	["benxi"] = "奔袭",
	[":benxi"] = "锁定技。你的回合内，你与其他角色的距离-X。你的回合内，若你与与所有其他角色的距离均为1，其他角色的防具无效，你使用【杀】时可以额外选择一个目标或令一名角色再一次成为此【杀】目标。（X为本回合你已使用的牌数）",
	["#benxi-dist"] = "奔袭",
	["@benxi-add"] = "你可以额外选择一个目标或令一名角色再一次成为此【杀】目标",

	["#zhangsong"] = "怀璧待凤仪",
	["zhangsong"] = "张松",
	["illustrator:zhangsong"] = "尼乐小丑",
	["designer:zhangsong"] = "冷王无双",
	["qiangzhi"] = "强识",
	[":qiangzhi"] = "出牌阶段开始时，你可以展示一名其他角色的一张手牌：若如此做，每当你于此阶段内使用与此牌类别相同的牌时，你可以摸一张牌。",
	["qiangzhi-invoke"] = "你可以发动“强识”<br/> <b>操作提示</b>: 选择一名有手牌的其他角色→点击确定<br/>",
	["xiantu"] = "献图",
	[":xiantu"] = "一名其他角色的出牌阶段开始时，你可以摸两张牌：若如此做，你交给其两张牌；且本阶段结束后，若该角色未于本阶段杀死过一名角色，你失去1点体力。",
	["@xiantu-give"] = "请交给 %dest %arg 张牌",
	["#Xiantu"] = "%from 未于本阶段杀死过角色，%to 的“%arg”被触发",

	["#zhuhuan"] = "中洲拒天人",
	["zhuhuan"] = "朱桓",
	["illustrator:zhuhuan"] = "XXX",
	["designer:zhuhuan"] = "半缘修道",
	["youdi"] = "诱敌",
	[":youdi"] = "结束阶段开始时，你可以令一名其他角色弃置你一张牌：若此牌不为【杀】，你获得其的一张牌或将其装备区里的一张牌置入你的装备区。",
	["youdi-invoke"] = "你可以发动“诱敌”<br> <b>操作提示</b>：选择一名其他角色→点击确定<br/>",
	["youdi_obtain"] = "诱敌获得牌",
	
	["#gongsunyuan"] = "狡徒悬海",
	["gongsunyuan"] = "公孙渊",
	["huaiyi"] = "怀异",
	[":huaiyi"] = "阶段技。你可以展示所有牌，若你的牌包含不止一种颜色，则你选择一种颜色并弃置该颜色的所有牌，然后你获得至多X名势力各不相同的其他角色的各一张牌（X为你以此法弃置的手牌数）；若你以此法获得的牌不少于两张，你失去1点体力。",
	["@huaiyi"] = "你可以选择至多 %arg 名势力各不相同的其他角色，获得他们的各一张牌。",
	["~huaiyi"] = "选择角色→点击确定",
}
