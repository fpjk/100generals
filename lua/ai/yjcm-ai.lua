function sgs.ai_cardsview.jiushi(self, class_name, player)
	if class_name == "Analeptic" then
		if player:hasSkill("jiushi") and player:faceUp() then
			return ("analeptic:jiushi[no_suit:0]=.")
		end
	end
end

function sgs.ai_skill_invoke.jiushi(self, data)
	return not self.player:faceUp()
end

sgs.ai_skill_invoke.luoying = function(self)
	if self.player:hasFlag("DimengTarget") then
		local another
		for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			if player:hasFlag("DimengTarget") then
				another = player
				break
			end
		end
		if not another or not self:isFriend(another) then return false end
	end
	return not self:needKongcheng(self.player, true)
end

sgs.ai_skill_askforag.luoying = function(self, card_ids)
	if self:needKongcheng(self.player, true) then return card_ids[1] else return -1 end
end

sgs.ai_skill_use["@@jujian"] = function(self, prompt, method)
	local needfriend = 0
	local nobasiccard = -1
	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
	if self:needToThrowArmor() and not self.player:isCardLimited(self.player:getArmor(), method) then
		nobasiccard = self.player:getArmor():getId()
	else
		self:sortByKeepValue(cards)
		for _, card in ipairs(cards) do
			if card:getTypeId() ~= sgs.Card_TypeBasic and not self.player:isCardLimited(card, method) then nobasiccard = card:getEffectiveId() end
		end
	end
	for _, friend in ipairs(self.friends_noself) do
		if self:isWeak(friend) or friend:getHandcardNum() < 2 or not friend:faceUp() or self:getOverflow() > 0
		or (friend:getArmor() and friend:getArmor():objectName() == "Vine" and (friend:isChained() and not self:isGoodChainPartner(friend))) then
			needfriend = needfriend + 1
		end
	end
	if nobasiccard < 0 or needfriend < 1 then return "." end
	self:sort(self.friends_noself,"defense")
	for _, friend in ipairs(self.friends_noself) do
		if not friend:faceUp() then
			return "@JujianCard="..nobasiccard.."->"..friend:objectName()
		end
	end
	for _, friend in ipairs(self.friends_noself) do
		if friend:getArmor() and friend:getArmor():objectName() == "Vine" and (friend:isChained() and not self:isGoodChainPartner(friend)) then
			return "@JujianCard="..nobasiccard.."->"..friend:objectName()
		end
	end
	for _, friend in ipairs(self.friends_noself) do
		if self:isWeak(friend) then
			return "@JujianCard="..nobasiccard.."->"..friend:objectName()
		end
	end

	local AssistTarget = self:AssistTarget()
	local friend
	if AssistTarget and AssistTarget:isWounded() and not self:needToLoseHp(AssistTarget, nil, nil, nil, true) then
		friend = AssistTarget
	elseif AssistTarget and not AssistTarget:hasSkill("manjuan") and not self:needKongcheng(AssistTarget, true) then
		friend = AssistTarget
	else
		friend = self.friends_noself[1]
	end
	return "@JujianCard="..nobasiccard.."->"..friend:objectName()
end

sgs.ai_skill_choice.jujian = function(self, choices)
	if not self.player:faceUp() then return "reset" end
	if self.player:hasArmorEffect("vine") and self.player:isChained() and not self:isGoodChainPartner() then
		return "reset"
	end
	if self:isWeak() and self.player:isWounded() then return "recover" end
	if self.player:hasSkill("manjuan") then
		if self.player:isWounded() then return "recover" end
		if self.player:isChained() then return "reset" end
	end
	return "draw"
end

sgs.ai_card_intention.JujianCard = -100
sgs.ai_use_priority.JujianCard = 4.5

sgs.jujian_keep_value = {
	Peach = 6,
	Jink = 5,
	EquipCard = 5,
	Duel = 5,
	FireAttack = 5,
	ArcheryAttack = 5,
	SavageAssault = 5
}

function sgs.ai_armor_value.yizhong(card)
	if not card then return 4 end
end

local xinzhan_skill={}
xinzhan_skill.name="xinzhan"
table.insert(sgs.ai_skills,xinzhan_skill)
xinzhan_skill.getTurnUseCard=function(self)
	if not self.player:hasUsed("XinzhanCard") and self.player:getHandcardNum() > self.player:getMaxHp() then
		return sgs.Card_Parse("@XinzhanCard=.")
	end
end

sgs.ai_skill_use_func.XinzhanCard=function(card,use,self)
	use.card = card
end

sgs.ai_use_value.XinzhanCard = 4.4
sgs.ai_use_priority.XinzhanCard = 9.4

function sgs.ai_slash_prohibit.huilei(self, from, to)
	if from:hasSkill("jueqing") or (from:hasSkill("nosqianxi") and from:distanceTo(to) == 1) then return false end
	if from:hasFlag("NosJiefanUsed") then return false end
	if self:isFriend(to, from) and self:isWeak(to) then return true end
	return #(self:getEnemies(from)) > 1 and self:isWeak(to) and from:getHandcardNum() > 3
end



sgs.ai_skill_invoke.enyuan = function(self, data)
	local move = data:toMoveOneTime()
	if move and move.from and move.card_ids and move.card_ids:length() > 0 then
		local from = findPlayerByObjectName(self.room, move.from:objectName())
		if from then return self:isFriend(from) and not self:needKongcheng(from, true) end
	end
	local damage = data:toDamage()
	if damage.from and damage.from:isAlive() then
		if self:isFriend(damage.from) then
			if self:getOverflow(damage.from) > 2 then return true end
			if self:needToLoseHp(damage.from, self.player, nil, true) and not self:hasSkills(sgs.masochism_skill, damage.from) then return true end
			if not self:hasLoseHandcardEffective(damage.from) and not damage.from:isKongcheng() then return true end
			return false
		else
			return true
		end
	end
	return
end

sgs.ai_choicemade_filter.skillInvoke.enyuan = function(self, player, promptlist)
	local invoked = (promptlist[3] == "yes")
	local intention = 0

	local EnyuanDrawTarget
	for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
		if p:hasFlag("EnyuanDrawTarget") then EnyuanDrawTarget = p break end
	end

	if EnyuanDrawTarget then
		if not invoked and not self:needKongcheng(EnyuanDrawTarget, true) then
			intention = 10
		elseif not self:needKongcheng(from, true) then
			intention = -10
		end
		sgs.updateIntention(player, EnyuanDrawTarget, intention)
	else
		local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
		if damage.from then
			if not invoked then
				intention = -10
			elseif self:needToLoseHp(damage.from, player, nil, true) then
				intention = 0
			elseif not self:hasLoseHandcardEffective(damage.from) and not damage.from:isKongcheng() then
				intention = 0
			elseif self:getOverflow(damage.from) <= 2 then
				intention = 10
			end
			sgs.updateIntention(player, damage.from, intention)
		end
	end

end

sgs.ai_skill_discard.enyuan = function(self, discard_num, min_num, optional, include_equip)
	local to_discard = {}
	local cards = self.player:getHandcards()
	local fazheng = self.room:findPlayerBySkillName("enyuan")
	cards = sgs.QList2Table(cards)
	if self:needToLoseHp(self.player, fazheng, nil, true) and not self:hasSkills(sgs.masochism_skill) then return {} end
	if self:isFriend(fazheng) then
		for _, card in ipairs(cards) do
			if isCard("Peach", card, fazheng) and ((not self:isWeak() and self:getCardsNum("Peach") > 0) or self:getCardsNum("Peach") > 1) then
				table.insert(to_discard, card:getEffectiveId())
				return to_discard
			end
			if isCard("Analeptic", card, fazheng) and self:getCardsNum("Analeptic") > 1 then
				table.insert(to_discard, card:getEffectiveId())
				return to_discard
			end
			if isCard("Jink", card, fazheng) and self:getCardsNum("Jink") > 1 then
				table.insert(to_discard, card:getEffectiveId())
				return to_discard
			end
		end
	end

	if self:needToLoseHp() and not self:hasSkills(sgs.masochism_skill) then return {} end
	self:sortByKeepValue(cards)
	for _, card in ipairs(cards) do
		if not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player) then
			table.insert(to_discard, card:getEffectiveId())
			return to_discard
		end
	end

	return {}
end

function sgs.ai_slash_prohibit.enyuan(self, from, to)
	if self:isFriend(to, from) then return false end
	if from:hasSkill("jueqing") then return false end
	if from:hasSkill("nosqianxi") and from:distanceTo(to) == 1 then return false end
	if from:hasFlag("nosjiefanUsed") then return false end
	if self:needToLoseHp(from) and not self:hasSkills(sgs.masochism_skill, from) then return false end
	local num = from:getHandcardNum()
	if num >= 3 or from:hasSkills("lianying|noslianying|shangshi|nosshangshi") or (self:needKongcheng(from, true) and num == 2) then return false end
	local role = from:objectName() == self.player:objectName() and from:getRole() or sgs.ai_role[from:objectName()]
	if (role == "loyalist" or role == "lord") and sgs.current_mode_players.rebel + sgs.current_mode_players.renegade == 1
		and to:getHp() == 1 and getCardsNum("Peach", to, from) < 1 and getCardsNum("Analeptic", to, from) < 1
		and (from:getHp() > 1 or getCardsNum("Peach", from, from) >= 1 and getCardsNum("Analeptic", from, from) >= 1) then
		return false
	end
	if role == "rebel" and isLord(to) and self:getAllPeachNum(player) < 1 and to:getHp() == 1
		and (from:getHp() > 1 or getCardsNum("Peach", from, from) >= 1 and getCardsNum("Analeptic", from, from) >= 1) then
		return false
	end
	if role == "renegade" and from:aliveCount() == 2 and to:getHp() == 1 and getCardsNum("Peach", to, from) < 1 and getCardsNum("Analeptic", to, from) < 1
		and (from:getHp() > 1 or getCardsNum("Peach", from, from) >= 1 and getCardsNum("Analeptic", from, from) >= 1) then
		return false
	end
	return #self.enemies > 1
end

sgs.ai_need_damaged.enyuan = function (self, attacker, player)
	if not player:hasSkill("enyuan") then return false end
	if not attacker then return end
	if self:isEnemy(attacker, player) and self:isWeak(attacker) and attacker:getHandcardNum() < 3
	  and not self:hasSkills("lianying|noslianying|shangshi|nosshangshi", attacker)
	  and not (attacker:hasSkill("kongcheng") and attacker:getHandcardNum() > 0)
	  and not (self:needToLoseHp(attacker) and not self:hasSkills(sgs.masochism_skill, attacker)) then
		return true
	end
	return false
end

function sgs.ai_cardneed.enyuan(to, card, self)
	return getKnownCard(to, self.player, "Card", false) < 2
end

sgs.ai_skill_playerchosen.xuanhuo = function(self, targets)
	local lord = self.room:getLord()
	self:sort(self.enemies, "defense")
	if lord and self:isEnemy(lord) then  --killloyal
		for _, enemy in ipairs(self.enemies) do
			if (self:getDangerousCard(lord) or self:getValuableCard(lord))
				and not self:hasSkills(sgs.lose_equip_skill, enemy) and not enemy:hasSkills("tuntian+zaoxian")
				and lord:canSlash(enemy) and (enemy:getHp() < 2 and not hasBuquEffect(enemy))
				and sgs.getDefense(enemy) < 2 then

				return lord
			end
		end
	end

	for _, enemy in ipairs(self.enemies) do --robequip
		for _, enemy2 in ipairs(self.enemies) do
			if enemy:canSlash(enemy2) and (self:getDangerousCard(enemy) or self:getValuableCard(enemy))
				and not self:hasSkills(sgs.lose_equip_skill, enemy) and not (enemy:hasSkill("tuntian") and enemy:hasSkill("zaoxian"))
				and not self:needLeiji(enemy2, enemy) and not self:getDamagedEffects(enemy2, enemy)
				and not self:needToLoseHp(enemy2, enemy, nil, true)
				or (enemy:hasSkill("manjuan") and enemy:getCards("he"):length() > 1 and getCardsNum("Slash", enemy) == 0) then

				return enemy
			end
		end
	end

	if #self.friends_noself == 0 then return nil end
	self:sort(self.friends_noself, "defense")

	for _, friend in ipairs(self.friends_noself) do
		if self:hasSkills(sgs.lose_equip_skill, friend) and not friend:getEquips():isEmpty() and not friend:hasSkill("manjuan") then
			return friend
		end
	end
	for _, friend in ipairs(self.friends_noself) do
		if friend:hasSkills("tuntian+zaoxian") and not friend:hasSkill("manjuan") then
			return friend
		end
	end
	for _, friend in ipairs(self.friends_noself) do
		for _, enemy in ipairs(self.enemies) do
			if friend:canSlash(enemy) and (enemy:getHp() < 2 and not hasBuquEffect(enemy))
				and sgs.getDefense(enemy) < 2 and not friend:hasSkill("manjuan") then
				return friend
			end
		end
	end
	if not self.player:hasSkill("enyuan") then return nil end
	for _, friend in ipairs(self.friends_noself) do
		if not friend:hasSkill("manjuan") then
			return friend
		end
	end
	return nil
end

sgs.ai_skill_playerchosen.xuanhuo_slash = sgs.ai_skill_playerchosen.zero_card_as_slash
sgs.ai_playerchosen_intention.xuanhuo_slash = 80

sgs.ai_skill_cardask["xuanhuo-slash"] = function(self, data, pattern, t1, t2, prompt)
	local parsedPrompt = prompt:split(":")
	local fazheng, victim
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:objectName() == parsedPrompt[2] then fazheng = p end
		if p:objectName() == parsedPrompt[3] then victim = p end
	end
	if not fazheng or not victim then self.room:writeToConsole(debug.traceback()) return "." end
	if fazheng and victim then
		for _, slash in ipairs(self:getCards("Slash")) do
			if self:isFriend(victim) and self:slashIsEffective(slash, victim) then
				if self:needLeiji(victim, self.player) then return slash:toString() end
				if self:getDamagedEffects(victim, self.player) then return slash:toString() end
				if not self:isFriend(fazheng) and self:needToLoseHp(victim, self.player) then return slash:toString() end
			end

			if self:isFriend(victim) and not self:isFriend(fazheng) and not self:slashIsEffective(slash, victim) then
				return slash:toString()
			end

			if self:isEnemy(victim) and self:slashIsEffective(slash, victim)
				and not self:getDamagedEffects(victim, self.player, true) and not self:needLeiji(victim, self.player) then
					return slash:toString()
			end
		end

		if self:hasSkills(sgs.lose_equip_skill) and not self.player:getEquips():isEmpty() and not self.player:hasSkill("manjuan") then return "." end

		for _, slash in ipairs(self:getCards("Slash")) do
			if self:isFriend(victim) and not self:isFriend(fazheng) then
				if (victim:getHp() > 3 or not self:canHit(victim, self.player, self:hasHeavySlashDamage(self.player, slash, victim)))
					and victim:getRole() ~= "lord" then
						return slash:toString()
				end
				if self:needToLoseHp(victim, self.player) then return slash:toString() end
			end

			if not self:isFriend(victim) and not self:isFriend(fazheng) then
				if not self:needLeiji(victim, self.player) then return slash:toString() end
				if not self:slashIsEffective(slash, victim) then return slash:toString() end
			end
		end
	end
	return "."
end

sgs.ai_skill_invoke.xuanfeng = function(self, data)
	for _, enemy in ipairs(self.enemies) do
		if (not self:doNotDiscard(enemy) or self:getDangerousCard(enemy) or self:getValuableCard(enemy)) and not enemy:isNude() and
		not (enemy:hasSkill("guzheng") and self.room:getCurrent():getPhase() == sgs.Player_Discard) then
			return true
		end
	end
	for _, friend in ipairs(self.friends) do
		if(self:hasSkills(sgs.lose_equip_skill, friend) and not friend:getEquips():isEmpty())
		or (self:needToThrowArmor(friend) and friend:getArmor()) or self:doNotDiscard(friend) then
			return true
		end
	end
	return false
end

sgs.ai_skill_playerchosen.xuanfeng = function(self, targets)
	targets = sgs.QList2Table(targets)
	self:sort(targets,"defense")
	for _, enemy in ipairs(self.enemies) do
		if (not self:doNotDiscard(enemy) or self:getDangerousCard(enemy) or self:getValuableCard(enemy)) and not enemy:isNude() and
		not (enemy:hasSkill("guzheng") and self.room:getCurrent():getPhase() == sgs.Player_Discard) then
			return enemy
		end
	end
	for _, friend in ipairs(self.friends) do
		if(self:hasSkills(sgs.lose_equip_skill, friend) and not friend:getEquips():isEmpty())
		or (self:needToThrowArmor(friend) and friend:getArmor()) or self:doNotDiscard(friend) then
			return friend
		end
	end
end

sgs.ai_skill_cardchosen.xuanfeng = function(self, who, flags)
	local cards = sgs.QList2Table(who:getEquips())
	local handcards = sgs.QList2Table(who:getHandcards())
	if #handcards < 3 or handcards[1]:hasFlag("visible") then table.insert(cards,handcards[1]) end

	for i=1,#cards,1 do
			return cards[i]
	end
	return nil
end
sgs.ai_choicemade_filter.cardChosen.xuanfeng = sgs.ai_choicemade_filter.cardChosen.dismantlement

sgs.xuanfeng_keep_value = sgs.xiaoji_keep_value

sgs.ai_cardneed.xuanfeng = sgs.ai_cardneed.equip

sgs.ai_skill_invoke.pojun = function(self, data)
	local damage = data:toDamage()

	if not damage.to:faceUp() then
		return self:isFriend(damage.to)
	end

	local good = damage.to:getHp() > 2
	if self:isFriend(damage.to) then
		return good
	elseif self:isEnemy(damage.to) then
		return not good
	end
end

sgs.ai_choicemade_filter.skillInvoke.pojun = function(self, player, promptlist)
	local intention = 60
	local index = promptlist[#promptlist] == "yes" and 1 or -1
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if damage.from and damage.to then
		if not damage.to:faceUp() then
			intention = index * intention
		elseif damage.to:getHp() > 2 then
			intention = -index / 2 * intention
		elseif index == -1 then
			intention = -20
		end
		sgs.updateIntention(damage.from, damage.to, intention)
	end
end

ganlu_skill = {}
ganlu_skill.name = "ganlu"
table.insert(sgs.ai_skills, ganlu_skill)
ganlu_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("GanluCard") then
		return sgs.Card_Parse("@GanluCard=.")
	end
end

sgs.ai_skill_use_func.GanluCard = function(card, use, self)
	local lost_hp = self.player:getLostHp()
	local target, min_friend, max_enemy

	local compare_func = function(a, b)
		return a:getEquips():length() > b:getEquips():length()
	end
	table.sort(self.enemies, compare_func)
	table.sort(self.friends, compare_func)

	self.friends = sgs.reverse(self.friends)

	for _, friend in ipairs(self.friends) do
		for _, enemy in ipairs(self.enemies) do
			if not self:hasSkills(sgs.lose_equip_skill, enemy) and not enemy:hasSkills("tuntian+zaoxian") then
				local ee = enemy:getEquips():length()
				local fe = friend:getEquips():length()
				local value = self:evaluateArmor(enemy:getArmor(),friend) - self:evaluateArmor(friend:getArmor(),enemy)
					- self:evaluateArmor(friend:getArmor(),friend) + self:evaluateArmor(enemy:getArmor(),enemy)
				if math.abs(ee - fe) <= lost_hp and ee > 0 and (ee > fe or ee == fe and value>0) then
					if self:hasSkills(sgs.lose_equip_skill, friend) then
						use.card = sgs.Card_Parse("@GanluCard=.")
						if use.to then
							use.to:append(friend)
							use.to:append(enemy)
						end
						return
					elseif not min_friend and not max_enemy then
						min_friend = friend
						max_enemy = enemy
					end
				end
			end
		end
	end
	if min_friend and max_enemy then
		use.card = sgs.Card_Parse("@GanluCard=.")
		if use.to then
			use.to:append(min_friend)
			use.to:append(max_enemy)
		end
		return
	end

	target = nil
	for _, friend in ipairs(self.friends) do
		if self:needToThrowArmor(friend) or ((self:hasSkills(sgs.lose_equip_skill, friend)
											or (friend:hasSkills("tuntian+zaoxian") and friend:getPhase() == sgs.Player_NotActive))
			and not friend:getEquips():isEmpty()) then
				target = friend
				break
		end
	end
	if not target then return end
	for _,friend in ipairs(self.friends) do
		if friend:objectName() ~= target:objectName() and math.abs(friend:getEquips():length() - target:getEquips():length()) <= lost_hp then
			use.card = sgs.Card_Parse("@GanluCard=.")
			if use.to then
				use.to:append(friend)
				use.to:append(target)
			end
			return
		end
	end
end

sgs.ai_use_priority.GanluCard = sgs.ai_use_priority.Dismantlement + 0.1
sgs.dynamic_value.control_card.GanluCard = true

sgs.ai_card_intention.GanluCard = function(self,card, from, to)
	local compare_func = function(a, b)
		return a:getEquips():length() < b:getEquips():length()
	end
	table.sort(to, compare_func)
	for i = 1, 2, 1 do
		if to[i]:hasArmorEffect("silver_lion") then
			sgs.updateIntention(from, to[i], -20)
			break
		end
	end
	if to[1]:getEquips():length() < to[2]:getEquips():length() then
		sgs.updateIntention(from, to[1], -80)
	end
end

sgs.ai_skill_invoke.buyi = function(self, data)
	local dying = data:toDying()
	local isFriend = false
	local allBasicCard = true
	if dying.who:isKongcheng() then return false end

	isFriend = not self:isEnemy(dying.who)
	if not sgs.GetConfig("EnableHegemony", false) then
		if self.role == "renegade" then
			if self.room:getMode() == "couple" then --“夫妻协战”模式，考虑对孙坚（内奸）发动
				--waiting for more details
			elseif not (dying.who:isLord() or dying.who:objectName() == self.player:objectName()) then
				if (sgs.current_mode_players["loyalist"] + 1 == sgs.current_mode_players["rebel"]
				or sgs.current_mode_players["loyalist"] == sgs.current_mode_players["rebel"]
				or self.room:getCurrent():objectName() == self.player:objectName()) then
					isFriend = false
				end
			end
		end
	end

	local knownNum = 0
	local cards = dying.who:getHandcards()
	for _, card in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s","visible", self.player:objectName(), dying.who:objectName())
		if dying.who:objectName() == self.player:objectName() or card:hasFlag("visible") or card:hasFlag(flag) then
			knownNum = knownNum + 1
			if card:getTypeId() ~= sgs.Card_TypeBasic then allBasicCard = false end
		end
	end
	if knownNum < dying.who:getHandcardNum() then allBasicCard = false end

	return isFriend and not allBasicCard
end

sgs.ai_choicemade_filter.skillInvoke.buyi = function(self, player, promptlist)
	local dying = self.room:getCurrentDyingPlayer()
	if promptlist[#promptlist] == "yes" then
		if dying and dying:objectName() ~= self.player:objectName() then sgs.updateIntention(player, dying, -80) end
	elseif promptlist[#promptlist] == "no" then
		if not dying or dying:isKongcheng() or dying:objectName() == self.player:objectName() then return end
		local allBasicCard = true
		local knownNum = 0
		local cards = dying:getHandcards()
		for _, card in sgs.qlist(cards) do
			local flag = string.format("%s_%s_%s","visible", player:objectName(), dying:objectName())
			if card:hasFlag("visible") or card:hasFlag(flag) then
				knownNum = knownNum + 1
				if card:getTypeId() ~= sgs.Card_TypeBasic then allBasicCard = false end
			end
		end
		if knownNum < dying:getHandcardNum() then allBasicCard = false end
		if not allBasicCard then sgs.updateIntention(player, dying, 80) end
	end
end

sgs.ai_cardshow.buyi = function(self, requestor)
	assert(self.player:objectName() == requestor:objectName())

	local cards = self.player:getHandcards()
	for _, card in sgs.qlist(cards) do
		if card:getTypeId() ~= sgs.Card_TypeBasic then
			return card
		end
	end

	return self.player:getRandomHandCard()
end

mingce_skill = {}
mingce_skill.name = "mingce"
table.insert(sgs.ai_skills, mingce_skill)
mingce_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("MingceCard") then return end

	local card
	if self:needToThrowArmor() then
		card = self.player:getArmor()
	end
	if not card then
		local hcards = self.player:getCards("h")
		hcards = sgs.QList2Table(hcards)
		self:sortByUseValue(hcards, true)

		for _, hcard in ipairs(hcards) do
			if hcard:isKindOf("Slash") then
				if self:getCardsNum("Slash") > 1 then
					card = hcard
					break
				else
					local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
					self:useBasicCard(hcard, dummy_use)
					if dummy_use and dummy_use.to and (dummy_use.to:length() == 0
							or dummy_use.to:length() == 1 and not self:hasHeavySlashDamage(self.player, hcard, dummy_use.to:first())) then
						card = hcard
						break
					end
				end
			elseif hcard:isKindOf("EquipCard") then
				card = hcard
				break
			end
		end
	end
	if not card then
		local ecards = self.player:getCards("e")
		ecards = sgs.QList2Table(ecards)

		for _, ecard in ipairs(ecards) do
			if ecard:isKindOf("Weapon") or ecard:isKindOf("OffensiveHorse") then
				card = ecard
				break
			end
		end
	end
	if card then
		card = sgs.Card_Parse("@MingceCard=" .. card:getEffectiveId())
		return card
	end

	return nil
end

sgs.ai_skill_use_func.MingceCard = function(card, use, self)
	local target
	local friends = self.friends_noself
	local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
	self.MingceTarget = nil

	local canMingceTo = function(player)
		local canGive = not self:needKongcheng(player, true)
		return canGive or (not canGive and self:getEnemyNumBySeat(self.player,player) == 0)
	end

	self:sort(self.enemies, "defense")
	for _, friend in ipairs(friends) do
		if canMingceTo(friend) then
			for _, enemy in ipairs(self.enemies) do
				if friend:canSlash(enemy) and not self:slashProhibit(slash, enemy) and sgs.getDefenseSlash(enemy, self) <= 2
						and self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self)
						and enemy:objectName() ~= self.player:objectName() then
					target = friend
					self.MingceTarget = enemy
					break
				end
			end
		end
		if target then break end
	end

	if not target then
		self:sort(friends, "defense")
		for _, friend in ipairs(friends) do
			if canMingceTo(friend) then
				target = friend
				break
			end
		end
	end

	if target then
		use.card = card
		if use.to then
			use.to:append(target)
		end
	end
end

sgs.ai_skill_choice.mingce = function(self, choices)
	local chengong = self.room:getCurrent()
	if not self:isFriend(chengong) then return "draw" end
	for _, player in sgs.qlist(self.room:getAlivePlayers()) do
		if player:hasFlag("MingceTarget") and not self:isFriend(player) then
			local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
			if not self:slashProhibit(slash, player) then return "use" end
		end
	end
	return "draw"
end

sgs.ai_skill_playerchosen.mingce = function(self, targets)
	if self.MingceTarget then return self.MingceTarget end
	return sgs.ai_skill_playerchosen.zero_card_as_slash(self, targets)
end

-- sgs.ai_playerchosen_intention.mingce = 80

sgs.ai_use_value.MingceCard = 5.9
sgs.ai_use_priority.MingceCard = 4

sgs.ai_card_intention.MingceCard = -70

sgs.ai_cardneed.mingce = sgs.ai_cardneed.equip

local xianzhen_skill = {}
xianzhen_skill.name = "xianzhen"
table.insert(sgs.ai_skills, xianzhen_skill)
xianzhen_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("XianzhenCard") or self.player:isKongcheng() then return end
	return sgs.Card_Parse("@XianzhenCard=.")
end

sgs.ai_skill_use_func.XianzhenCard = function(card, use, self)
	self:sort(self.enemies, "handcard")
	local max_card = self:getMaxCard()
	local max_point = max_card:getNumber()
	local slashcount = self:getCardsNum("Slash")
	if max_card:isKindOf("Slash") then slashcount = slashcount - 1 end

	if slashcount > 0  then
		for _, enemy in ipairs(self.enemies) do
			if enemy:hasFlag("AI_HuangtianPindian") and enemy:getHandcardNum() == 1 then
				self.xianzhen_card = max_card:getId()
				use.card = sgs.Card_Parse("@XianzhenCard=.")
				if use.to then
					use.to:append(enemy)
					enemy:setFlags("-AI_HuangtianPindian")
				end
				return
			end
		end

		local slash = self:getCard("Slash")
		assert(slash)
		local dummy_use = {isDummy = true}
		self:useBasicCard(slash, dummy_use)

		for _, enemy in ipairs(self.enemies) do
			if not (enemy:hasSkill("kongcheng") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() and self:canAttack(enemy, self.player)
				and not self:canLiuli(enemy, self.friends_noself) and not self:findLeijiTarget(enemy, 50, self.player) then
				local enemy_max_card = self:getMaxCard(enemy)
				local enemy_max_point =enemy_max_card and enemy_max_card:getNumber() or 100
				if max_point > enemy_max_point then
					self.xianzhen_card = max_card:getId()
					use.card = sgs.Card_Parse("@XianzhenCard=.")
					if use.to then use.to:append(enemy) end
					return
				end
			end
		end
		for _, enemy in ipairs(self.enemies) do
			if not (enemy:hasSkill("kongcheng") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() and self:canAttack(enemy, self.player)
				and not self:canLiuli(enemy, self.friends_noself) and not self:findLeijiTarget(enemy, 50, self.player) then
				if max_point >= 10 then
					self.xianzhen_card = max_card:getId()
					use.card = sgs.Card_Parse("@XianzhenCard=.")
					if use.to then use.to:append(enemy) end
					return
				end
			end
		end
	end
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards, true)
	if (self:getUseValue(cards[1]) < 6 and self:getKeepValue(cards[1]) < 6) or self:getOverflow() > 0 then
		for _, enemy in ipairs(self.enemies) do
			if not (enemy:hasSkill("kongcheng") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() and not enemy:hasSkills("tuntian+zaoxian") then
				self.xianzhen_card = cards[1]:getId()
				use.card = sgs.Card_Parse("@XianzhenCard=.")
				if use.to then use.to:append(enemy) end
				return
			end
		end
	end
end

sgs.ai_cardneed.xianzhen = function(to, card, self)
	local cards = to:getHandcards()
	local has_big = false
	for _, c in sgs.qlist(cards) do
		local flag = string.format("%s_%s_%s","visible",self.room:getCurrent():objectName(),to:objectName())
		if c:hasFlag("visible") or c:hasFlag(flag) then
			if c:getNumber()>10 then
				has_big = true
				break
			end
		end
	end
	if not has_big then
		return card:getNumber() > 10
	else
		return card:isKindOf("Slash") or card:isKindOf("Analeptic")
	end
end

function sgs.ai_skill_pindian.xianzhen(minusecard, self, requestor)
	if requestor:getHandcardNum() == 1 then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		return cards[1]
	end
	if requestor:getHandcardNum() <= 2 then return minusecard end
end

sgs.ai_card_intention.XianzhenCard = 70

sgs.dynamic_value.control_card.XianzhenCard = true

sgs.ai_use_value.XianzhenCard = 9.2
sgs.ai_use_priority.XianzhenCard = 9.2

sgs.ai_skill_invoke.shangshi = function(self, data)
	if self.player:getLostHp() == 1 then return sgs.ai_skill_invoke.noslianying(self, data) end
	return true
end

sgs.ai_skill_invoke.quanji = function(self, data)
	local current = self.room:getCurrent()
	local juece_effect = (current and current:isAlive() and current:getPhase() ~= sgs.Player_NotActive and self:isEnemy(current) and current:hasSkill("juece"))
	local manjuan_effect = hasManjuanEffect(self.player)
	if self.player:isKongcheng() then
		if manjuan_effect or juece_effect then return false end
	elseif self.player:getHandcardNum() == 1 then
		if manjuan_effect and juece_effect then return false end
	end
	return true
end

sgs.ai_skill_discard.quanji = function(self)
	local to_discard = {}
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)

	table.insert(to_discard, cards[1]:getEffectiveId())

	return to_discard
end

sgs.ai_skill_choice.zili = function(self, choice)
	if self.player:getHp() < self.player:getMaxHp()-1 then return "recover" end
	return "draw"
end

local paiyi_skill = {}
paiyi_skill.name = "paiyi"
table.insert(sgs.ai_skills, paiyi_skill)
paiyi_skill.getTurnUseCard = function(self)
	if not (self.player:getPile("power"):isEmpty()
		or self.player:hasUsed("PaiyiCard")) then
		return sgs.Card_Parse("@PaiyiCard=" .. self.player:getPile("power"):first())
	end
	return nil
end

sgs.ai_skill_use_func.PaiyiCard = function(card, use, self)
	local target
	self:sort(self.friends_noself, "defense")
	for _, friend in ipairs(self.friends_noself) do
		if friend:getHandcardNum() < 2 and friend:getHandcardNum() + 1 < self.player:getHandcardNum()
		  and not self:needKongcheng(friend, true) and not friend:hasSkill("manjuan") then
			target = friend
		end
		if target then break end
	end
	if not target then
		if self.player:getHandcardNum() < self.player:getHp() + self.player:getPile("power"):length() - 1 then
			target = self.player
		end
	end
	self:sort(self.friends_noself, "hp")
	self.friends_noself = sgs.reverse(self.friends_noself)
	if not target then
		for _, friend in ipairs(self.friends_noself) do
			if friend:getHandcardNum() + 2 > self.player:getHandcardNum()
			  and (self:getDamagedEffects(friend, self.player) or self:needToLoseHp(friend, self.player, nil, true))
			  and not friend:hasSkill("manjuan") then
				target = friend
			end
			if target then break end
		end
	end
	self:sort(self.enemies, "defense")
	if not target then
		for _, enemy in ipairs(self.enemies) do
			if enemy:hasSkill("manjuan")
				and not (self:hasSkills(sgs.masochism_skill, enemy) and not self.player:hasSkill("jueqing"))
				and self:damageIsEffective(enemy, sgs.DamageStruct_Normal, self.player)
				and not (self:getDamagedEffects(enemy, self.player) or self:needToLoseHp(enemy))
				and enemy:getHandcardNum() > self.player:getHandcardNum() then
				target = enemy
			end
			if target then break end
		end
		if not target then
			for _, enemy in ipairs(self.enemies) do
				if not (self:hasSkills(sgs.masochism_skill, enemy) and not self.player:hasSkill("jueqing"))
					and not enemy:hasSkills(sgs.cardneed_skill .. "|jijiu|tianxiang|buyi")
					and self:damageIsEffective(enemy, sgs.DamageStruct_Normal, self.player) and not self:cantbeHurt(enemy)
					and not (self:getDamagedEffects(enemy, self.player) or self:needToLoseHp(enemy))
					and enemy:getHandcardNum() + 2 > self.player:getHandcardNum()
					and not enemy:hasSkill("manjuan") then
					target = enemy
				end
				if target then break end
			end
		end
	end

	if target then
		use.card = sgs.Card_Parse("@PaiyiCard=" .. self.player:getPile("power"):first())
		if use.to then
			use.to:append(target)
		end
	end
end

sgs.ai_skill_askforag.paiyi = function(self, card_ids)
	return card_ids[math.random(1, #card_ids)]
end

sgs.ai_card_intention.PaiyiCard = function(self, card, from, tos)
	local to = tos[1]
	if to:objectName() == from:objectName() then return end
	if not to:hasSkill("manjuan")
		and ((to:getHandcardNum() < 2 and to:getHandcardNum() + 1 < from:getHandcardNum() and not self:needKongcheng(to, true))
			or (to:getHandcardNum() + 2 > from:getHandcardNum() and (self:getDamagedEffects(to, from) or self:needToLoseHp(to, from)))) then
	else
		sgs.updateIntention(from, to, 60)
	end
end

sgs.ai_skill_invoke.qianxi = function(self, data)
	if self.player:hasFlag("AI_doNotInvoke_qianxi") then
		self.player:setFlags("-AI_doNotInvoke_qianxi")
		return
	end
	if self.player:getPile("incantation"):length() > 0 then
		local card = sgs.Sanguosha:getCard(self.player:getPile("incantation"):first())
		if not self.player:getJudgingArea():isEmpty() and not self.player:containsTrick("YanxiaoCard") and not self:hasWizard(self.enemies, true) then
			local trick = self.player:getJudgingArea():last()
			if trick:isKindOf("Indulgence") then
				if card:getSuit() == sgs.Card_Heart or (self.player:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade) then return false end
			elseif trick:isKindOf("SupplyShortage") then
				if card:getSuit() == sgs.Card_Club then return false end
			end
		end
		local zhangbao = self.room:findPlayerBySkillName("yingbing")
		if zhangbao and self:isEnemy(zhangbao) and not zhangbao:hasSkill("manjuan")
			and (card:isRed() or (self.player:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade)) then return false end
	end
	for _, p in ipairs(self.enemies) do
		if self.player:distanceTo(p) == 1 and not p:isKongcheng() then
			return true
		end
	end
	return false
end

sgs.ai_skill_playerchosen.qianxi = function(self, targets)
	local enemies = {}
	local slash = self:getCard("Slash") or sgs.Sanguosha:cloneCard("slash")
	local isRed = (self.player:getTag("qianxi"):toString() == "red")

	for _, target in sgs.qlist(targets) do
		if self:isEnemy(target) and not target:isKongcheng() then
			table.insert(enemies, target)
		end
	end

	if #enemies == 1 then
		return enemies[1]
	else
		self:sort(enemies, "defense")
		if not isRed then
			for _, enemy in ipairs(enemies) do
				if enemy:hasSkill("qingguo") and self:slashIsEffective(slash, enemy) then return enemy end
			end
			for _, enemy in ipairs(enemies) do
				if enemy:hasSkill("kanpo") then return enemy end
			end
		else
			for _, enemy in ipairs(enemies) do
				if getKnownCard(enemy, self.player, "Jink", false, "h") > 0 and self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self) then return enemy end
			end
			for _, enemy in ipairs(enemies) do
				if getKnownCard(enemy, self.player, "Peach", true, "h") > 0 or enemy:hasSkill("jijiu") then return enemy end
			end
			for _, enemy in ipairs(enemies) do
				if getKnownCard(enemy, self.player, "Jink", false, "h") > 0 and self:slashIsEffective(slash, enemy) then return enemy end
			end
		end
		for _, enemy in ipairs(enemies) do
			if enemy:hasSkill("longhun") then return enemy end
		end
		return enemies[1]
	end
	return targets:first()
end

sgs.ai_playerchosen_intention.qianxi = 80

function sgs.ai_cardneed.dangxian(to, card, self)
	return isCard("Slash", card, to) and getKnownCard(to, self.player, "Slash", true) == 0
end

sgs.ai_skill_invoke.zishou = function(self, data)
	if self:needBear() then return true end
	if self.player:isSkipped(sgs.Player_Play) then return true end

	local chance_value = 1
	local peach_num = self:getCardsNum("Peach")
	local can_save_card_num = self:getOverflow(self.player, true) - self.player:getHandcardNum()

	if self.player:getHp() <= 2 and self.player:getHp() < getBestHp(self.player) then chance_value = chance_value + 1 end
	if self.player:hasSkills("nosrende|rende") and self:findFriendsByType(sgs.Friend_Draw) then chance_value = chance_value - 1 end
	if self.player:hasSkill("qingnang") then
		for _, friend in ipairs(self.friends) do
			if friend:isWounded() then chance_value = chance_value - 1 break end
		end
	end
	if self.player:hasSkill("jieyin") then
		for _, friend in ipairs(self.friends) do
			if friend:isWounded() and friend:isMale() then chance_value = chance_value - 1 break end
		end
	end

	return self:ImitateResult_DrawNCards(self.player, self.player:getVisibleSkillList(true)) - can_save_card_num + peach_num  <= chance_value
end

sgs.ai_skill_invoke.fuli = true

function sgs.ai_cardsview.fuhun(self, class_name, player)
	if class_name == "Slash" then
		return cardsView_spear(self, player, "fuhun")
	end
end

local fuhun_skill = {}
fuhun_skill.name = "fuhun"
table.insert(sgs.ai_skills, fuhun_skill)
fuhun_skill.getTurnUseCard = function(self, inclusive)
	return turnUse_spear(self, inclusive, "fuhun")
end

function sgs.ai_skill_invoke.zhenlie(self, data)
	local use = data:toCardUse()
	if not use.from or use.from:isDead() then return false end
	if self.role == "rebel" and sgs.evaluatePlayerRole(use.from) == "rebel" and not use.from:hasSkill("jueqing")
		and self.player:getHp() == 1 and self:getAllPeachNum() < 1 then return false end

	if self:isEnemy(use.from) or (self:isFriend(use.from) and self.role == "loyalist" and not use.from:hasSkill("jueqing") and use.from:isLord() and self.player:getHp() == 1) then
		if use.card:isKindOf("Slash") then
			if not self:slashIsEffective(use.card, self.player, use.from) then return false end
			if self:hasHeavySlashDamage(use.from, use.card, self.player) then return true end

			local jink_num = self:getExpectedJinkNum(use)
			local hasHeart = false
			for _, card in ipairs(self:getCards("Jink")) do
				if card:getSuit() == sgs.Card_Heart then
					hasHeart = true
					break
				end
			end
			if self:getCardsNum("Jink") == 0
				or jink_num == 0
				or self:getCardsNum("Jink") < jink_num
				or (use.from:hasSkill("dahe") and self.player:hasFlag("dahe") and not hasHeart) then

				if use.card:isKindOf("NatureSlash") and self.player:isChained() and not self:isGoodChainTarget(self.player, use.from, nil, nil, use.card) then return true end
				if use.from:hasSkill("nosqianxi") and use.from:distanceTo(self.player) == 1 then return true end
				if self:isFriend(use.from) and self.role == "loyalist" and not use.from:hasSkill("jueqing") and use.from:isLord() and self.player:getHp() == 1 then return true end
				if (not (self:hasSkills(sgs.masochism_skill) or (self.player:hasSkill("tianxiang") and getKnownCard(self.player, self.player, "heart") > 0)) or use.from:hasSkill("jueqing"))
					and not self:doNotDiscard(use.from) then
					return true
				end
			end
		elseif use.card:isKindOf("AOE") then
			local from = use.from
			if use.card:isKindOf("SavageAssault") then
				local menghuo = self.room:findPlayerBySkillName("huoshou")
				if menghuo then from = menghuo end
			end

			local friend_null = 0
			for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if self:isFriend(p) then friend_null = friend_null + getCardsNum("Nullification", p, self.player) end
				if self:isEnemy(p) then friend_null = friend_null - getCardsNum("Nullification", p, self.player) end
			end
			friend_null = friend_null + self:getCardsNum("Nullification")
			local sj_num = self:getCardsNum(use.card:isKindOf("SavageAssault") and "Slash" or "Jink")

			if not self:hasTrickEffective(use.card, self.player, from) then return false end
			if not self:damageIsEffective(self.player, sgs.DamageStruct_Normal, from) then return false end
			if use.from:hasSkill("drwushuang") and self.player:getCardCount() == 1 and self:hasLoseHandcardEffective() then return true end
			if sj_num == 0 and friend_null <= 0 then
				if self:isEnemy(from) and from:hasSkill("jueqing") then return not self:doNotDiscard(from) end
				if self:isFriend(from) and self.role == "loyalist" and from:isLord() and self.player:getHp() == 1 and not from:hasSkill("jueqing") then return true end
				if (not (self:hasSkills(sgs.masochism_skill) or (self.player:hasSkill("tianxiang") and getKnownCard(self.player, self.player, "heart") > 0)) or use.from:hasSkill("jueqing"))
					and not self:doNotDiscard(use.from) then
					return true
				end
			end
		elseif self:isEnemy(use.from) then
			if use.card:isKindOf("FireAttack") and use.from:getHandcardNum() > 0 then
					if not self:hasTrickEffective(use.card, self.player) then return false end
				if not self:damageIsEffective(self.player, sgs.DamageStruct_Fire, use.from) then return false end
				if (self.player:hasArmorEffect("vine") or self.player:getMark("@gale") > 0) and use.from:getHandcardNum() > 3
					and not (use.from:hasSkill("hongyan") and getKnownCard(self.player, self.player, "spade") > 0) then
					return not self:doNotDiscard(use.from)
				elseif self.player:isChained() and not self:isGoodChainTarget(self.player, use.from) then
					return not self:doNotDiscard(use.from)
				end
			elseif (use.card:isKindOf("Snatch") or use.card:isKindOf("Dismantlement"))
					and self:getCardsNum("Peach") == self.player:getHandcardNum() and not self.player:isKongcheng() then
				if not self:hasTrickEffective(use.card, self.player) then return false end
				return not self:doNotDiscard(use.from)
			elseif use.card:isKindOf("Duel") then
				if self:getCardsNum("Slash") == 0 or self:getCardsNum("Slash") < getCardsNum("Slash", use.from, self.player) then
					if not self:hasTrickEffective(use.card, self.player) then return false end
					if not self:damageIsEffective(self.player, sgs.DamageStruct_Normal, use.from) then return false end
					return not self:doNotDiscard(use.from)
				end
			elseif use.card:isKindOf("TrickCard") and not use.card:isKindOf("AmazingGrace") then
				if not self:doNotDiscard(use.from) and self:needToLoseHp(self.player) then
					return true
				end
			end
		end
	end
	return false
end

sgs.ai_skill_choice.miji_draw = function(self, choices)
	return "" .. self.player:getLostHp()
end

sgs.ai_skill_invoke.miji = function(self, data)
	if #self.friends_noself == 0 then return false end
	for _, friend in ipairs(self.friends_noself) do
		if not friend:hasSkill("manjuan") and not self:isLihunTarget(friend) then return true end
	end
	return false
end

sgs.ai_skill_askforyiji.miji = function(self, card_ids)
	local available_friends = {}
	for _, friend in ipairs(self.friends_noself) do
		if not friend:hasSkill("manjuan") and not self:isLihunTarget(friend) then table.insert(available_friends, friend) end
	end

	local toGive, allcards = {}, {}
	local keep
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		if not keep and (isCard("Jink", card, self.player) or isCard("Analeptic", card, self.player)) then
			keep = true
		else
			table.insert(toGive, card)
		end
		table.insert(allcards, card)
	end

	local cards = #toGive > 0 and toGive or allcards
	self:sortByKeepValue(cards, true)
	local id = cards[1]:getId()

	local card, friend = self:getCardNeedPlayer(cards)
	if card and friend and table.contains(available_friends, friend) then return friend, card:getId() end

	if #available_friends > 0 then
		self:sort(available_friends, "handcard")
		for _, afriend in ipairs(available_friends) do
			if not self:needKongcheng(afriend, true) then
				return afriend, id
			end
		end
		self:sort(available_friends, "defense")
		return available_friends[1], id
	end
	return nil, -1
end

function sgs.ai_cardneed.jiangchi(to, card, self)
	return isCard("Slash", card, to) and getKnownCard(to, self.player, "Slash", true) < 2
end

sgs.ai_skill_choice.jiangchi = function(self, choices)
	local target = 0
	local goodtarget = 0
	local slashnum = 0
	local needburst = 0

	for _, slash in ipairs(self:getCards("Slash")) do
		for _,enemy in ipairs(self.enemies) do
			if self:slashIsEffective(slash, enemy) then
				slashnum = slashnum + 1
				break
			end
		end
	end

	for _,enemy in ipairs(self.enemies) do
		for _, slash in ipairs(self:getCards("Slash")) do
			if not self:slashProhibit(slash, enemy) and self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self) then
				goodtarget = goodtarget + 1
				break
			end
		end
	end

	for _,enemy in ipairs(self.enemies) do
		if self.player:canSlash(enemy) then
			target = target + 1
			break
		end
	end

	if slashnum > 1 or (slashnum > 0 and goodtarget > 0) then needburst = 1 end
	self:sort(self.enemies,"defenseSlash")
	local can_save_card_num = self.player:getMaxCards() - self.player:getHandcardNum()
	if target == 0 or can_save_card_num > 1 or self.player:isSkipped(sgs.Player_Play) then return "jiang" end
	if self:needBear() then return "jiang" end

	for _,enemy in ipairs(self.enemies) do
		local def = sgs.getDefense(enemy)
		local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
		local eff = self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self)

		if self:slashProhibit(slash, enemy) then
		elseif eff and def < 8 and needburst > 0 then return "chi"
		end
	end

	return "cancel"
end

local gongqi_skill={}
gongqi_skill.name = "gongqi"
table.insert(sgs.ai_skills, gongqi_skill)
gongqi_skill.getTurnUseCard = function(self,inclusive)
	if self.player:hasUsed("GongqiCard") then return end
	if self:needBear() then return end
	if #self.enemies == 0 then return end
	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
	if self:needToThrowArmor() then
		return sgs.Card_Parse("@GongqiCard=" .. self.player:getArmor():getEffectiveId())
	end

	for _, c in ipairs(cards) do
		if c:isKindOf("Weapon") then return sgs.Card_Parse("@GongqiCard=" .. c:getEffectiveId()) end
	end

	local handcards = self.player:getHandcards()
	handcards = sgs.QList2Table(handcards)
	local has_weapon, has_armor, has_def, has_off = false, false, false, false
	local weapon, armor
	for _, c in ipairs(handcards) do
		if c:isKindOf("Weapon") then
			has_weapon = true
			if not weapon or self:evaluateWeapon(weapon) < self:evaluateWeapon(c) then weapon = c end
		end
		if c:isKindOf("Armor") then
			has_armor = true
			if not armor or self:evaluateArmor(armor) < self:evaluateArmor(c) then armor = c end
		end
		if c:isKindOf("DefensiveHorse") then has_def = true end
		if c:isKindOf("OffensiveHorse") then has_off = true end
	end
	if has_off and self.player:getOffensiveHorse() then return sgs.Card_Parse("@GongqiCard=" .. self.player:getOffensiveHorse():getEffectiveId()) end
	if has_def and self.player:getDefensiveHorse() then return sgs.Card_Parse("@GongqiCard=" .. self.player:getDefensiveHorse():getEffectiveId()) end
	if has_weapon and self.player:getWeapon() and self:evaluateWeapon(self.player:getWeapon()) <= self:evaluateWeapon(weapon) then
		return sgs.Card_Parse("@GongqiCard=" .. self.player:getWeapon():getEffectiveId())
	end
	if has_armor and self.player:getArmor() and self:evaluateArmor(self.player:getArmor()) <= self:evaluateArmor(armor) then
		return sgs.Card_Parse("@GongqiCard=" .. self.player:getArmor():getEffectiveId())
	end

	if self:getOverflow() > 0 and self:getCardsNum("Slash") >= 1 then
		self:sortByKeepValue(handcards)
		self:sort(self.enemies, "defense")
		for _, c in ipairs(handcards) do
			if c:isKindOf("Snatch") or c:isKindOf("Dismantlement") then
				local use = { isDummy = true }
				self:useCardSnatch(c, use)
				if use.card then return end
			elseif isCard("Peach", c, self.player)
				or isCard("ExNihilo", c, self.player)
				or (isCard("Analeptic", c, self.player) and self.player:getHp() <= 2)
				or (isCard("Jink", c, self.player) and self:getCardsNum("Jink") < 2)
				or (isCard("Nullification", c, self.player) and self:getCardsNum("Nullification") < 2)
				or (isCard("Slash", c, self.player) and self:getCardsNum("Slash") == 1) then
				-- do nothing
			elseif not c:isKindOf("EquipCard") and #self.enemies > 0 and self.player:inMyAttackRange(self.enemies[1]) then
			else
				return sgs.Card_Parse("@GongqiCard=" .. c:getEffectiveId())
			end
		end
	end
end

sgs.ai_skill_use_func.GongqiCard = function(card, use, self)
	local id = card:getSubcards():first()
	local subcard = sgs.Sanguosha:getCard(id)
	if subcard:isKindOf("SilverLion") and self.room:getCardPlace(id) == sgs.Player_PlaceHand and self:isWounded() then
		use.card = subcard
		return
	end
	use.card = card
end

sgs.ai_skill_playerchosen.gongqi = function(self, targets)
	local player = self:findPlayerToDiscard()
	return player
end

sgs.ai_use_value.GongqiCard = 2
sgs.ai_use_priority.GongqiCard = 8

local jiefan_skill = {}
jiefan_skill.name = "jiefan"
table.insert(sgs.ai_skills, jiefan_skill)
jiefan_skill.getTurnUseCard = function(self, inclusive)
	if self.player:getMark("@rescue") == 0 then return end
	return sgs.Card_Parse("@JiefanCard=.")
end

sgs.ai_skill_use_func.JiefanCard = function(card, use, self)
	local target
	local use_value = 0
	local max_value = -10000
	local p_count = 0
	for _, friend in ipairs(self.friends) do
		use_value = 0
		local count = 0
		for _, p in sgs.qlist(self.room:getOtherPlayers(friend)) do
			if p:inMyAttackRange(friend) then
				count = count + 1
				if self:isFriend(p) then
					if not friend:hasSkill("manjuan") then use_value = use_value + 1 end
				else
					if p:getWeapon() then
						use_value = use_value + 1.2
					else
						if not friend:hasSkill("manjuan") then use_value = use_value + p:getHandcardNum() / 5 end
					end
				end
			end
		end
		if friend:objectName() == self.player:objectName() then p_count = count end
		use_value = use_value - friend:getHandcardNum() / 2
		if use_value > max_value then
			max_value = use_value
			target = friend
		end
	end

	if target and max_value >= self.player:aliveCount() / 2 then
		use.card = card
		if use.to then use.to:append(target) end
		return
	end

	if self:isWeak() and p_count > 0 then
		use.card = card
		if use.to then use.to:append(self.player) end
		return
	end
end

sgs.ai_skill_cardask["@jiefan-discard"] = function(self, data)
	local player = data:toPlayer()
	if not player or not player:isAlive() or player:hasSkill("manjuan") or self:isFriend(player) then return "." end
	for _, card in sgs.qlist(self.player:getCards("he")) do
		if card:isKindOf("Weapon") and not self.player:hasEquip(card) then
			return "$" .. card:getEffectiveId()
		end
	end

	if not self.player:getWeapon() then return "." end
	local count = 0
	local range_fix = ( sgs.weapon_range[self.player:getWeapon():getClassName()] or 1 ) - self.player:getAttackRange(false)

	for _, p in sgs.qlist(self.room:getAllPlayers()) do
		if self:isEnemy(p) and self.player:distanceTo(p, range_fix) > self.player:getAttackRange() then count = count + 1 end
	end

	if count <= 2 then return "$" .. self.player:getWeapon():getEffectiveId() end
	return "."
end

sgs.ai_card_intention.JiefanCard = -80

function sgs.ai_cardneed.jiefan(to, card, self)
	return isCard("Slash", card, to) and getKnownCard(to, self.player, "Slash", true) == 0
end

anxu_skill = {}
anxu_skill.name = "anxu"
table.insert(sgs.ai_skills, anxu_skill)
anxu_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("AnxuCard") then return nil end
	card = sgs.Card_Parse("@AnxuCard=.")
	return card

end

sgs.ai_skill_use_func.AnxuCard = function(card,use,self)
	if #self.enemies == 0 then return end
	local intention = 50
	local friends = {}
	for _, friend in ipairs(self.friends_noself) do
		if friend:hasSkill("manjuan") then
			if self:needKongcheng(friend, true) then table.insert(friends, friend) end
		elseif not self:needKongcheng(friend, true) then
			table.insert(friends, friend)
		end
	end
	self:sort(friends, "handcard")

	local least_friend, most_friend
	if #friends > 0 then
		least_friend = friends[1]
		most_friend = friends[#friends]
	end

	local need_kongcheng_friend
	for _, friend in ipairs(friends) do
		if friend:getHandcardNum() == 1 and (friend:hasSkill("kongcheng") or (friend:hasSkill("zhiji") and friend:getMark("zhiji") == 0 and friend:getHp() >= 3)) then
			need_kongcheng_friend = friend
			break
		end
	end

	local enemies = {}
	for _, enemy in ipairs(self.enemies) do
		if not enemy:hasSkills("tuntian+zaoxian") and not (enemy:isKongcheng() or (enemy:getHandcardNum() <= 1 and self:needKongcheng(enemy))) then
			table.insert(enemies, enemy)
		end
	end

	self:sort(enemies, "handcard")
	enemies = sgs.reverse(enemies)
	local most_enemy
	if #enemies > 0 then most_enemy = enemies[1] end

	local prior_enemy, kongcheng_enemy, manjuan_enemy
	for _, enemy in ipairs(enemies) do
		if enemy:getHandcardNum() >= 2 and self:hasSkills(sgs.cardneed_skill, enemy) then
			if not prior_enemy then prior_enemy = enemy end
		end
		if enemy:hasSkill("kongcheng") and enemy:isKongcheng() then
			if not kongcheng_enemy then kongcheng_enemy = enemy end
		end
		if enemy:hasSkill("manjuan") then
			if not manjuan_enemy then manjuan_enemy = enemy end
		end
		if prior_enemy and kongcheng_enemy and manjuan_enemy then break end
	end

	-- Enemy -> Friend
	if least_friend then
		local tg_enemy
		if not tg_enemy and prior_enemy and prior_enemy:getHandcardNum() > least_friend:getHandcardNum() then tg_enemy = prior_enemy end
		if not tg_enemy and most_enemy  and most_enemy:getHandcardNum() > least_friend:getHandcardNum() then tg_enemy = most_enemy end
		if tg_enemy  then
			use.card = card
			if use.to then
				use.to:append(tg_enemy)
				use.to:append(least_friend)
			end
			if not use.isDummy then
				sgs.updateIntention(self.player, tg_enemy, intention)
				sgs.updateIntention(self.player, least_friend, -intention)
			end
			return
		end

		if most_enemy and most_enemy:getHandcardNum() > least_friend:getHandcardNum() then
			use.card = card
			if use.to then
				use.to:append(most_enemy)
				use.to:append(least_friend)
			end
			if not use.isDummy then
				sgs.updateIntention(self.player, most_enemy, intention)
				sgs.updateIntention(self.player, least_friend, -intention)
			end
			return
		end

	end


	self:sort(enemies,"defense")
	if least_friend then
		for _,enemy in ipairs(enemies) do
			local hand1 = enemy:getHandcardNum()
			local hand2 = least_friend:getHandcardNum()

			if (hand1 > hand2) then
				use.card=card
				if use.to then
					use.to:append(enemy)
					use.to:append(least_friend)
					return
				end
			end
		end
	end



	self:sort(enemies, "handcard", true)
	-- Friend -> Friend
	if #friends >= 2 then
		if need_kongcheng_friend and least_friend:isKongcheng() then
			use.card = card
			if use.to then
				use.to:append(need_kongcheng_friend)
				use.to:append(least_friend)
			end
			if not use.isDummy then
				sgs.updateIntention(self.player, need_kongcheng_friend, -intention)
				sgs.updateIntention(self.player, least_friend, -intention)
			end
			return
		elseif most_friend:getHandcardNum() >= 4 and most_friend:getHandcardNum() > least_friend:getHandcardNum() then
			use.card = card
			if use.to then
				use.to:append(most_friend)
				use.to:append(least_friend)
			end
			if not use.isDummy then sgs.updateIntention(self.player, least_friend, -intention) end
			return
		end
	end

	-- Enemy -> Enemy
	if kongcheng_enemy and not kongcheng_enemy:hasSkill("manjuan") then
		local tg_enemy = prior_enemy or most_enemy
		if tg_enemy and not tg_enemy:isKongcheng() then
			use.card = card
			if use.to then
				use.to:append(tg_enemy)
				use.to:append(kongcheng_enemy)
			end
			if not use.isDummy then
				sgs.updateIntention(self.player, tg_enemy, intention)
				sgs.updateIntention(self.player, kongcheng_enemy, intention)
			end
			return
		elseif most_friend and most_friend:getHandcardNum() >= 4 then -- Friend -> Enemy for KongCheng
			use.card = card
			if use.to then
				use.to:append(most_friend)
				use.to:append(kongcheng_enemy)
			end
			if not use.isDummy then sgs.updateIntention(self.player, kongcheng_enemy, intention) end
			return
		end
	elseif manjuan_enemy then
		local tg_enemy = prior_enemy or most_enemy
		if tg_enemy and tg_enemy:getHandcardNum() > manjuan_enemy:getHandcardNum() then
			use.card = card
			if use.to then
				use.to:append(tg_enemy)
				use.to:append(manjuan_enemy)
			end
			if not use.isDummy then sgs.updateIntention(self.player, tg_enemy, intention) end
			return
		end
	elseif most_enemy then
		local tg_enemy, second_enemy
		if prior_enemy then
			for _, enemy in ipairs(enemies) do
				if enemy:getHandcardNum() < prior_enemy:getHandcardNum() then
					second_enemy = enemy
					tg_enemy = prior_enemy
					break
				end
			end
		end
		if not second_enemy then
			tg_enemy = most_enemy
			for _, enemy in ipairs(enemies) do
				if enemy:getHandcardNum() < most_enemy:getHandcardNum() then
					second_enemy = enemy
					break
				end
			end
		end
		if tg_enemy and second_enemy then
			use.card = card
			if use.to then
				use.to:append(tg_enemy)
				use.to:append(second_enemy)
			end
			if not use.isDummy then
				sgs.updateIntention(self.player, tg_enemy, intention)
				sgs.updateIntention(self.player, second_enemy, intention)
			end
			return
		end
	end
end

sgs.ai_card_intention.AnxuCard = 0
sgs.ai_use_priority.AnxuCard = 9.6


sgs.ai_skill_playerchosen.zhuiyi = function(self, targets)
	local first, second
	targets = sgs.QList2Table(targets)
	self:sort(targets,"defense")
	for _, friend in ipairs(targets) do
		if self:isFriend(friend) and friend:isAlive() and not (friend:hasSkill("manjuan") and friend:getPhase() == sgs.Player_NotActive and friend:getLostHp() == 0) then
			if isLord(friend) and self:isWeak(friend) then return friend end
			if not (friend:hasSkill("zhiji") and friend:getMark("zhiji") == 0 and not self:isWeak(friend) and friend:getPhase() == sgs.Player_NotActive) then
				if sgs.evaluatePlayerRole(friend) == "renegade" then second = friend
				elseif sgs.evaluatePlayerRole(friend) ~= "renegade" and not first then first = friend
				end
			end
		end
	end
	return first or second
end


function sgs.ai_cardneed.lihuo(to, card, self)
	local slash = card:isKindOf("Slash") and not (card:isKindOf("FireSlash") or card:isKindOf("ThunderSlash"))
	return (card:isKindOf("FireSlash") and getKnownCard(to, self.player, "FireSlash", false) == 0) or (slash and getKnownCard(to, self.player, "Slash", false) == 0)
end

sgs.ai_skill_invoke.lihuo = function(self, data)
	if self.player:hasWeapon("fan") then return false end
	if not sgs.ai_skill_invoke.fan(self, data) then return false end
	local use = data:toCardUse()
	for _, player in sgs.qlist(use.to) do
		if self:isEnemy(player) and self:damageIsEffective(player, sgs.DamageStruct_Fire) and sgs.isGoodTarget(player, self.enemies, self) then
			if player:isChained() then return self:isGoodChainTarget(player) end
			if player:hasArmorEffect("vine") then return true end
		end
	end
	return false
end

sgs.ai_view_as.lihuo = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if sgs.Sanguosha:getCurrentCardUseReason() ~= sgs.CardUseStruct_CARD_USE_REASON_RESPONSE
		and card_place ~= sgs.Player_PlaceSpecial and card:objectName() == "slash" then
		return ("fire_slash:lihuo[%s:%s]=%d"):format(suit, number, card_id)
	end
end

local lihuo_skill={}
lihuo_skill.name="lihuo"
table.insert(sgs.ai_skills,lihuo_skill)
lihuo_skill.getTurnUseCard = function(self)
	local cards = self.player:getCards("h")
	cards=sgs.QList2Table(cards)
	local slash_card

	for _,card in ipairs(cards)  do
		if card:isKindOf("Slash") and not (card:isKindOf("FireSlash") or card:isKindOf("ThunderSlash")) then
			slash_card = card
			break
		end
	end

	if not slash_card then return nil end
	local dummy_use = { to = sgs.SPlayerList(), isDummy = true }
	self:useCardFireSlash(slash_card, dummy_use)
	if dummy_use.card and dummy_use.to:length() > 0 then
		local use = sgs.CardUseStruct()
		use.from = self.player
		use.to = dummy_use.to
		use.card = slash_card
		local data = sgs.QVariant()
		data:setValue(use)
		if not sgs.ai_skill_invoke.lihuo(self, data) then return nil end
	else return nil end

	local suit = slash_card:getSuitString()
	local number = slash_card:getNumberString()
	local card_id = slash_card:getEffectiveId()
	local card_str = ("fire_slash:lihuo[%s:%s]=%d"):format(suit, number, card_id)
	local fireslash = sgs.Card_Parse(card_str)
	assert(fireslash)

	return fireslash

end


function sgs.ai_cardneed.chunlao(to, card)
	return card:isKindOf("Slash") and to:getPile("wine"):isEmpty()
end

sgs.ai_skill_use["@@chunlao"] = function(self, prompt)
	if prompt ~= "@chunlao" then return "." end
	local slashcards={}
	local chunlao = self.player:getPile("wine")
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	for _,card in ipairs(cards)  do
		if card:isKindOf("Slash") then
			table.insert(slashcards,card:getId())
		end
	end
	if #slashcards > 0 and chunlao:isEmpty() then
		return "@ChunlaoCard="..table.concat(slashcards,"+")
	end
	return "."
end

function sgs.ai_cardsview_valuable.chunlao(self, class_name, player)
	if class_name == "Peach" and player:getPile("wine"):length() > 0 then
		local dying = player:getRoom():getCurrentDyingPlayer()
		if dying then
			local analeptic = sgs.Sanguosha:cloneCard("analeptic")
			if dying:isLocked(analeptic) then return nil end
			return "@ChunlaoWineCard=" .. tostring(player:getPile("wine"):first())
		end
	end
end

sgs.ai_card_intention.ChunlaoWineCard = sgs.ai_card_intention.Peach

function sgs.ai_cardneed.chunlao(to, card)
	return to:getPile("wine"):isEmpty() and card:isKindOf("Slash")
end

sgs.chunlao_keep_value = {
	Peach = 6,
	Jink = 5.1,
	Slash = 5.5,
}

sgs.ai_skill_invoke.zhiyu = function(self, data)
	local manjuan = hasManjuanEffect(self.player)
	local damage = data:toDamage()
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	local first
	local difcolor = 0
	for _,card in ipairs(cards)  do
		if not first then first = card end
		if (first:isRed() and card:isBlack()) or (card:isRed() and first:isBlack()) then
			difcolor = 1
			break
		end
	end
	if difcolor == 0 and damage.from then
		if self:isFriend(damage.from) and (not damage.from:isKongcheng() or manjuan) then
			return false
		elseif self:isEnemy(damage.from) then
			if manjuan and self.player:isKongcheng() then return false end
			if self:doNotDiscard(damage.from, "h") and not damage.from:isKongcheng() then return false end
			return true
		end
	end
	if manjuan then return false end
	return true
end

local function get_handcard_suit(cards)
	if #cards == 0 then return sgs.Card_NoSuit end
	if #cards == 1 then return cards[1]:getSuit() end
	local black = false
	if cards[1]:isBlack() then black = true end
	for _, c in ipairs(cards) do
		if black ~= c:isBlack() then return sgs.Card_NoSuit end
	end
	return black and sgs.Card_NoSuitBlack or sgs.Card_NoSuitRed
end

local qice_skill = {}
qice_skill.name = "qice"
table.insert(sgs.ai_skills, qice_skill)
qice_skill.getTurnUseCard = function(self)
	sgs.ai_use_priority.QiceCard = 1.5
	if self.player:hasUsed("QiceCard") or self.player:isKongcheng() then return end
	local cards = self.player:getHandcards()
	local allcard = {}
	cards = sgs.QList2Table(cards)
	local suit = get_handcard_suit(cards)
	local aoename = "savage_assault|archery_attack"
	local aoenames = aoename:split("|")
	local aoe
	local i
	local good, bad = 0, 0
	local caocao = self.room:findPlayerBySkillName("jianxiong")
	local qicetrick = "savage_assault|archery_attack|ex_nihilo|god_salvation"
	local qicetricks = qicetrick:split("|")
	local aoe_available, ge_available, ex_available = true, true, true
	for i = 1, #qicetricks do
		local forbiden = qicetricks[i]
		forbid = sgs.Sanguosha:cloneCard(forbiden, suit)
		if self.player:isCardLimited(forbid, sgs.Card_MethodUse, true) or not forbid:isAvailable(self.player) then
			if forbid:isKindOf("AOE") then aoe_available = false end
			if forbid:isKindOf("GlobalEffect") then ge_available = false end
			if forbid:isKindOf("ExNihilo") then ex_available = false end
		end
	end
	if self.player:hasUsed("QiceCard") then return end
	for _, friend in ipairs(self.friends) do
		if friend:isWounded() then
			good = good + 10 / friend:getHp()
			if friend:isLord() then good = good + 10 / friend:getHp() end
		end
	end

	for _, enemy in ipairs(self.enemies) do
		if enemy:isWounded() then
			bad = bad + 10 / enemy:getHp()
			if enemy:isLord() then
				bad = bad + 10 / enemy:getHp()
			end
		end
	end

	for _, card in ipairs(cards) do
		table.insert(allcard, card:getId())
	end

	if #allcard > 1 then sgs.ai_use_priority.QiceCard = 0 end
	local godsalvation = sgs.Sanguosha:cloneCard("god_salvation", suit, 0)
	if self.player:getHandcardNum() < 3 then
		if aoe_available then
			for i = 1, #aoenames do
				local newqice = aoenames[i]
				aoe = sgs.Sanguosha:cloneCard(newqice)
				if self:getAoeValue(aoe) > 0 then
					local parsed_card = sgs.Card_Parse("@QiceCard=" .. table.concat(allcard, "+") .. ":" .. newqice)
					return parsed_card
				end
			end
		end
		if ge_available and self:willUseGodSalvation(godsalvation) then
			local parsed_card = sgs.Card_Parse("@QiceCard=" .. table.concat(allcard, "+") .. ":" .. "god_salvation")
			return parsed_card
		end
		if ex_available and self:getCardsNum("Jink") == 0 and self:getCardsNum("Peach") == 0 then
			local parsed_card = sgs.Card_Parse("@QiceCard=" .. table.concat(allcard, "+") .. ":" .. "ex_nihilo")
			return parsed_card
		end
	end

	if self.player:getHandcardNum() == 3 then
		if aoe_available then
			for i = 1, #aoenames do
				local newqice = aoenames[i]
				aoe = sgs.Sanguosha:cloneCard(newqice)
				if self:getAoeValue(aoe) > 0 then
					local parsed_card = sgs.Card_Parse("@QiceCard=" .. table.concat(allcard, "+") .. ":" .. newqice)
					return parsed_card
				end
			end
		end
		if ge_available and self:willUseGodSalvation(godsalvation) and self.player:isWounded() then
			local parsed_card = sgs.Card_Parse("@QiceCard=" .. table.concat(allcard, "+") .. ":" .. "god_salvation")
			return parsed_card
		end
		if ex_available and self:getCardsNum("Jink") == 0 and self:getCardsNum("Peach") == 0 and self:getCardsNum("Analeptic") == 0 and self:getCardsNum("Nullification") == 0 then
			local parsed_card = sgs.Card_Parse("@QiceCard=" .. table.concat(allcard, "+") .. ":" .. "ex_nihilo")
			return parsed_card
		end
	end
	if aoe_available then
		for i = 1, #aoenames do
			local newqice = aoenames[i]
			aoe = sgs.Sanguosha:cloneCard(newqice)
			if self:getAoeValue(aoe) > -5 and caocao and self:isFriend(caocao) and caocao:getHp() > 1 and not self:willSkipPlayPhase(caocao)
				and not self.player:hasSkill("jueqing") and self:aoeIsEffective(aoe, caocao, self.player) then
				local parsed_card = sgs.Card_Parse("@QiceCard=" .. table.concat(allcard, "+") .. ":" .. newqice)
				return parsed_card
			end
		end
	end
	if self:getCardsNum("Jink") == 0 and self:getCardsNum("Peach") == 0 and self:getCardsNum("Analeptic") == 0
		and self:getCardsNum("Nullification") == 0 and self.player:getHandcardNum() <= 3 then
		if ge_available and self:willUseGodSalvation(godsalvation) and self.player:isWounded() then
			local parsed_card = sgs.Card_Parse("@QiceCard=" .. table.concat(allcard, "+") .. ":" .. "god_salvation")
			return parsed_card
		end
		if ex_available then
			local parsed_card = sgs.Card_Parse("@QiceCard=" .. table.concat(allcard, "+") .. ":" .. "ex_nihilo")
			return parsed_card
		end
	end
end

sgs.ai_skill_use_func.QiceCard = function(card, use, self)
	local userstring = card:toString()
	userstring = (userstring:split(":"))[3]
	local qicecard = sgs.Sanguosha:cloneCard(userstring, card:getSuit(), card:getNumber())
	qicecard:setSkillName("qice")
	self:useTrickCard(qicecard, use)
	if use.card then
		for _, acard in sgs.qlist(self.player:getHandcards()) do
			if isCard("Peach", acard, self.player) and self.player:getHandcardNum() > 1 and self.player:isWounded()
				and not self:needToLoseHp(self.player) then
					use.card = acard
					return
			end
		end
		use.card = card
	end
end

sgs.ai_use_priority.QiceCard = 1.5

sgs.ai_skill_cardask["@renxin-card"] = function(self, data, pattern)
	local dmg = data:toDamage()
	local invoke
	if self:isFriend(dmg.to) then
		if self:damageIsEffective_(dmg) and not self:getDamagedEffects(dmg.to, dmg.from, dmg.card and dmg.card:isKindOf("Slash"))
			and not self:needToLoseHp(dmg.to, dmg.from, dmg.card and dmg.card:isKindOf("Slash")) then
			invoke = true
		elseif not self:toTurnOver(self.player) then
			invoke = true
		end
	elseif self:objectiveLevel(dmg.to) == 0 and not self:toTurnOver(self.player) then
		invoke = true
	end
	if invoke then
		local equipCards = {}
		for _, c in sgs.qlist(self.player:getCards("he")) do
			if c:isKindOf("EquipCard") and self.player:canDiscard(self.player, c:getEffectiveId()) then
				table.insert(equipCards, c)
			end
		end
		if #equipCards > 0 then
			self:sortByKeepValue(equipCards)
			return equipCards[1]:getEffectiveId()
		end
	end
	return "."
end

sgs.ai_skill_invoke.jingce = function(self, data)
	return not self:needKongcheng(self.player, true)
end

local junxing_skill = {}
junxing_skill.name = "junxing"
table.insert(sgs.ai_skills, junxing_skill)
junxing_skill.getTurnUseCard = function(self)
	if self.player:isKongcheng() or self.player:hasUsed("JunxingCard") then return nil end
	return sgs.Card_Parse("@JunxingCard=.")
end

sgs.ai_skill_use_func.JunxingCard = function(card, use, self)
	-- find enough cards
	local unpreferedCards = {}
	local cards = sgs.QList2Table(self.player:getHandcards())
	local use_slash_num = 0
	self:sortByKeepValue(cards)
	for _, card in ipairs(cards) do
		if card:isKindOf("Slash") then
			local will_use = false
			if use_slash_num <= sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, card) then
				local dummy_use = { isDummy = true }
				self:useBasicCard(card, dummy_use)
				if dummy_use.card then
					will_use = true
					use_slash_num = use_slash_num + 1
				end
			end
			if not will_use then table.insert(unpreferedCards, card:getId()) end
		end
	end
	local num = self:getCardsNum("Jink") - 1
	if self.player:getArmor() then num = num + 1 end
	if num > 0 then
		for _, card in ipairs(cards) do
			if card:isKindOf("Jink") and num > 0 then
				table.insert(unpreferedCards, card:getId())
				num = num - 1
			end
		end
	end
	for _, card in ipairs(cards) do
		if card:isKindOf("EquipCard") then
			local dummy_use = { isDummy = true }
			self:useEquipCard(card, dummy_use)
			if not dummy_use.card then table.insert(unpreferedCards, card:getId()) end
		end
	end
	for _, card in ipairs(cards) do
		if card:isNDTrick() or card:isKindOf("Lightning") then
			local dummy_use = { isDummy = true }
			self:useTrickCard(card, dummy_use)
			if not dummy_use.card then table.insert(unpreferedCards, card:getId()) end
		end
	end
	local use_cards = {}
	for index = #unpreferedCards, 1, -1 do
		if not self.player:isJilei(sgs.Sanguosha:getCard(unpreferedCards[index])) then table.insert(use_cards, unpreferedCards[index]) end
	end
	if #use_cards == 0 then return end

	-- to friends
	self:sort(self.friends_noself, "defense")
	for _, friend in ipairs(self.friends_noself) do
		if not self:toTurnOver(friend, #use_cards) then
			use.card = sgs.Card_Parse("@JunxingCard=" .. table.concat(use_cards, "+"))
			if use.to then use.to:append(friend) end
			return
		end
	end
	if #use_cards > 3 then
		for _, friend in ipairs(self.friends_noself) do
			if friend:hasSkills(sgs.notActive_cardneed_skill) and not friend:hasSkills(sgs.Active_cardneed_skill) then
				use.card = sgs.Card_Parse("@JunxingCard=" .. table.concat(use_cards, "+"))
				if use.to then use.to:append(friend) end
				return
			end
		end
	end

	-- to enemies
	local basic, trick, equip
	for _, id in ipairs(use_cards) do
		local typeid = sgs.Sanguosha:getEngineCard(id):getTypeId()
		if not basic and typeid == sgs.Card_TypeBasic then basic = id
		elseif not trick and typeid == sgs.Card_TypeTrick then trick = id
		elseif not equip and typeid == sgs.Card_TypeEquip then equip = id
		end
		if basic and trick and equip then break end
	end
	self:sort(self.enemies, "handcard")
	local other_enemy
	for _, enemy in ipairs(self.enemies) do
		local id = nil
		if self:toTurnOver(enemy, 1) then
			if getKnownCard(enemy, self.player, "BasicCard") == 0 then id = equip or trick end
			if not id and getKnownCard(enemy, self.player, "TrickCard") == 0 then id = equip or basic end
			if not id and getKnownCard(enemy, self.player, "EquipCard") == 0 then id = trick or basic end
			if id then
				use.card = sgs.Card_Parse("@JunxingCard=" .. id)
				if use.to then use.to:append(enemy) end
				return
			elseif not other_enemy then
				other_enemy = enemy
			end
		end
	end
	if other_enemy then
		use.card = sgs.Card_Parse("@JunxingCard=" .. use_cards[1])
		if use.to then use.to:append(other_enemy) end
		return
	end
end

sgs.ai_use_priority.JunxingCard = 1.2
sgs.ai_card_intention.JunxingCard = function(self, card, from, tos)
	local to = tos[1]
	if not to:faceUp() then
		sgs.updateIntention(from, to, -80)
	else
		if to:getHandcardNum() <= 1 and card:subcardsLength() >= 3 then
			sgs.updateIntention(from, to, -40)
		else
			sgs.updateIntention(from, to, 80)
		end
	end
end

sgs.ai_skill_cardask["@junxing-discard"] = function(self, data, pattern)
	local manchong = self.room:findPlayerBySkillName("junxing")
	if manchong and self:isFriend(manchong) then return "." end

	local types = pattern:split("|")[1]:split(",")
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	for _, card in ipairs(cards) do
		if not self:isValuableCard(card) then
			for _, classname in ipairs(types) do
				if card:isKindOf(classname) then return "$" .. card:getEffectiveId() end
			end
		end
	end
	return "."
end

sgs.ai_skill_cardask["@yuce-show"] = function(self, data)
	local damage = self.room:getTag("CurrentDamageStruct"):toDamage()
	if not damage.from or damage.from:isDead() then return "." end
	if self:isFriend(damage.from) then return "$" .. self.player:handCards():first() end
	local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), damage.from:objectName())
	local types = { sgs.Card_TypeBasic, sgs.Card_TypeEquip, sgs.Card_TypeTrick }
	for _, card in sgs.qlist(damage.from:getHandcards()) do
		if card:hasFlag("visible") or card:hasFlag(flag) then
			table.removeOne(types, card:getTypeId())
		end
		if #types == 0 then break end
	end
	if #types == 0 then types = { sgs.Card_TypeBasic } end
	for _, card in sgs.qlist(self.player:getHandcards()) do
		for _, cardtype in ipairs(types) do
			if card:getTypeId() == cardtype then return "$" .. card:getEffectiveId() end
		end
	end
	return "$" .. self.player:handCards():first()
end

sgs.ai_skill_cardask["@yuce-discard"] = function(self, data, pattern, target)
	if target and self:isFriend(target) then return "." end
	local types = pattern:split("|")[1]:split(",")
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards)
	for _, card in ipairs(cards) do
		if not self:isValuableCard(card) then
			for _, classname in ipairs(types) do
				if card:isKindOf(classname) then return "$" .. card:getEffectiveId() end
			end
		end
	end
	return "."
end

sgs.ai_skill_use["@@xiansi"] = function(self, prompt)
	local crossbow_effect
	if not self.player:getTag("HuashenSkill"):toString() == "xiansi" then
		for _, enemy in ipairs(self.enemies) do
			if enemy:inMyAttackRange(self.player) and (self:hasCrossbowEffect(enemy) or getKnownCard(enemy, self.player, "Crossbow") > 0) then
				crossbow_effect = true
				break
			end
		end
	end
	local max_num = 999
	if crossbow_effect then max_num = 3
	elseif self:getCardsNum("Jink") < 1 or self:isWeak() then max_num = 5 end
	if self.player:getPile("counter"):length() >= max_num then return "." end
	local rest_num = math.min(2, max_num - self.player:getPile("counter"):length())
	local targets = {}

	local add_player = function(player, isfriend)
		if player:getHandcardNum() == 0 or player:objectName() == self.player:objectName() then return #targets end
		if self:objectiveLevel(player) == 0 and player:isLord() and sgs.current_mode_players["rebel"] > 1 then return #targets end
		if #targets == 0 then
			table.insert(targets, player:objectName())
		elseif #targets == 1 then
			if player:objectName() ~= targets[1] then
				table.insert(targets, player:objectName())
			end
		end
		if isfriend and isfriend == 1 then
			self.player:setFlags("AI_XiansiToFriend_" .. player:objectName())
		end
		return #targets
	end

	local player = self:findPlayerToDiscard("he", true, false)
	if player then
		if rest_num == 1 then return "@XiansiCard=.->" .. player:objectName() end
		add_player(player, self:isFriend(player) and 1 or nil)
		local another = self:findPlayerToDiscard("he", true, false, self.room:getOtherPlayers(player))
		if another then
			add_player(another, self:isFriend(another) and 1 or nil)
			return "@XiansiCard=.->" .. table.concat(targets, "+")
		end
	end

	local lord = self.room:getLord()
	if lord and self:isEnemy(lord) and sgs.turncount <= 1 and not lord:isNude() then
		if add_player(lord) == rest_num then return "@XiansiCard=.->" .. table.concat(targets, "+") end
	end

	local zhugeliang = self.room:findPlayerBySkillName("kongcheng")
	local luxun = self.room:findPlayerBySkillName("lianying") or self.room:findPlayerBySkillName("noslianying")
	local dengai = self.room:findPlayerBySkillName("tuntian")
	local jiangwei = self.room:findPlayerBySkillName("zhiji")

	if jiangwei and self:isFriend(jiangwei) and jiangwei:getMark("zhiji") == 0 and jiangwei:getHandcardNum()== 1
		and self:getEnemyNumBySeat(self.player, jiangwei) <= (jiangwei:getHp() >= 3 and 1 or 0) then
		if add_player(jiangwei, 1) == rest_num then return "@XiansiCard=.->" .. table.concat(targets, "+") end
	end
	if dengai and dengai:hasSkill("zaoxian") and self:isFriend(dengai) and (not self:isWeak(dengai) or self:getEnemyNumBySeat(self.player, dengai) == 0)
		and dengai:getMark("zaoxian") == 0 and dengai:getPile("field"):length() == 2 and add_player(dengai, 1) == rest_num then
		return "@XiansiCard=.->" .. table.concat(targets, "+")
	end

	if zhugeliang and self:isFriend(zhugeliang) and zhugeliang:getHandcardNum() == 1 and self:getEnemyNumBySeat(self.player, zhugeliang) > 0 then
		if zhugeliang:getHp() <= 2 then
			if add_player(zhugeliang, 1) == rest_num then return "@XiansiCard=.->" .. table.concat(targets, "+") end
		else
			local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), zhugeliang:objectName())
			local cards = sgs.QList2Table(zhugeliang:getHandcards())
			if #cards == 1 and (cards[1]:hasFlag("visible") or cards[1]:hasFlag(flag)) then
				if cards[1]:isKindOf("TrickCard") or cards[1]:isKindOf("Slash") or cards[1]:isKindOf("EquipCard") then
					if add_player(zhugeliang, 1) == rest_num then return "@XiansiCard=.->" .. table.concat(targets, "+") end
				end
			end
		end
	end

	if luxun and self:isFriend(luxun) and luxun:getHandcardNum() == 1 and self:getEnemyNumBySeat(self.player, luxun) > 0 then
		local flag = string.format("%s_%s_%s", "visible", self.player:objectName(), luxun:objectName())
		local cards = sgs.QList2Table(luxun:getHandcards())
		if #cards == 1 and (cards[1]:hasFlag("visible") or cards[1]:hasFlag(flag)) then
			if cards[1]:isKindOf("TrickCard") or cards[1]:isKindOf("Slash") or cards[1]:isKindOf("EquipCard") then
				if add_player(luxun, 1) == rest_num then return "@XiansiCard=.->" .. table.concat(targets, "+") end
			end
		end
	end

	if luxun and add_player(luxun, (self:isFriend(luxun) and 1 or nil)) == rest_num then
		return "@XiansiCard=.->" .. table.concat(targets, "+")
	end

	if dengai and self:isFriend(dengai) and (not self:isWeak(dengai) or self:getEnemyNumBySeat(self.player, dengai) == 0) and add_player(dengai, 1) == rest_num then
		return "@XiansiCard=.->" .. table.concat(targets, "+")
	end

	if #targets == 1 then
		local target = findPlayerByObjectName(self.room, targets[1])
		if target then
			local another
			if rest_num > 1 then another = self:findPlayerToDiscard("he", true, false, self.room:getOtherPlayers(target)) end
			if another then
				add_player(another, self:isFriend(another) and 1 or nil)
				return "@XiansiCard=.->" .. table.concat(targets, "+")
			else
				return "@XiansiCard=.->" .. targets[1]
			end
		end
	end
	return "."
end

sgs.ai_card_intention.XiansiCard = function(self, card, from, tos)
	local lord = self.room:getLord()
	if sgs.evaluatePlayerRole(from) == "neutral" and sgs.evaluatePlayerRole(tos[1]) == "neutral"
		and (not tos[2] or sgs.evaluatePlayerRole(tos[2]) == "neutral") and lord and not lord:isNude()
		and self:doNotDiscard(lord, "he", true) and from:aliveCount() >= 4 then
		sgs.updateIntention(from, lord, -35)
		return
	end
	if from:getState() == "online" then
		for _, to in ipairs(tos) do
			if (self:hasSkills("kongcheng|zhiji|lianying|noslianying", to) and to:getHandcardNum() == 1) or to:hasSkills("tuntian+zaoxian") then
			else
				sgs.updateIntention(from, to, 80)
			end
		end
	else
		for _, to in ipairs(tos) do
			local intention = from:hasFlag("AI_XiansiToFriend_" .. to:objectName()) and -5 or 80
			sgs.updateIntention(from, to, intention)
		end
	end
end

local getXiansiCard = function(pile)
	if #pile > 1 then return pile[1], pile[2] end
	return nil
end

local xiansi_slash_skill = {}
xiansi_slash_skill.name = "xiansi_slash"
table.insert(sgs.ai_skills, xiansi_slash_skill)
xiansi_slash_skill.getTurnUseCard = function(self)
	if not self:slashIsAvailable() then return end
	local liufeng = self.room:findPlayerBySkillName("xiansi")
	if not liufeng or liufeng:getPile("counter"):length() <= 1 or not self.player:canSlash(liufeng) then return end
	local ints = sgs.QList2Table(liufeng:getPile("counter"))
	local a, b = getXiansiCard(ints)
	if a and b then
		return sgs.Card_Parse("@XiansiSlashCard=" .. tostring(a) .. "+" .. tostring(b))
	end
end

sgs.ai_skill_use_func.XiansiSlashCard = function(card, use, self)
	local liufeng = self.room:findPlayerBySkillName("xiansi")
	if not liufeng or liufeng:getPile("counter"):length() <= 1 or not self.player:canSlash(liufeng) then return "." end
	local slash = sgs.Sanguosha:cloneCard("slash")

	if self:slashIsAvailable() and not self:slashIsEffective(slash, liufeng, self.player) and self:isFriend(liufeng) then
		sgs.ai_use_priority.XiansiSlashCard = 0.1
		use.card = card
		if use.to then use.to:append(liufeng) end
	else
		sgs.ai_use_priority.XiansiSlashCard = 2.6
		local dummy_use = { to = sgs.SPlayerList() }
		self:useCardSlash(slash, dummy_use)
		if dummy_use.card then
			if (dummy_use.card:isKindOf("GodSalvation") or dummy_use.card:isKindOf("Analeptic") or dummy_use.card:isKindOf("Weapon"))
				and self:getCardsNum("Slash") > 0 then
				use.card = dummy_use.card
				if use.to then use.to:append(liufeng) end
			else
				if dummy_use.card:isKindOf("Slash") and dummy_use.to:length() > 0 then
					local lf
					for _, p in sgs.qlist(dummy_use.to) do
						if p:objectName() == liufeng:objectName() then
							lf = true
							break
						end
					end
					if lf then
						use.card = card
						if use.to then use.to:append(liufeng) end
					end
				end
			end
		end
	end
	if not use.card then
		sgs.ai_use_priority.XiansiSlashCard = 2.0
		if self:slashIsAvailable() and self:isEnemy(liufeng)
			and not self:slashProhibit(slash, liufeng) and self:slashIsEffective(slash, liufeng) and sgs.isGoodTarget(liufeng, self.enemies, self) then
			use.card = card
			if use.to then use.to:append(liufeng) end
		end
	end
end

sgs.ai_card_intention.XiansiSlashCard = function(self, card, from, tos)
	local slash = sgs.Sanguosha:cloneCard("slash")
	if not self:slashIsEffective(slash, tos[1], from) then
		sgs.updateIntention(from, tos[1], -30)
	else
		return sgs.ai_card_intention.Slash(self, slash, from, tos)
	end
end

sgs.ai_skill_cardask["@longyin"] = function(self, data)
	local function getLeastValueCard(isRed)
		local offhorse_avail, weapon_avail
		for _, enemy in ipairs(self.enemies) do
			if self:canAttack(enemy, self.player) then
				if not offhorse_avail and self.player:getOffensiveHorse() and self.player:distanceTo(enemy, 1) <= self.player:getAttackRange() then
					offhorse_avail = true
				end
				if not weapon_avail and self.player:getWeapon() and self.player:distanceTo(enemy) == 1 then
					weapon_avail = true
				end
			end
			if offhorse_avail and weapon_avail then break end
		end
		if self:needToThrowArmor() then return "$" .. self.player:getArmor():getEffectiveId() end
		if self.player:getPhase() > sgs.Player_Play then
			local cards = sgs.QList2Table(self.player:getHandcards())
			self:sortByKeepValue(cards)
			for _, c in ipairs(cards) do
				if self:getKeepValue(c) < 8 and not self.player:isJilei(c) and not self:isValuableCard(c) then return "$" .. c:getEffectiveId() end
			end
			if offhorse_avail and not self.player:isJilei(self.player:getOffensiveHorse()) then return "$" .. self.player:getOffensiveHorse():getEffectiveId() end
			if weapon_avail and not self.player:isJilei(self.player:getWeapon()) and self:evaluateWeapon(self.player:getWeapon()) < 5 then return "$" .. self.player:getWeapon():getEffectiveId() end
		else
			local slashc
			local cards = sgs.QList2Table(self.player:getHandcards())
			self:sortByUseValue(cards)
			for _, c in ipairs(cards) do
				if self:getUseValue(c) < 6 and not self:isValuableCard(c) and not self.player:isJilei(c) then
					if isCard("Slash", c, self.player) then
						if not slashc then slashc = c end
					else
						return "$" .. c:getEffectiveId()
					end
				end
			end
			if offhorse_avail and not self.player:isJilei(self.player:getOffensiveHorse()) then return "$" .. self.player:getOffensiveHorse():getEffectiveId() end
			if isRed and slashc then return "$" .. slashc:getEffectiveId() end
		end
	end
	local use = data:toCardUse()
	local slash = use.card
	local slash_num = 0
	if use.from:objectName() == self.player:objectName() then slash_num = self:getCardsNum("Slash") else slash_num = getCardsNum("Slash", use.from, self.player) end
	if self:isEnemy(use.from) and use.m_addHistory and not self:hasCrossbowEffect(use.from) and slash_num > 0 then return "." end
	if (slash:isRed() and not hasManjuanEffect(self.player))
		or (use.m_reason == sgs.CardUseStruct_CARD_USE_REASON_PLAY and use.m_addHistory and self:isFriend(use.from) and slash_num >= 1
			and (not self:hasCrossbowEffect(use.from) or slash:isRed())) then
		local str = getLeastValueCard(slash:isRed())
		if str then return str end
	end
	return "."
end

sgs.ai_skill_use["@@qiaoshui"] = function(self, prompt)
	local trick_num = 0
	for _, card in sgs.qlist(self.player:getHandcards()) do
		if card:isNDTrick() and not card:isKindOf("Nullification") then trick_num = trick_num + 1 end
	end
	self:sort(self.enemies, "handcard")
	local max_card = self:getMaxCard()
	local max_point = max_card:getNumber()

	for _, enemy in ipairs(self.enemies) do
		if not (enemy:hasSkill("kongcheng") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			local enemy_max_card = self:getMaxCard(enemy)
			local enemy_max_point = enemy_max_card and enemy_max_card:getNumber() or 100
			if max_point > enemy_max_point then
				self.qiaoshui_card = max_card:getEffectiveId()
				return "@QiaoshuiCard=.->" .. enemy:objectName()
			end
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if not (enemy:hasSkill("kongcheng") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
			if max_point >= 10 then
				self.qiaoshui_card = max_card:getEffectiveId()
				return "@QiaoshuiCard=.->" .. enemy:objectName()
			end
		end
	end

	self:sort(self.friends_noself, "handcard")
	for index = #self.friends_noself, 1, -1 do
		local friend = self.friends_noself[index]
		if not friend:isKongcheng() then
			local friend_min_card = self:getMinCard(friend)
			local friend_min_point = friend_min_card and friend_min_card:getNumber() or 100
			if max_point > friend_min_point then
				self.qiaoshui_card = max_card:getEffectiveId()
				return "@QiaoshuiCard=.->" .. friend:objectName()
			end
		end
	end

	local zhugeliang = self.room:findPlayerBySkillName("kongcheng")
	if zhugeliang and self:isFriend(zhugeliang) and zhugeliang:getHandcardNum() == 1 and zhugeliang:objectName() ~= self.player:objectName() then
		if max_point >= 7 then
			self.qiaoshui_card = max_card:getEffectiveId()
			return "@QiaoshuiCard=.->" .. zhugeliang:objectName()
		end
	end

	for index = #self.friends_noself, 1, -1 do
		local friend = self.friends_noself[index]
		if not friend:isKongcheng() then
			if max_point >= 7 then
				self.qiaoshui_card = max_card:getEffectiveId()
				return "@QiaoshuiCard=.->" .. friend:objectName()
			end
		end
	end

	if trick_num == 0 or (trick_num <= 2 and self.player:hasSkill("zongshih")) and not self:isValuableCard(max_card) then
		for _, enemy in ipairs(self.enemies) do
			if not (enemy:hasSkill("kongcheng") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() and self:hasLoseHandcardEffective(enemy) then
				self.qiaoshui_card = max_card:getEffectiveId()
				return "@QiaoshuiCard=.->" .. enemy:objectName()
			end
		end
	end
	return "."
end

sgs.ai_card_intention.QiaoshuiCard = 0

sgs.ai_skill_choice.qiaoshui = function(self, choices, data)
	local use = data:toCardUse()
	if use.card:isKindOf("Collateral") then
		local dummy_use = { isDummy = true, to = sgs.SPlayerList(), current_targets = {} }
		for _, p in sgs.qlist(use.to) do
			table.insert(dummy_use.current_targets, p:objectName())
		end
		self:useCardCollateral(use.card, dummy_use)
		if dummy_use.card and dummy_use.to:length() == 2 then
			local first = dummy_use.to:at(0):objectName()
			local second = dummy_use.to:at(1):objectName()
			self.qiaoshui_collateral = { first, second }
			return "add"
		else
			self.qiaoshui_collateral = nil
		end
	elseif use.card:isKindOf("Analeptic") then
	elseif use.card:isKindOf("Peach") then
		self:sort(self.friends_noself, "hp")
		for _, friend in ipairs(self.friends_noself) do
			if friend:isWounded() and friend:getHp() < getBestHp(friend) then
				self.qiaoshui_extra_target = friend
				return "add"
			end
		end
	elseif use.card:isKindOf("ExNihilo") then
		local friend = self:findPlayerToDraw(false, 2)
		if friend then
			self.qiaoshui_extra_target = friend
			return "add"
		end
	elseif use.card:isKindOf("GodSalvation") then
		self:sort(self.enemies, "hp")
		for _, enemy in ipairs(self.enemies) do
			if enemy:isWounded() and self:hasTrickEffective(use.card, enemy, self.player) then
				self.qiaoshui_remove_target = enemy
				return "remove"
			end
		end
	elseif use.card:isKindOf("AmazingGrace") then
		self:sort(self.enemies)
		for _, enemy in ipairs(self.enemies) do
			if self:hasTrickEffective(use.card, enemy, self.player) and not hasManjuanEffect(enemy)
				and not self:needKongcheng(enemy, true) then
				self.qiaoshui_remove_target = enemy
				return "remove"
			end
		end
	elseif use.card:isKindOf("AOE") then
		self:sort(self.friends_noself)
		local lord = self.room:getLord()
		if lord and lord:objectName() ~= self.player:objectName() and self:isFriend(lord) and self:isWeak(lord) then
			self.qiaoshui_remove_target = lord
			return "remove"
		end
		for _, friend in ipairs(self.friends_noself) do
			if self:hasTrickEffective(use.card, friend, self.player) then
				self.qiaoshui_remove_target = friend
				return "remove"
			end
		end
	elseif use.card:isKindOf("Snatch") or use.card:isKindOf("Dismantlement") then
		local trick = sgs.Sanguosha:cloneCard(use.card:objectName(), use.card:getSuit(), use.card:getNumber())
		trick:setSkillName("qiaoshui")
		local dummy_use = { isDummy = true, to = sgs.SPlayerList(), current_targets = {} }
		for _, p in sgs.qlist(use.to) do
			table.insert(dummy_use.current_targets, p:objectName())
		end
		self:useCardSnatchOrDismantlement(trick, dummy_use)
		if dummy_use.card and dummy_use.to:length() > 0 then
			self.qiaoshui_extra_target = dummy_use.to:first()
			return "add"
		end
	elseif use.card:isKindOf("Slash") then
		local slash = sgs.Sanguosha:cloneCard(use.card:objectName(), use.card:getSuit(), use.card:getNumber())
		slash:setSkillName("qiaoshui")
		local dummy_use = { isDummy = true, to = sgs.SPlayerList(), current_targets = {} }
		for _, p in sgs.qlist(use.to) do
			table.insert(dummy_use.current_targets, p:objectName())
		end
		self:useCardSlash(slash, dummy_use)
		if dummy_use.card and dummy_use.to:length() > 0 then
			self.qiaoshui_extra_target = dummy_use.to:first()
			return "add"
		end
	else
		local dummy_use = { isDummy = true, to = sgs.SPlayerList(), current_targets = {} }
		for _, p in sgs.qlist(use.to) do
			table.insert(dummy_use.current_targets, p:objectName())
		end
		self:useCardByClassName(use.card, dummy_use)
		if dummy_use.card and dummy_use.to:length() > 0 then
			self.qiaoshui_extra_target = dummy_use.to:first()
			return "add"
		end
	end
	self.qiaoshui_extra_target = nil
	self.qiaoshui_remove_target = nil
	return "cancel"
end

sgs.ai_skill_playerchosen.qiaoshui = function(self, targets)
	if not self.qiaoshui_extra_target and not self.qiaoshui_remove_target then self.room:writeToConsole("Qiaoshui player chosen error!!") end
	return self.qiaoshui_extra_target or self.qiaoshui_remove_target
end

sgs.ai_skill_use["@@qiaoshui!"] = function(self, prompt) -- extra target for Collateral
	if not self.qiaoshui_collateral then self.room:writeToConsole("Qiaoshui player chosen error!!") end
	return "@ExtraCollateralCard=.->" .. self.qiaoshui_collateral[1] .. "+" .. self.qiaoshui_collateral[2]
end

sgs.ai_skill_invoke.zongshih = function(self, data)
	return not self:needKongcheng(self.player, true)
end

sgs.ai_skill_cardask["@duodao-get"] = function(self, data)
	local function getLeastValueCard(from)
		if self:needToThrowArmor() then return "$" .. self.player:getArmor():getEffectiveId() end
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		for _, c in ipairs(cards) do
			if self:getKeepValue(c) < 8 and not self.player:isJilei(c) and not self:isValuableCard(c) then return "$" .. c:getEffectiveId() end
		end
		local offhorse_avail, weapon_avail
		for _, enemy in ipairs(self.enemies) do
			if self:canAttack(enemy, self.player) then
				if not offhorse_avail and self.player:getOffensiveHorse() and self.player:distanceTo(enemy, 1) <= self.player:getAttackRange() then
					offhorse_avail = true
				end
				if not weapon_avail and self.player:getWeapon() and self.player:distanceTo(enemy) == 1 then
					weapon_avail = true
				end
			end
			if offhorse_avail and weapon_avail then break end
		end
		if offhorse_avail and not self.player:isJilei(self.player:getOffensiveHorse()) then return "$" .. self.player:getOffensiveHorse():getEffectiveId() end
		if weapon_avail and not self.player:isJilei(self.player:getWeapon()) and self:evaluateWeapon(self.player:getWeapon()) < self:evaluateWeapon(from:getWeapon()) then
			return "$" .. self.player:getWeapon():getEffectiveId()
		end
	end
	local damage = data:toDamage()
	if not damage.from or not damage.from:getWeapon() then
		if self:needToThrowArmor() then
			return "$" .. self.player:getArmor():getEffectiveId()
		elseif self.player:getHandcardNum() == 1 and (self.player:hasSkill("kongcheng") or (self.player:hasSkill("zhiji") and self.player:getMark("zhiji") == 0)) then
			return "$" .. self.player:handCards():first()
		end
	else
		if self:isFriend(damage.from) then
			if damage.from:hasSkills("kofxiaoji|xiaoji") and self:isWeak(damage.from) then
				local str = getLeastValueCard(damage.from)
				if str then return str end
			else
				if self:getCardsNum("Slash") == 0 or self:willSkipPlayPhase() then return "." end
				local invoke = false
				local range = sgs.weapon_range[damage.from:getWeapon():getClassName()] or 0
				if self.player:hasSkill("anjian") then
					for _, enemy in ipairs(self.enemies) do
						if not enemy:inMyAttackRange(self.player) and not self.player:inMyAttackRange(enemy) and self.player:distanceTo(enemy) <= range then
							invoke = true
							break
						end
					end
				end
				if not invoke and self:evaluateWeapon(damage.from:getWeapon()) > 8 then invoke = true end
				if invoke then
					local str = getLeastValueCard(damage.from)
					if str then return str end
				end
			end
		else
			if damage.from:hasSkill("nosxuanfeng") then
				for _, friend in ipairs(self.friends) do
					if self:isWeak(friend) then return "." end
				end
			else
				if hasManjuanEffect(self.player) then
					if self:needToThrowArmor() and not self.player:isJilei(self.player:getArmor()) then
						return "$" .. self.player:getArmor():getEffectiveId()
					elseif self.player:getHandcardNum() == 1
							and (self.player:hasSkill("kongcheng") or (self.player:hasSkill("zhiji") and self.player:getMark("zhiji") == 0))
							and not self.player:isJilei(self.player:getHandcards():first()) then
						return "$" .. self.player:handCards():first()
					end
				else
					local str = getLeastValueCard(damage.from)
					if str then return str end
				end
			end
		end
	end
	return "."
end

local danshou_skill = {}
danshou_skill.name = "danshou"
table.insert(sgs.ai_skills, danshou_skill)
danshou_skill.getTurnUseCard = function(self)
	local times = self.player:getMark("danshou") + 1
	if times < self.player:getCardCount(true) then
		return sgs.Card_Parse("@DanshouCard=.")
	end
end

sgs.ai_skill_use_func.DanshouCard = function(card, use, self)
	local times = self.player:getMark("danshou") + 1
	local cards = self.player:getCards("he")
	local jink_num = self:getCardsNum("Jink")
	local to_discard = {}
	for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
		cards:append(sgs.Sanguosha:getCard(id))
	end
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	local has_weapon = false
	local DisWeapon = false
	for _, card in ipairs(cards) do
		if card:isKindOf("Weapon") and card:isBlack() then has_weapon = true end
	end
	for _, card in ipairs(cards) do
		if self.player:canDiscard(self.player, card:getEffectiveId()) and ((self:getUseValue(card) < sgs.ai_use_value.Dismantlement + 1) or self:getOverflow() > 0) then
			local shouldUse = true
			if card:isKindOf("Armor") then
				if not self.player:getArmor() then shouldUse = false
				elseif self.player:hasEquip(card) and not (card:isKindOf("SilverLion") and self.player:isWounded()) then shouldUse = false
				end
			end
			if card:isKindOf("Weapon") and self.player:getHandcardNum() > 2 then
				if not self.player:getWeapon() then shouldUse = false
				elseif self.player:hasEquip(card) and not has_weapon then shouldUse = false
				else DisWeapon = true
				end
			end
			if card:isKindOf("Slash") then
				local dummy_use = { isDummy = true }
				if self:getCardsNum("Slash") == 1 then
					self:useBasicCard(card, dummy_use)
					if dummy_use.card then shouldUse = false end
				end
			end
			if self:getUseValue(card) > sgs.ai_use_value.Dismantlement + 1 and card:isKindOf("TrickCard") then
				local dummy_use = { isDummy = true }
				self:useTrickCard(card, dummy_use)
				if dummy_use.card then shouldUse = false end
			end
			if isCard("Peach", card, self.player) then
				if times ~= 3 then shouldUse = false end
			end
			if isCard("Jink", card, self.player) then
				if jink_num <= 1 and times == 1 then shouldUse = false
				else jink_num = jink_num - 1 end
			end
			if shouldUse then
				table.insert(to_discard, card:getEffectiveId())
				if #to_discard == times then break end
			end
		end
	end

	local range =   self.player:getAttackRange()
	if DisWeapon then range = 1 end
	local target
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if self.player:distanceTo(p) <= range then
			if times == 1 or times == 2 then
				if self.player:canDiscard(p, "he") and not p:isNude() then
					if self:isFriend(p) then
						if(self:hasSkills(sgs.lose_equip_skill, p) and not p:getEquips():isEmpty())
						or (self:needToThrowArmor(p) and p:getArmor()) or self:doNotDiscard(p) then
							target = p  break end
					elseif self:isEnemy(p) then
						if times == 2 and self:needToThrowArmor(p) then continue
						elseif (not self:doNotDiscard(p) or self:getDangerousCard(p) or self:getValuableCard(p)) then
							target = p  break end
					end
				end
			elseif times == 3 then
				if self:isEnemy(p) then
					if self:damageIsEffective(p, nil, self.player) and not self:getDamagedEffects(p, self.player)
					and not self:needToLoseHp(p, self.player) and ((self:isWeak(p) and p:getHp() < 3) or self:getOverflow() > 3)  then
						target = p  break end
				end
			elseif times == 4 then
				if self:isFriend(p) and p:isWounded() then
					target = p  break end
			end
		end
	end
	if target and #to_discard == times then
		use.card = sgs.Card_Parse("@DanshouCard=" .. table.concat(to_discard, "+"))
		if use.to then use.to:append(target) end
		return
	end
end

sgs.ai_use_priority.DanshouCard = sgs.ai_use_priority.Dismantlement + 2
sgs.ai_use_value.DanshouCard = sgs.ai_use_value.Dismantlement + 1

sgs.ai_card_intention.DanshouCard = function(self, card, from, tos)
	local num = card:subcardsLength()
	if num == 2 or num == 3 then
		sgs.updateIntentions(from, tos, 10)
	elseif num == 4 then
		sgs.updateIntentions(from, tos, -10)
	end
end

sgs.ai_choicemade_filter.cardChosen.danshou = sgs.ai_choicemade_filter.cardChosen.snatch


sgs.ai_skill_use["@@zongxuan"] = function(self, prompt)
	if self.top_draw_pile_id or self.player:getPhase() >= sgs.Player_Finish then return "." end
	local list = self.player:property("zongxuan"):toString():split("+")
	local valuable
	for _, id in ipairs(list) do
		local card_id = tonumber(id)
		local card = sgs.Sanguosha:getCard(card_id)
		if card:isKindOf("EquipCard") then
			for _, friend in ipairs(self.friends) do
				if not (card:isKindOf("Armor") and not friend:getArmor() and friend:hasSkills("bazhen|yizhong"))
					and (not self:getSameEquip(card, friend) or card:isKindOf("DefensiveHorse") or card:isKindOf("OffensiveHorse")
						or (card:isKindOf("Weapon") and self:evaluateWeapon(card) > self:evaluateWeapon(friend:getWeapon()) - 1)) then
					self.top_draw_pile_id = card_id
					return "@ZongxuanCard=" .. card_id
				end
			end
		elseif self:isValuableCard(card) and not valuable then
			valuable = card_id
		end
	end
	if valuable then
		self.top_draw_pile_id = valuable
		return "@ZongxuanCard=" .. valuable
	end
	return "."
end

sgs.ai_skill_playerchosen.zhiyan = function(self, targets)
	if self.top_draw_pile_id then
		local card = sgs.Sanguosha:getCard(self.top_draw_pile_id)
		if card:isKindOf("EquipCard") then
			self:sort(self.friends, "hp")
			for _, friend in ipairs(self.friends) do
				if (not self:getSameEquip(card, friend) or card:isKindOf("DefensiveHorse") or card:isKindOf("OffensiveHorse"))
					and not (card:isKindOf("Armor") and (friend:hasSkills("bazhen|yizhong") or self:evaluateArmor(card, friend) < 0)) then
					return friend
				end
			end
			if not (card:isKindOf("Armor") and (self.player:hasSkills("bazhen|yizhong") or self:evaluateArmor(card) < 0))
				and not (card:isKindOf("Weapon") and self.player:getWeapon() and self:evaluateWeapon(card) < self:evaluateWeapon(self.player:getWeapon()) - 1) then
				return self.player
			end
		else
			local cards = { card }
			local card, player = self:getCardNeedPlayer(cards)
			if player then
				return player
			else
				self:sort(self.friends)
				for _, friend in ipairs(self.friends) do
					if not self:needKongcheng(friend, true) and not hasManjuanEffect(friend) then return friend end
				end
			end
		end
	else
		self:sort(self.friends)
		for _, friend in ipairs(self.friends) do
			if not self:needKongcheng(friend, true) and not hasManjuanEffect(friend) then return friend end
		end
	end
	return nil
end

sgs.ai_playerchosen_intention.zhiyan = -60

sgs.ai_skill_invoke.zhuikong = sgs.ai_skill_invoke.noszhuikong

sgs.ai_skill_playerchosen.qiuyuan = function(self, targets)
	local targetlist = sgs.QList2Table(targets)
	self:sort(targetlist, "handcard")
	local enemy
	for _, p in ipairs(targetlist) do
		local jink = getKnownCard(p, self.player, "Jink", true, "he")
		if self:isEnemy(p) and (jink == 0 or (self:isWeak(p) and jink < 2)) then
			enemy = p
			break
		end
	end
	if enemy then return enemy end
	targetlist = sgs.reverse(targetlist)
	local friend
	for _, p in ipairs(targetlist) do
		local jink = getKnownCard(p, self.player, "Jink", true, "he")
		if self:isFriend(p) then
			if (self:needKongcheng(p) and p:getHandcardNum() == 1 and jink == 1)
				or (p:getCardCount() >= 2 and self:canLiuli(p, self.enemies))
				or self:needLeiji(p) or p:getHandcardNum() > 3 or jink >= 1 then
				friend = p
				break
			end
		end
	end
	if friend then return friend end
return nil
end

sgs.ai_skill_cardask["@qiuyuan-give"] = function(self, data, pattern, target)
	local give = true
	local huanghou = self.room:findPlayerBySkillName("qiuyuan")
	if self:isEnemy(huanghou) then
		if not (self:needKongcheng() and self.player:getHandcardNum() == 1) then
			give = false
		end
	elseif self:isFriend(huanghou) then
		if not self:isWeak(huanghou) and self:hasSkills("leiji|nosleiji") then
			give = false
		end
	end
	if give == true then
		local cards = sgs.QList2Table(self.player:getHandcards())
		self:sortByKeepValue(cards)
		for _, card in ipairs(cards) do
			if card:isKindOf("Jink") then
				return "$" .. card:getEffectiveId()
			end
		end
	end
	return "."
end

function SmartAI:hasQiuyuanEffect(from, to)
	if not from or not to:hasSkill("qiuyuan") then return false end
	for _, target in ipairs(self:getEnemies(to)) do
		if self:isFriend(target) then
			if (target:isKongcheng() and not (target:getHandcardNum() == 1 and self:needKongcheng(target, true))) 
			or self:isWeak(target) then
				return true
			end
		end
	end
	return
end

sgs.ai_skill_playerchosen.juece = function(self, targetlist)
	local targets = sgs.QList2Table(targetlist)
	self:sort(targets)
	local friends, enemies = {}, {}
	for _, target in ipairs(targets) do
		if self:cantbeHurt(target, self.player) or not self:damageIsEffective(target, nil, self.player) then continue end
		if self:isEnemy(target) then table.insert(enemies, target)
		elseif self:isFriend(target) then table.insert(friends, target) end
	end
	for _, enemy in ipairs(enemies) do
		if not self:getDamagedEffects(enemy, self.player) and not self:needToLoseHp(enemy, self.player) then return enemy end
	end
	for _, friend in ipairs(friends) do
		if self:getDamagedEffects(friend, self.player) and self:needToLoseHp(friend, self.player) then return friend end
	end
return nil
end

sgs.ai_playerchosen_intention.juece = function(self, from, to)
	if self:damageIsEffective(to, nil, from) and not self:getDamagedEffects(friend, self.player) and not self:needToLoseHp(friend, self.player) then
		sgs.updateIntention(from, to, 10)
	end
end

local fencheng_skill = {}
fencheng_skill.name = "fencheng"
table.insert(sgs.ai_skills, fencheng_skill)
fencheng_skill.getTurnUseCard = function(self)
	if self.player:getMark("@burn") == 0 then return false end
	return sgs.Card_Parse("@FenchengCard=.")
end

sgs.ai_skill_use_func.FenchengCard = function(card, use, self)
	local value = 0
	local neutral = 0
	local damage = { from = self.player, damage = 2, nature = sgs.DamageStruct_Fire }
	local lastPlayer = self.player
	for i, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		damage.to = p
		if self:damageIsEffective_(damage) then
			if sgs.evaluatePlayerRole(p, self.player) == "neutral" then neutral = neutral + 1 end
			local v = 4
			if (self:getDamagedEffects(p, self.player) or self:needToLoseHp(p, self.player)) and getCardsNum("Peach", p, self.player) + p:getHp() > 2 then
				v = v - 6
			elseif lastPlayer:objectName() ~= self.player:objectName() and lastPlayer:getCardCount(true) < p:getCardCount(true) then
				v = v - 4
			elseif lastPlayer:objectName() == self.player:objectName() and not p:isNude() then
				v = v - 4
			end
			if self:isFriend(p) then
				value = value - v - p:getHp() + 2
			elseif self:isEnemy(p) then
				value = value + v + p:getLostHp() - 1
			end
			if p:isLord() and p:getHp() <= 2
				and (self:isEnemy(p, lastPlayer) and p:getCardCount(true) <= lastPlayer:getCardCount(true)
					or lastPlayer:objectName() == self.player:objectName() and (not p:canDiscard(p, "he") or p:isNude())) then
				if not self:isEnemy(p) then
					if self:getCardsNum("Peach") + getCardsNum("Peach", p, self.player) + p:getHp() <= 2 then return end
				else
					use.card = card
					return
				end
			end
		end
	end

	if neutral > self.player:aliveCount() / 2 then return end
	if value > 0 then
		use.card = card
	end
end

sgs.ai_use_priority.FenchengCard = 9.1

sgs.ai_skill_discard.fencheng = function(self, discard_num, min_num, optional, include_equip)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByKeepValue(cards)
	local current = self.room:getCurrent()
	local damage = { from = current, damage = 2, nature = sgs.DamageStruct_Fire }
	local to_discard = {}
	local length = min_num
	local peaches = 0

	local nextPlayer = self.player:getNextAlive()
	if self:isEnemy(nextPlayer) and self.player:getCardCount(true) > nextPlayer:getCardCount(true) and self.player:getCardCount(true) > length then
		length = tonumber(nextPlayer:getCardCount(true))
	end

	for _, c in ipairs(cards) do
		if self.player:canDiscard(self.player, c:getEffectiveId()) then
			table.insert(to_discard, c:getEffectiveId())
			if isCard("Peach", c, self.player) then peaches = peaches + 1 end
			if #to_discard == length then break end
		end
	end

	if peaches > 2 then
		return {}
	elseif peaches == 2 and self.player:getHp() > 1 and length == min_num then
		for _, friend in ipairs(self.friends_noself) do
			damage.to = friend
			if friend:getHp() <= 2 and self:damageIsEffective_(damage) then return {} end
		end
	end

	if nextPlayer:isLord() and self.role ~= "rebel" and nextPlayer:getHandcardNum() < min_num
		and not self:getDamagedEffects(nextPlayer, current) and not self:needToLoseHp(nextPlayer, current) then
		if nextPlayer:getHp() + getCardsNum("Peach", nextPlayer, self.player) + self:getCardsNum("Peach") <= 2 then return {} end
		if self.player:getHp() > nextPlayer:getHp() and self.player:getHp() > 2 then return {} end
	end
	return to_discard
end

local mieji_skill = {}
mieji_skill.name = "mieji"
table.insert(sgs.ai_skills, mieji_skill)
mieji_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("MiejiCard") or self.player:isKongcheng() then return end
	return sgs.Card_Parse("@MiejiCard=.")
end

sgs.ai_skill_use_func.MiejiCard = function(card, use, self)
	local nextAlive = self.player:getNextAlive()
	local hasLightning, hasIndulgence, hasSupplyShortage
	local tricks = nextAlive:getJudgingArea()
	if not tricks:isEmpty() and not nextAlive:containsTrick("YanxiaoCard") and not nextAlive:hasSkill("qianxi") then
		local trick = tricks:at(tricks:length() - 1)
		if self:hasTrickEffective(trick, nextAlive) then
			if trick:isKindOf("Lightning") then hasLightning = true
			elseif trick:isKindOf("Indulgence") then hasIndulgence = true
			elseif trick:isKindOf("SupplyShortage") then hasSupplyShortage = true
			end
		end
	end

	local putcard
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	for _, card in ipairs(cards) do
		if card:isBlack() and card:isKindOf("TrickCard") then
			if hasLightning and card:getSuit() == sgs.Card_Spade and card:getNumber() >= 2 and card:getNumber() <= 9 then
				if self:isEnemy(nextAlive) then
					putcard = card break
				else continue
				end
			end
			if hasSupplyShortage and card:getSuit() == sgs.Card_Club then
				if self:isFriend(nextAlive) then
					putcard = card break
				else continue
				end
			end
			if not putcard then
				putcard = card break
			end
		end
	end

	local target
	for _, enemy in ipairs(self.enemies) do
		if self:needKongcheng(enemy) and enemy:getHandcardNum() <= 2 then continue end
		if not enemy:isNude()  then
			target = enemy break
		end
	end
	if not target then
		for _, friend in ipairs(self.friends_noself) do
			if self:needKongcheng(friend) and friend:getHandcardNum() < 2 and not friend:isKongcheng() then
				target = friend break
			end
		end
	end

	if putcard and target then
		use.card = sgs.Card_Parse("@MiejiCard="..putcard:getEffectiveId())
		if use.to then use.to:append(target) end
		return
	end

end

sgs.ai_use_priority.MiejiCard = sgs.ai_use_priority.Dismantlement + 1

sgs.ai_card_intention.MiejiCard = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		if self:needKongcheng(to) and to:getHandcardNum() <= 2 then continue end
		sgs.updateIntention(from, to, 10)
	end
end

sgs.ai_skill_cardask["@@miejidiscard!"] = function(self, prompt)
	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
	self:sortByKeepValue(cards)
	local trick = {}
	local nontrick = {}
	local discard = {}
	for _,card in ipairs(cards) do
		if card:isKindOf("TrickCard") then
			table.insert(trick, card)
		else
			table.insert(nontrick, card)
		end
	end
	if #cards <= 2 then return "." end
	if self:needToThrowArmor() and #nontrick >= 2 then
		table.insert(discard, self.player:getArmor())
		if nontrick[1] ~= discard[1] then
			table.insert(discard, nontrick[1])
		else
			table.insert(discard, nontrick[2])
		end
	end
	if #trick == 0 then
		for _,card in ipairs(nontrick) do
			table.insert(discard, card)
			if #discard == 2 or #discard == #nontrick then
				break
			end
		end
	end
	if #nontrick == 0 and #trick >= 1 then
		table.insert(discard, trick[1])
	end
	if #discard > 0 then
		return "$"..table.concat(discard:getEffectiveId(), "+")
	end
return "."
end

local dingpin_skill = {}
dingpin_skill.name = "dingpin"
table.insert(sgs.ai_skills, dingpin_skill)
dingpin_skill.getTurnUseCard = function(self, inclusive)
	sgs.ai_use_priority.DingpinCard = 0
	if not self.player:canDiscard(self.player, "h") or self.player:getMark("dingpin") == 0xE then return false end
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if not p:hasFlag("dingpin") and p:isWounded() then
			if not self:toTurnOver(self.player) then sgs.ai_use_priority.DingpinCard = 8.9 end
			return sgs.Card_Parse("@DingpinCard=.")
		end
	end
end
sgs.ai_skill_use_func.DingpinCard = function(card, use, self)
	local cards = {}
	local cardType = {}
	for _, card in sgs.qlist(self.player:getHandcards()) do
		if bit32.band(self.player:getMark("dingpin"), bit32.lshift(1, card:getTypeId())) == 0 then
			table.insert(cards, card)
			if not table.contains(cardType, card:getTypeId()) then table.insert(cardType, card:getTypeId()) end
		end
	end
	for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
		local card = sgs.Sanguosha:getCard(id)
		if bit32.band(self.player:getMark("dingpin"), bit32.lshift(1, card:getTypeId())) == 0 then
			table.insert(cards, card)
			if not table.contains(cardType, card:getTypeId()) then table.insert(cardType, card:getTypeId()) end
		end
	end
	if #cards == 0 then return end
	self:sortByUseValue(cards, true)
	if self:isValuableCard(cards[1]) then return end

	if #cardType > 1 or not self:toTurnOver(self.player) then
		self:sort(self.friends)
		for _, friend in ipairs(self.friends) do
			if not friend:hasFlag("dingpin") and friend:isWounded() then
				use.card = sgs.Card_Parse("@DingpinCard=" .. cards[1]:getEffectiveId())
				if use.to then use.to:append(friend) end
				return
			end
		end
	end

end

sgs.ai_use_priority.DingpinCard = 0
sgs.ai_card_intention.DingpinCard = -10

sgs.ai_skill_invoke.faen = function(self, data)
	local player = data:toPlayer()
	if self:needKongcheng(player, true) then return not self:isFriend(player) end
	return self:isFriend(player)
end

sgs.ai_choicemade_filter.skillInvoke.faen = function(self, player, promptlist)
	local target = findPlayerByObjectName(self.room, promptlist[#promptlist - 1])
	if not target then return end
	local yes = promptlist[#promptlist] == "yes"
	if self:needKongcheng(target, true) then
		sgs.updateIntention(player, target, yes and 10 or -10)
	else
		sgs.updateIntention(player, target, yes and -10 or 10)
	end
end

sgs.ai_skill_invoke.sidi = true

sgs.ai_skill_use["@@sidi"] = function(self)
	local current = self.room:getCurrent()
	local slash = sgs.Sanguosha:cloneCard("slash")
	if self:isEnemy(current) then
		if (getCardsNum("Slash", current, self.player) >= 1 or self.player:getPile("sidi"):length() > 2)
		and not (current:hasWeapon("crossbow") or current:hasSkill("paoxiao")) then
			for _, player in sgs.qlist(self.room:getOtherPlayers(current)) do
				if self:isFriend(player) and player:distanceTo(current) <= current:getAttackRange()
				and self:slashIsEffective(slash, player) and (self:isWeak(player) or self.player:getPile("sidi"):length() > 1) then
					return "@SidiCard=" .. self.player:getPile("sidi"):first()
				end
			end
		end
	end
	return "."
end

sgs.ai_skill_use["@@shenduan"] = function(self)
	local ids = self.player:property("shenduan"):toString():split("+")
	for _, id in ipairs(ids) do
		local card = sgs.Sanguosha:getCard(id)
		if self.player:isCardLimited(card, sgs.Card_MethodUse) then continue end
		local card_str = ("supply_shortage:shenduan[%s:%s]=%d"):format(card:getSuitString(), card:getNumberString(), id)
		local ss = sgs.Card_Parse(card_str)
		local dummy_use = { isDummy = true , to = sgs.SPlayerList() }
		self:useCardSupplyShortage(ss, dummy_use)
		if dummy_use.card and not dummy_use.to:isEmpty() then
			return card_str .. "->" .. dummy_use.to:first():objectName()
		end
	end
	return "."
end

sgs.ai_skill_invoke.yonglve = function(self)
	local current = self.room:getCurrent()
	if self:isFriend(current) and self:askForCardChosen(current, "h", "dummyReason", sgs.Card_MethodDiscard) then
		if not self:slashIsEffective(sgs.Sanguosha:cloneCard("slash"), current, self.player) then return true end
		if not self:isWeak(current) or getKnownCard(current, self.player, "Jink") > 0 then return true end
	elseif self:isEnemy(current) then
		if self:askForCardChosen(current, "h", "dummyReason", sgs.Card_MethodDiscard) then return true end
		for _, card in sgs.qlist(current:getJudgingArea()) do
			if card:isKindOf("SupplyShortage") and (current:getHandcardNum() > 4 or current:containsTrick("indulgence")) then
				sgs.ai_skill_cardchosen.yonglve = card:getEffectiveId()
				return true
			elseif card:isKindOf("Indulgence") and current:getHandcardNum() + self:ImitateResult_DrawNCards(current) <= self:getOverflow(current, true) then
				sgs.ai_skill_cardchosen.yonglve = card:getEffectiveId()
				return true
			end
		end
		if self:isWeak(current) and current:getHp() == 1 and (sgs.card_lack[current:objectName()]["Jink"] == 1 or getCardsNum("Jink", current, self.player) == 0)
			and self:slashIsEffective(sgs.Sanguosha:cloneCard("slash"), current, self.player) then
			sgs.ai_skill_cardchosen.yonglve = self:getCardRandomly(current, "j")
			return true
		end
	end
	return false
end

sgs.ai_skill_invoke.qiangzhi = function(self)
	return not self:needKongcheng(self.player, true)
end

sgs.ai_skill_playerchosen.qiangzhi = function(self, targetlist)
	local slash = self:getCard("Slash")
	if slash then
		local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
		self:useCardSlash(slash, dummy_use)
		if dummy_use.card and not dummy_use.to:isEmpty() then
			local target = dummy_use.to:first()
			if targetlist:contains(target) and target:getHandcardNum() - getKnownNum(target, self.player) <= 2 and target:getHandcardNum() <= 2 then
				return target
			end
		end
	end

	local cardType = { trick = 0, basic = 0, equip = 0 }
	local turnUse = self:getTurnUse()
	for _, card in ipairs(turnUse) do
		if card:getType() ~= "skill_card" then cardType[card:getType()] = cardType[card:getType()] + 1 end
	end

	local target = {}
	local max_trick, max_basic, max_equip = 0, 0, 0
	for _, player in sgs.qlist(targetlist) do
		local known = getKnownCard(player, self.player, "TrickCard")
		if cardType.trick > 0 and known / player:getHandcardNum() > max_trick then
			max_trick = known
			target.trick = player
		end

		known = getKnownCard(player, self.player, "BasicCard")
		if cardType.basic > 0 and known / player:getHandcardNum() > max_basic then
			max_basic = known
			target.basic = player
		end

		known = getKnownCard(player, self.player, "EquipCard")
		if cardType.equip > 0 and known / player:getHandcardNum() > max_equip then
			max_equip = known
			target.equip = player
		end
	end

	local max_value = math.max(cardType.trick * max_trick, cardType.basic * max_basic, cardType.equip * max_equip)
	if max_value > 0 then
		for cardype, value in pairs(cardType) do
			if max_value == value then return target[cardype] end
		end
	end

	self:sort(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		if targetlist:contains(enemy) then return enemy end
	end

	local players = sgs.QList2Table(self.room:getOtherPlayers(self.player))
	self:sort(players)
	for _, p in ipairs(players) do
		if not self:isFriend(p) and targetlist:contains(p) then return p end
	end

	self:sort(self.friends_noself, "handcard")
	self.friends_noself = sgs.reverse(self.friends_noself)
	for _, friend in ipairs(self.friends_noself) do
		if targetlist:contains(friend) then return friend end
	end

	return fasle
end

sgs.ai_skill_invoke.xiantu = function(self)
	local current = self.room:getCurrent()
	if self:isFriend(current) then
		if current:isLord() and sgs.isLordInDanger() then return true end
		if self.role == "renegade" and not self:needToThrowArmor() then return false end
		if sgs.evaluatePlayerRole(current, self.player) == "renegade" and not self:needToThrowArmor() then return false end
		for _, enemy in ipairs(self.enemies) do
			if self:isWeak(enemy) and enemy:getHp() == 1 and not self:slashProhibit(nil, enemy, current)
				and (not sgs.isJinkAvailable(current, enemy) or getCardsNum("Jink", enemy, self.player) == 0 or sgs.card_lack[enemy:objectName()] == 1)
				and (getCardsNum("Slash", current, self.player) >= 1 or self:getCardsNum("Slash") > 0) then
				return true
			end
		end
		if not self.player:isWounded() and self:isWeak(current) then return true end
	end
end

sgs.ai_skill_discard.xiantu = function(self, discard_num, min_num, optional, include_equip)
	local to_exchange = {}
	local current = self.room:getCurrent()
	if self.player:isWounded() and self.player:hasArmorEffect("silver_lion") then table.insert(to_exchange, self.player:getArmor():getEffectiveId()) end
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByUseValue(cards)
	if getCardsNum("Slash", current, self.player) < 1 then
		for _, card in ipairs(cards) do
			if not isCard("Peach", card, self.player) and isCard("Slash", card, current) then
				table.insert(to_exchange, card:getEffectiveId())
			end
		end
	end

	if #to_exchange == 2 then return to_exchange end

	for _, card in ipairs(cards) do
		if self.player:hasEquip(card) and self.player:hasSkills(sgs.lose_equip_skill) then
			table.insert(to_exchange, card:getEffectiveId())
			if #to_exchange == 2 then return to_exchange end
			break
		end
	end
	for _, card in ipairs(cards) do
		table.insert(to_exchange, card:getEffectiveId())
		if #to_exchange == 2 then return to_exchange end
	end
end

sgs.ai_skill_playerchosen.zhongyong = function(self, targetlist)
	self:sort(self.friends)
	if self:getCardsNum("Slash") > 0 then
		for _, friend in ipairs(self.friends) do
			if not targetlist:contains(friend) or friend:objectName() == self.player:objectName() then continue end
			if getCardsNum("Jink", friend, self.player) < 1 or sgs.card_lack[friend:objectName()]["Jink"] == 1 then
				return friend
			end
		end
		if self:getCardsNum("Jink") == 0 and targetlist:contains(self.player) then return self.player end
	end
	local lord = self.room:getLord()
	if self:isFriend(lord) and sgs.isLordInDanger() and targetlist:contains(lord) and getCardsNum("Jink", lord, self.player) < 2 then return lord end
	if self.role == "renegade" and targetlist:contains(self.player) then return self.player end
	return self:findPlayerToDraw(true, 1) or self.friends[1]
end

local shenxing_skill = {}
shenxing_skill.name = "shenxing"
table.insert(sgs.ai_skills, shenxing_skill)
shenxing_skill.getTurnUseCard = function(self)
	sgs.ai_use_priority.ShenxingCard = 3
	if self.player:getCardCount(true) < 2 then return false end
	if self:getOverflow() <= 0 then return false end
	if self:isWeak() and self:getOverflow() <= 1 then return false end
	return sgs.Card_Parse("@ShenxingCard=.")
end
sgs.ai_skill_use_func.ShenxingCard = function(card, use, self)
	local unpreferedCards = {}
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)

	local red_num, black_num = 0, 0
	if self.player:getHp() < 3 then
		local zcards = self.player:getCards("he")
		local use_slash, keep_jink, keep_analeptic, keep_weapon = false, false, false
		local keep_slash = self.player:getTag("JilveWansha"):toBool()
		for _, zcard in sgs.qlist(zcards) do
			if self.player:isCardLimited(zcard, sgs.Card_MethodDiscard) then continue end
			if not isCard("Peach", zcard, self.player) and not isCard("ExNihilo", zcard, self.player) then
				local shouldUse = true
				if isCard("Slash", zcard, self.player) and not use_slash then
					local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
					self:useBasicCard(zcard, dummy_use)
					if dummy_use.card then
						if keep_slash then shouldUse = false end
						if dummy_use.to then
							for _, p in sgs.qlist(dummy_use.to) do
								if p:getHp() <= 1 then
									shouldUse = false
									if self.player:distanceTo(p) > 1 then keep_weapon = self.player:getWeapon() end
									break
								end
							end
							if dummy_use.to:length() > 1 then shouldUse = false end
						end
						if not self:isWeak() then shouldUse = false end
						if not shouldUse then use_slash = true end
					end
				end
				if zcard:getTypeId() == sgs.Card_TypeTrick then
					local dummy_use = { isDummy = true }
					self:useTrickCard(zcard, dummy_use)
					if dummy_use.card then shouldUse = false end
				end
				if zcard:getTypeId() == sgs.Card_TypeEquip and not self.player:hasEquip(zcard) then
					local dummy_use = { isDummy = true }
					self:useEquipCard(zcard, dummy_use)
					if dummy_use.card then shouldUse = false end
					if keep_weapon and zcard:getEffectiveId() == keep_weapon:getEffectiveId() then shouldUse = false end
				end
				if self.player:hasEquip(zcard) and zcard:isKindOf("Armor") and not self:needToThrowArmor() then shouldUse = false end
				if self.player:hasTreasure("wooden_ox") then shouldUse = false end
				if self.player:hasEquip(zcard) and zcard:isKindOf("DefensiveHorse") and not self:needToThrowArmor() then shouldUse = false end
				if isCard("Jink", zcard, self.player) and not keep_jink then
					keep_jink = true
					shouldUse = false
				end
				if self.player:getHp() == 1 and isCard("Analeptic", zcard, self.player) and not keep_analeptic then
					keep_analeptic = true
					shouldUse = false
				end
				if shouldUse then
					if (table.contains(unpreferedCards, zcard:getId())) then continue end
					table.insert(unpreferedCards, zcard:getId())
					if self.room:getCardPlace(zcard:getId()) == sgs.Player_PlaceHand then
						if zcard:isRed() then red_num = red_num + 1
						else black_num = black_num + 1 end
					end
				end
				if #unpreferedCards == 2 then
					use.card = sgs.Card_Parse("@ShenxingCard=" .. table.concat(unpreferedCards, "+"))
					return
				end
			end
		end
	end

	local red = self:getSuitNum("red")
	local black = self:getSuitNum("black")
	if red - red_num <= 2 - #unpreferedCards then
		for _, c in ipairs(cards) do
			if c:isRed() and (not isCard("Peach", c, self.player) or not self:findFriendsByType(sgs.Friend_Weak) and #cards > 1) then
				if self.player:isCardLimited(c, sgs.Card_MethodDiscard) then continue end
				if table.contains(unpreferedCards, c:getId()) then continue end
				table.insert(unpreferedCards, c:getId())
			end
		end
	elseif black - black_num <= 2 - #unpreferedCards then
		for _, c in ipairs(cards) do
			if c:isBlack() and (not isCard("Peach", c, self.player) or not self:findFriendsByType(sgs.Friend_Weak) and #cards > 1) then
				if self.player:isCardLimited(c, sgs.Card_MethodDiscard) then continue end
				if table.contains(unpreferedCards, c:getId()) then continue end
				table.insert(unpreferedCards, c:getId())
			end
		end
	end

	if #unpreferedCards < 2 then
		for _, c in ipairs(cards) do
			if not self.player:isCardLimited(c, sgs.Card_MethodDiscard) then
				if table.contains(unpreferedCards, c:getId()) then continue end
				table.insert(unpreferedCards, c:getId())
			end
			if #unpreferedCards == 2 then break end
		end
	end

	if #unpreferedCards == 2 then
		use.card = sgs.Card_Parse("@ShenxingCard=" .. table.concat(unpreferedCards, "+"))
		sgs.ai_use_priority.ShenxingCard = 0
		return
	end

end

sgs.ai_use_priority.ShenxingCard = 3

sgs.ai_skill_use["@@bingyi"] = function(self)

	local cards = self.player:getHandcards()
	if cards:length() == 0 then return "." end

	if cards:first():isBlack() then
		for _, c in sgs.qlist(cards) do
			if c:isRed() then return "." end
		end
	elseif cards:first():isRed() then
		for _, c in sgs.qlist(cards) do
			if c:isBlack() then return "." end
		end
	end

	self:sort(self.friends)
	local targets = {}
	for _, friend in ipairs(self.friends) do
		if not hasManjuanEffect(friend) and not self:needKongcheng(friend, true) then
			table.insert(targets, friend:objectName())
		end
		if #targets == self.player:getHandcardNum() then break end
	end

	if #targets < self.player:getHandcardNum() then
		for _, enemy in ipairs(self.enemies) do
			if self:needKongcheng(enemy, true) then
				table.insert(targets, enemy:objectName())
			end
		end
	end

	if #targets > 0 then
		return "@BingyiCard=.->" .. table.concat(targets, "+")
	end
end

sgs.ai_card_intention.BingyiCard = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		if self:needKongcheng(to, true) then sgs.updateIntention(from, to, 10)
		elseif hasManjuanEffect(to) then continue
		else sgs.updateIntention(from, to, -10) end
	end
end

sgs.ai_skill_playerchosen.zenhui = function(self, targetlist)
	self.zenhui_collateral = nil
	local use = self.player:getTag("zenhui"):toCardUse()
	local dummy_use = { isDummy = true, to = sgs.SPlayerList(), current_targets = {}, extra_target = 99 }
	local target = use.to:first()
	if not target then return end
	table.insert(dummy_use.current_targets, target:objectName())
	self:useCardByClassName(use.card, dummy_use)
	if dummy_use.card and use.card:getClassName() == dummy_use.card:getClassName() and not dummy_use.to:isEmpty() then
		if use.card:isKindOf("Collateral") then
			assert(dummy_use.to:length() == 2)
			local player = dummy_use.to:first()
			if targetlist:contains(player) and (self:isFriend(player) or self:hasTrickEffective(use.card, target, player)) then
				self.zenhui_collateral = dummy_use.to:at(1)
				return player
			end
			return false
		elseif use.card:isKindOf("Slash") then
			for _, player in sgs.qlist(dummy_use.to) do
				if targetlist:contains(player) and (self:isFriend(player) or not self:slashProhibit(use.card, target, player)) then
					return player
				end
			end
		elseif use.card:isKindOf("FireAttack") then
			for _, player in sgs.qlist(dummy_use.to) do
				if targetlist:contains(player) and not self:isFriend(player, target) and not self:isFriend(player) and self:hasTrickEffective(use.card, target, player) then
					return player
				end
			end
			dummy_use.to = sgs.QList2Table(dummy_use.to)
			self:sort(dummy_use.to, "handcard")
			dummy_use.to = sgs.reverse(dummy_use.to)
			local suits = {}
			for _, c in sgs.qlist(self.player:getHandcards()) do
				if c:getSuit() <= 3 and not table.contains(suits, c:getSuitString()) then table.insert(suits, c:getSuitString()) end
			end
			if #suits <= 2 or self:getSuitNum("heart", false, target) > 0 and self:getSuitNum("heart") == 0 then
				for _, player in ipairs(dummy_use.to) do
					if self:isFriend(player) and targetlist:contains(player) then return player end
				end
			end
		elseif use.card:isKindOf("Duel") then
			for _, player in sgs.qlist(dummy_use.to) do
				if targetlist:contains(player) and (self:isFriend(player) or self:hasTrickEffective(use.card, target, player)) then
					return player
				end
			end
		elseif use.card:isKindOf("Drowning") then
			for _, player in sgs.qlist(dummy_use.to) do
				if targetlist:contains(player) and (self:isFriend(player) or self:hasTrickEffective(use.card, target, player)) then
					return player
				end
			end
		elseif use.card:isKindOf("Dismantlement") then
			for _, player in sgs.qlist(dummy_use.to) do
				if targetlist:contains(player) and self:isFriend(player) then
					return player
				end
			end
			for _, player in sgs.qlist(dummy_use.to) do
				if targetlist:contains(player) then
					if not self:isFriend(player, target) then
						return player
					elseif not self:needToThrowArmor(target)
						and (target:getJudgingArea():isEmpty()
							or (not target:containsTrick("indulgence")
								and not target:containsTrick("supply_shortage")
								and not (target:containsTrick("lightning") and self:getFinalRetrial(target, "lightning") == 1))) then
						return player
					end
				end
			end
		elseif use.card:isKindOf("Snatch") then
			for _, player in sgs.qlist(dummy_use.to) do
				if targetlist:contains(player) and self:isFriend(player) then
					return player
				end
			end
			local friend = self:findPlayerToDraw(false)
			if friend and targetlist:contains(friend) and self:hasTrickEffective(use.card, target, friend) then
				return friend
			end
		else
			self.room:writeToConsole("playerchosen.zenhui->" .. use.card:getClassName() .. "?")
		end
	end
	return false
end

sgs.ai_skill_playerchosen.zenhui_collateral = function(self, targetlist)
	if self.zenhui_collateral then return self.zenhui_collateral end
	self.room:writeToConsole(debug.traceback())
	return targetlist:at(math.random(0, targetlist:length() - 1))
end

sgs.ai_skill_cardask["@zenhui-give"] = function(self, data)
	local use = data:toCardUse()
	local target = use.to:first()
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByKeepValue(cards)
	local id = cards[1]:getEffectiveId()

	if use.card:isKindOf("Snatch") then
		if self:isFriend(use.from) then
			if self:askForCardChosen(self.player, "ej", "dummyReason") or not use.from:getAI() then
				return "."
			else
				self:sortByUseValue(cards)
				return cards[1]:getEffectiveId()
			end
		elseif not self:hasTrickEffective(use.card, self.player, use.from) then
			return "."
		end
		return id
	elseif use.card:isKindOf("Slash") then
		if self:slashProhibit(use.card, self.player, use.from)
			or not self:hasHeavySlashDamage(use.from, use.card, self.player) and (self:getDamagedEffects(self.player, use.from, true) or self:needToLoseHp(self.player, use.from)) then
			return "."
		elseif self:isFriend(target) then
			if not self:slashIsEffective(use.card, target, self.player) then
				return id
			end
		elseif not self:isValuableCard(cards[1]) or self:isWeak() or self:getCardsNum("Jink") == 0 or not sgs.isJinkAvailable(use.from, self.player, use.card) then
			return id
		end
		return "."
	elseif use.card:isKindOf("Dismantlement") then
		if not self:hasTrickEffective(use.card, self.player, use.from) then
			return "."
		elseif self:isFriend(use.from) and self:askForCardChosen(self.player, "ej", "dummyReason") then
			return "."
		end
		return id
	elseif use.card:isKindOf("Duel") then
		if self:getDamagedEffects(self.player, use.from) or self:needToLoseHp(self.player, use.from) then
			return "."
		elseif not self:hasTrickEffective(use.card, self.player, use.from) then
			return "."
		elseif self:isFriend(use.from) then
			if (self:getDamagedEffects(use.from, self.player) or self:needToLoseHp(use.from, self.player))
				and self:getCardsNum("Slash") - (cards[1]:isKindOf("Slash") and 1 or 0) > 0 then
				return "."
			end
		else
			if self:getCardsNum("Slash") - (cards[1]:isKindOf("Slash") and 1 or 0) > getCardsNum("Slash", use.from, self.player) then
				return "."
			end
		end
		return id
	elseif use.card:isKindOf("FireAttack") then
		if self:getDamagedEffects(self.player, use.from) or self:needToLoseHp(self.player, use.from) then
			return "."
		end
		return id
	elseif use.card:isKindOf("Collateral") then
		local victim = self.player:getTag("collateralVictim"):toPlayer()
		if sgs.ai_skill_cardask["collateral-slash"](self, nil, nil, victim, use.from) ~= "."
			and self:isFriend(use.from) or not self:isValuableCard(cards[1]) then
			return "."
		end
		return id
	elseif use.card:isKindOf("Drowning") then
		if self:getDamagedEffects(self.player, use.from) or self:needToLoseHp(self.player, use.from) or self:needToThrowArmor() then
			return "."
		elseif self:isValuableCard(cards[1]) and self:isEnemy(use.from) then
			return "."
		end
		return id
	else
		self.room:writeToConsole("@zenhui-give->" .. use.card:getClassName() .. "?")
	end

	return "."
end

sgs.ai_skill_cardask["@jiaojin"] = function(self, data)
	local damage = data:toDamage()
	if self:damageIsEffective_(damage) and not self:getDamagedEffects(damage.to, damage.from, damage.card and damage.card:isKindOf("Slash"))
		and not self:needToLoseHp(damage.to, damage.from, damage.card and damage.card:isKindOf("Slash")) then
		local cards = sgs.QList2Table(self.player:getCards("he"))
		self:sortByKeepValue(cards)
		for _, c in ipairs(cards) do
			if c:getTypeId() == sgs.Card_TypeEquip then return c:getEffectiveId() end
		end
	end
	return "."
end

sgs.ai_skill_choice.qieting = function(self, choices)
	local target = self.room:getCurrent()
	local id = self:askForCardChosen(target, "e", "dummyReason")
	if id then
		for i = 0, 4 do
			if target:getEquip(i) and target:getEquip(i):getEffectiveId() == id and string.find(choices, i) then
				return i
			end
		end
	end
	return "draw"
end


local xianzhou_skill = {}
xianzhou_skill.name = "xianzhou"
table.insert(sgs.ai_skills, xianzhou_skill)
xianzhou_skill.getTurnUseCard = function(self)
	if self.player:getMark("@handover") <= 0 then return end
	if self.player:getEquips():isEmpty() then return end
	return sgs.Card_Parse("@XianzhouCard=.")
end
sgs.ai_skill_use_func.XianzhouCard = function(card, use, self)
	if self:isWeak() then
		for _, friend in ipairs(self.friends_noself) do
			if friend:hasSkills(sgs.need_equip_skill) then
				use.card = card
				if use.to then use.to:append(friend) end
				return
			end
		end
		for _, friend in ipairs(self.friends_noself) do
			if not hasManjuanEffect(friend) then
				use.card = card
				if use.to then use.to:append(friend) end
				return
			end
		end
		self:sort(self.friends)
		for _, target in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			local canUse = true
			for _, friend in ipairs(self.friends) do
				if target:inMyAttackRange(friend) and self:damageIsEffective(friend, nil, target)
					and not self:getDamagedEffects(friend, target) and not self:needToLoseHp(friend, target) then
					canUse = false
					break
				end
			end
			if canUse then
				use.card = card
				if use.to then use.to:append(target) end
				return
			end
		end
	end
	if not self.player:isWounded() then
		local killer
		self:sort(self.friends_noself)
		for _, target in sgs.qlist(self.room:getOtherPlayers(self.player)) do
			local canUse = false
			for _, friend in ipairs(self.friends_noself) do
				if friend:inMyAttackRange(target) and self:damageIsEffective(target, nil, friend)
					and not self:needToLoseHp(target, friend) and self:isWeak(target) then
					canUse = true
					killer = friend
					break
				end
			end
			if canUse then
				use.card = card
				if use.to then use.to:append(killer) end
				return
			end
		end
	end

	if #self.friends_noself == 0 then return end
	if self.player:getEquips():length() > 2 or self.player:getEquips():length() > #self.enemies and sgs.turncount > 2 then
		local function cmp_AttackRange(a, b)
			local ar_a = a:getAttackRange()
			local ar_b = b:getAttackRange()
			if ar_a == ar_b then
				return sgs.getDefense(a) > sgs.getDefense(b)
			else
				return ar_a > ar_b
			end
		end
		table.sort(self.friends_noself, cmp_AttackRange)
		use.card = card
		if use.to then use.to:append(self.friends_noself[1]) end
	end
end

sgs.ai_use_priority.XianzhouCard = 4.9
sgs.ai_card_intention.XianzhouCard = function(self, card, from, tos)
	if not from:isWounded() then sgs.updateIntentions(from, tos, -10) end
end

sgs.ai_skill_use["@xianzhou"] = function(self, prompt)
	local prompt = prompt:split(":")
	local num = prompt[#prompt]
	local current = self.room:getCurrent()
	if self:isWeak(current) and self:isFriend(current) then return "." end
	local targets = {}
	self:sort(self.enemies, "hp")
	for _, enemy in ipairs(self.enemies) do
		if self.player:inMyAttackRange(enemy) and self:damageIsEffective(enemy, nil, self.player)
			and not self:getDamagedEffects(enemy, self.player) and not self:needToLoseHp(enemy, self.player) then
			table.insert(targets, enemy:objectName())
			if #targets == tonumber(num) then break end
		end
	end
	if #targets < tonumber(num) then
		self:sort(self.friends_noself)
		self.friends_noself = sgs.reverse(self.friends_noself)
		for _, friend in ipairs(self.friends_noself) do
			if self.player:inMyAttackRange(friend) and self:damageIsEffective(friend, nil, self.player)
				and not self:getDamagedEffects(friend, self.player) and not self:needToLoseHp(friend, self.player) then
				table.insert(targets, friend:objectName())
				if #targets == tonumber(num) then break end
			end
		end
	end
	if #targets < tonumber(num) then
		for _, target in sgs.qlist(self.room:getAlivePlayers()) do
			if not self:isFriend(target) and self:isWeak(target) then
				table.insert(targets, target:objectName())
			end
		end
	end

	if #targets > 0 and #targets == tonumber(num) then
		return "@XianzhouDamageCard=.->" .. table.concat(targets, "+")
	end
	return "."
end

sgs.ai_card_intention.XianzhouDamageCard = function(self, card, from, tos)
	for _, to in ipairs(tos) do
		if self:damageIsEffective(to, nil, from) and not self:getDamagedEffects(to, from) and not self:needToLoseHp(to, from) then
			sgs.updateIntention(from, to, 10)
		end
	end
end

sgs.ai_skill_invoke.jianying = function(self)
	return not self:needKongcheng(self.player, true)
end

sgs.ai_skill_playerchosen.youdi = function(self, targets)
	self.youdi_obtain_to_friend = false
	local throw_armor = self:needToThrowArmor()
	if throw_armor and #self.friends_noself > 0 and self.player:getCardCount(true) > 1 then
		for _, friend in ipairs(self.friends_noself) do
			if friend:canDiscard(self.player, self.player:getArmor():getEffectiveId())
				and (self:needToThrowArmor(friend) or (self:needKongcheng(friend) and friend:getHandcardNum() == 1)
					or friend:getHandcardNum() <= self:getLeastHandcardNum(friend)) then
				return friend
			end
		end
	end

	local valuable, dangerous = self:getValuableCard(self.player), self:getDangerousCard(self.player)
	local slash_ratio = 0
	if not self.player:isKongcheng() then
		local slash_count = 0
		for _, c in sgs.qlist(self.player:getHandcards()) do
			if c:isKindOf("Slash") then slash_count = slash_count + 1 end
		end
		slash_ratio = slash_count / self.player:getHandcardNum()
	end
	if not valuable and not dangerous and slash_ratio > 0.45 then return nil end

	self:sort(self.enemies, "defense")
	self.enemies = sgs.reverse(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		if enemy:canDiscard(self.player, "he") and not self:doNotDiscard(enemy, "he") then
			if (valuable and enemy:canDiscard(self.player, valuable)) or (dangerous and enemy:canDiscard(self.player, dangerous)) then
				if (self:getValuableCard(enemy) or self:getDangerousCard(enemy)) and sgs.getDefense(enemy) > 8 then return enemy end
			elseif not enemy:isNude() then return enemy
			end
		end
	end
end

sgs.ai_choicemade_filter.cardChosen.youdi_obtain = sgs.ai_choicemade_filter.cardChosen.snatch