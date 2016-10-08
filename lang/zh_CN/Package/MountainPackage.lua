-- translation for Mountain Package

return {
	["mountain"] = "山包",

	["#zhanghe"] = "料敌机先",
	["zhanghe"] = "张郃",
	["illustrator:zhanghe"] = "张帅",
	["qiaobian"] = "巧变",
	[":qiaobian"] = "除准备阶段和结束阶段的阶段开始前，若你有手牌，你可以进行判定：若结果不为红桃，你弃置一张手牌并跳过该阶段。若以此法跳过摸牌阶段，你可以依次获得一至两名其他角色的各一张手牌；若以此法跳过出牌阶段，你可以将场上的一张牌置于另一名角色相应的区域内。",
	["@qiaobian-2"] = "你可以依次令一至两名其他角色交给你一张手牌",
	["@qiaobian-3"] = "你可以将场上的一张牌移动至另一名角色相应的区域内",
	["#qiaobian-1"] = "请弃置 1 张手牌并跳过判定阶段",
	["#qiaobian-2"] = "请弃置 1 张手牌并跳过摸牌阶段",
	["#qiaobian-3"] = "请弃置 1 张手牌并跳过出牌阶段",
	["#qiaobian-4"] = "请弃置 1 张手牌并跳过弃牌阶段",
	["~qiaobian2"] = "选择 1-2 名其他角色→点击确定",
	["~qiaobian3"] = "选择一名角色→点击确定",
	["@qiaobian-to"] = "请选择移动【%arg】的目标角色",
	["@qiaobian"] = "%src 发动对您发动了“巧变”，请将一张手牌交给 %src",

	["#dengai"] = "矫然的壮士",
	["dengai"] = "邓艾",
	["tuntian"] = "屯田",
	[":tuntian"] = "每当你的牌于回合外被其他角色弃置或获得后，或弃牌阶段结束时（若你于此阶段内弃置过你的至少两张手牌），你可以将牌堆顶一张牌置于武将牌上，称为“田”。你与其他角色的距离-X。（X为“田”的数量）",
	["#tuntian-dist"] = "屯田",
	["field"] = "田",
	["zaoxian"] = "凿险",
	[":zaoxian"] = "觉醒技。准备阶段开始时，若你已受伤且你的“田”大于或等于两张，你失去1点体力上限，然后获得“急袭”（你可以将一张“田”当【顺手牵羊】使用）。",
	["$ZaoxianAnimate"] = "image=image/animate/zaoxian.png",
	["jixi"] = "急袭",
	[":jixi"] = "你可以将一张“田”当【顺手牵羊】使用。",
	["@jixi-target"] = "请选择【顺手牵羊】的目标角色",
	["~jixi"] = "选择【顺手牵羊】的目标角色→点击确定",
	["#ZaoxianWake"] = "%from 的“田”为 %arg 张，触发“%arg2”觉醒",

	["#jiangwei"] = "龙的衣钵",
	["jiangwei"] = "姜维",
	["tiaoxin"] = "挑衅",
	[":tiaoxin"] = "阶段技。你可以令攻击范围内包含你的一名角色对你使用一张【杀】，否则你弃置其一张牌。",
	["@tiaoxin-slash"] = "%src 对你发动“挑衅”，请对其使用一张【杀】",
	["zhiji"] = "志继",
	[":zhiji"] = "觉醒技。准备阶段开始时，若你没有手牌，你失去1点体力上限，然后回复1点体力或摸两张牌，并获得“观星”。",
	["zhiji:draw"] = "摸两张牌",
	["zhiji:recover"] = "回复1点体力",
	["$ZhijiAnimate"] = "image=image/animate/zhiji.png",
	["#ZhijiWake"] = "%from 没有手牌，触发“%arg”觉醒",

	["#liushan"] = "无为的真命主",
	["liushan"] = "刘禅",
	["illustrator:liushan"] = "LiuHeng",
	["xiangle"] = "享乐",
	[":xiangle"] = "锁定技。每当你成为【杀】的目标时，【杀】的使用者须弃置一张基本牌，否则此【杀】对你无效。",
	["@xiangle-discard"] = "你须再弃置一张基本牌使此【杀】生效",
	["fangquan"] = "放权",
	[":fangquan"] = "若你出牌阶段未被跳过且你未使用过除【桃】或【桃园结义】以外的牌，结束阶段开始时你可以弃置一张手牌并选择一名其他角色：若如此做，该角色进行一个额外的回合。",
	["@fangquan-give"] = "你可以弃置一张手牌令一名其他角色进行一个额外的回合",
	["~fangquan"] = "选择一张手牌→选择一名其他角色→点击确定",
	["ruoyu"] = "若愚",
	[":ruoyu"] = "主公技。觉醒技。准备阶段开始时，若你的体力值为场上最少（或之一），你增加1点体力上限，回复1点体力，然后获得“激将”。",
	["$RuoyuAnimate"] = "image=image/animate/ruoyu.png",
	["#Fangquan"] = "%to 将进行一个额外的回合",
	["#RuoyuWake"] = "%from 的体力值 %arg 为场上最少，触发“%arg2”觉醒",
	
	["#zhoutai"] = "历战之驱",
	["zhoutai"] = "周泰",
	["illustrator:zhoutai"] = "Thinking",
	["buqu"] = "不屈",
	[":buqu"] = "锁定技。当你处于濒死状态时，你展示牌堆顶的X张牌（X为你需要回复的体力数）并依次判断：若该牌与你武将牌上的牌点数均不同，将其置于武将牌上，称为“创”，并回复1点体力，否则将其置入弃牌堆。若你有“创”：你的默认手牌上限为“创”数且不超过5；当你失去该技能时，失去与牌数相等的体力值。",
	["wound"] = "创",
	["fenji"] = "奋激",
	[":fenji"] = "每当【过河拆桥】或【顺手牵羊】对一名其他角色生效后，你可以失去1点体力：若如此做，该角色摸两张牌。",

	["#erzhang"] = "经天纬地",
	["erzhang"] = "张昭＆张纮",
	["&erzhang"] = "张昭张纮",
	["illustrator:erzhang"] = "废柴男",
	["zhijian"] = "直谏",
	[":zhijian"] = "出牌阶段，你可以选择一名其他角色。若如此做，你可以展示手牌中的一张装备牌并令其使用之，然后你摸一张牌。",
	["@zhijian"] = "请选择是否对 %src 发动【直谏】<br/> <b>操作提示</b>: 选择一张手牌中的装备牌→点击确定<br/>",
	["guzheng"] = "固政",
	[":guzheng"] = "其他角色的弃牌阶段结束时，你可以将弃牌堆里的一张其于此阶段内因其弃置而失去过的手牌交给该角色交给该角色，若如此做，你可以获得弃牌堆里的其余其于此阶段内因弃置而失去过的手牌中的一张。",

	["#caiwenji"] = "异乡的孤女",
	["caiwenji"] = "蔡文姬",
	["illustrator:caiwenji"] = "SoniaTang",
	["beige"] = "悲歌",
	[":beige"] = "每当一名角色受到一次【杀】的伤害后，你可以弃置一张牌令该角色进行判定：若结果为红桃，该角色回复1点体力；方块，该角色摸两张牌；黑桃，伤害来源将其武将牌翻面；梅花，伤害来源弃置两张牌。",
	["@beige"] = "你可以弃置一张牌发动“悲歌”",
	["duanchang"] = "断肠",
	[":duanchang"] = "锁定技。杀死你的角色失去所有武将技能。",
	["@duanchang"] = "断肠",
	["#DuanchangLoseSkills"] = "%from 的“%arg”被触发， %to 失去所有武将技能",

	["#zuoci"] = "谜之仙人",
	["zuoci"] = "左慈",
	["illustrator:zuoci"] = "废柴男",
	["huashen"] = "化身",
	[":huashen"] ="所有人都展示武将牌后，你随机获得两张未加入游戏的武将牌，选一张置于你面前并声明该武将的一项技能，若如此做，你拥有该技能且同时将性别和势力属性变成与该武将相同直到你选择重新声明技能或失去“化身”。在你的每个回合开始时和结束后，你可以选择是否以此法重新声明一项技能(你不可声明限定技、觉醒技、主公技及其他不可化身的技能)。",
	["xinsheng"] = "新生",
	[":xinsheng"] = "每当你受到1点伤害后，你可以获得一张“化身牌”。",
	["#GetHuashen"] = "%from 获得了 %arg 张“化身牌”，现在共有 %arg2 张“化身牌”",
	["#GetHuashenDetail"] = "%from 获得了“化身牌” %arg",
}
