-- translation for Thicket Package

return {
	["thicket"] = "林包",

	["#caopi"] = "霸业的继承者",
	["caopi"] = "曹丕",
	["illustrator:caopi"] = "SoniaTang",
	["xingshang"] = "行殇",
	[":xingshang"] = "每当一名其他角色死亡时，你可以获得该角色的牌。",
	["fangzhu"] = "放逐",
	[":fangzhu"] = "每当你受到伤害后，你可以令一名其他角色摸X张牌，然后将其武将牌翻面。（X为你已损失的体力值）",
	["fangzhu-invoke"] = "你可以发动“放逐”<br/> <b>操作提示</b>: 选择一名其他角色→点击确定<br/>",
	["songwei"] = "颂威",
	[":songwei"] = "主公技。其他魏势力角色的黑色判定牌生效后，该角色可以令你摸一张牌。",
	["@songwei-to"] = "请选择“颂威”的目标角色",
	   
	["#caoren"] = "大将军",
	["caoren"] = "曹仁",
	["illustrator:caoren"] = "Ccat",
	["jushou"] = "据守",
	[":jushou"] = "结束阶段开始时，你可以摸一张牌，然后将武将牌翻面。",
	["jiewei"] = "解围",
	[":jiewei"] = "每当你的武将牌翻面后，你可以摸一张牌。若如此做，回合结束时你可以使用一张锦囊牌或装备牌，且此牌结算后，你可以弃置场上一张同类型的牌。",
	["@jiewei"] = "你可以使用一张锦囊牌或装备牌（包括这张在内还可以用 %arg 张）",
	["@jiewei-discard"] = "你可以弃置一名角色场上与你使用的牌同类型的牌",
	["@solve"] = "溃",

	["#menghuo"] = "南蛮王",
	["menghuo"] = "孟获",
	["illustrator:menghuo"] = "废柴男",
	["huoshou"] = "祸首",
	[":huoshou"] = "你可以令【南蛮入侵】对你无效；你使用的【杀】被目标角色使用的【闪】抵消时，你可以将所有手牌（至少一张）当【南蛮入侵】使用。",
	["huoshou:avoid"] = "你可以令【南蛮入侵】对你无效",
	["huoshou:use"] = "你可以将所有手牌（至少一张）当【南蛮入侵】使用",
	["zaiqi"] = "再起",
	[":zaiqi"] = "锁定技。你每于你当前出牌阶段累计造成3点伤害后，你摸X张牌（X为你已损失的体力值），然后回复1点体力。",

	["#zhurong"] = "野性的女王",
	["zhurong"] = "祝融",
	["illustrator:zhurong"] = "废柴男",
	["juxiang"] = "巨象",
	[":juxiang"] = "你可以令【南蛮入侵】对你无效；其他角色使用的未转化的【南蛮入侵】在结算完毕后置入弃牌堆时，你可以获得之。",
	["juxiang:avoid"] = "你可以令【南蛮入侵】对你无效",
	["juxiang:get"] = "你可以获得该【南蛮入侵】",
	["lieren"] = "烈刃",
	[":lieren"] = "每当你使用【杀】对一名角色造成伤害后，你可以与该角色拼点：若你赢，你获得其一张牌。",

	["#sunjian"] = "武烈帝",
	["sunjian"] = "孙坚",
	["illustrator:sunjian"] = "LiuHeng",
	["yinghun"] = "英魂",
	[":yinghun"] = "准备阶段开始时，你可以依次选择以下一至两项：1．扣减1点体力上限；2．令一名其他角色摸或弃置X张牌（X为你已扣减的体力上限值）。",
	["yinghun:lose"] = "你可以发动“英魂”扣减1点体力上限",
	["yinghun-invoke"] = "你可以发动“英魂”选择一名摸牌或弃牌的目标<br/> <b>操作提示</b>: 选择一名其他角色→点击确定<br/>",
	["yinghun:tx"] = "弃置X张牌",
	["yinghun:dx"] = "摸X张牌",
	["polu"] = "破虏",
	[":polu"] = "锁定技。若你未受伤，你使用的【杀】可以额外选择一个目标。",

	["#lusu"] = "独断的外交家",
	["lusu"] = "鲁肃",
	["illustrator:lusu"] = "LiuHeng",
	["haoshi"] = "好施",
	[":haoshi"] = "摸牌阶段开始时，你可以将至多两张牌交给一名手牌数不多于体力值的角色，然后摸等量的牌。",
	["@haoshi"] = "请选择“好施”的目标，将至多两张牌交给该角色",
	["~haoshi"] = "选择至多两张牌→选择一名手牌数不多于体力值的角色→点击确定",
	["dimeng"] = "缔盟",
	[":dimeng"] = "阶段技。你可以弃置至多两张牌，然后选择两名手牌数与势力均不相同的角色：手牌数多的角色摸X张牌，然后将X张手牌交给手牌数少的角色。（X为你弃置的牌数）",
	["~dimeng"] = "点击“缔盟”→选择一名其他吴势力角色→点击确定",
	["@dimeng-card"] = "你可以选择两名手牌数与势力均不相同的角色发动“缔盟”",
	["@dimeng-give"] = "请交给 %dest %arg 张牌",
	
	["#huaxiong"] = "飞扬跋扈",
	["huaxiong"] = "华雄",
	["illustrator:huaxiong"] = "地狱许",
	["yaowu"] = "耀武",
	[":yaowu"] = "每当你受到红色【杀】的伤害时，你可以扣减1点体力上限。若如此做，伤害来源选择一项：回复1点体力，或摸一张牌。",
	["yaowu:recover"] = "回复1点体力",
	["yaowu:draw"] = "摸一张牌",

	["#dongzhuo"] = "魔王",
	["dongzhuo"] = "董卓",
	["illustrator:dongzhuo"] = "小冷",
	["jiuchi"] = "酒池",
	[":jiuchi"] = "你可以将一张黑桃手牌当【酒】使用。",
	["roulin"] = "肉林",
	[":roulin"] = "锁定技。每当你指定女性角色为【杀】的目标后，或成为女性角色的【杀】的目标后，目标角色须连续使用两张【闪】抵消此【杀】。",
	["benghuai"] = "崩坏",
	[":benghuai"] = "锁定技。结束阶段开始时，若你的体力值不为场上最少（或之一），你须选择一项：失去1点体力，或失去1点体力上限。",
	["benghuai:hp"] = "体力",
	["benghuai:maxhp"] = "体力上限",
	["luanshi"] = "乱世",
	[":luanshi"] = "主公技。一名群雄势力角色的出牌阶段限一次。其可以弃一张牌或横置，然后令一名其他角色的所有技能无效直至回合结束。",
	["luanshi_attach"] = "乱世",
	[":luanshi_attach"] = "主公技。一名群雄势力角色的出牌阶段限一次。其可以弃一张牌或横置，然后令一名其他角色的所有技能无效直至回合结束。",
}
