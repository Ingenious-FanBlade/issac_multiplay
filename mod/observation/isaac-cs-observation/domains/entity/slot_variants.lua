-- 变体说明只解释实际值，不以预期替换 Getter 返回。
return {
 [1]={name="SlotMachine",prize="3..12奖励类别，13..24无奖励，9未见使用"},
 [2]={name="BloodDonation"},[3]={name="FortuneTelling"},
 [4]={name="Beggar",donation="投入硬币数",timeout="支付后1<<16/1<<17增量，奖励后归0"},
 [5]={name="DemonBeggar",donation="投入心量",timeout="支付后1<<16/1<<17增量，奖励后归0"},
 [6]={name="ShellGame",prize="PickupVariant",sprite="Prizes图层"},
 [7]={name="KeyMaster"},[8]={name="DonationMachine",persistent_interaction="不由观测器触发"},
 [9]={name="BombBum",prize="1硬币，2心，3道具",trigger="爆炸时30"},
 [10]={name="RestockMachine",trigger="成功重抽加1，爆炸可能设11"},
 [11]={name="GreedDonation",persistent_interaction="不由观测器触发"},
 [12]={name="DressingTable"},[13]={name="BatteryBum",donation="随机增加至3，出奖归0"},
 [14]={name="HomeClosetPlayer"},[15]={name="HellGame",prize="PickupVariant及PrizeCollectible"},
 [16]={name="CraneGame",prize="PrizeCollectible",timeout="前三奖1<<16、1<<17、两者之和，仍有30帧增量／倒数"},
 [17]={name="Confessional"},[18]={name="RottenBeggar",donation="随机增加至3，出奖归0"}
}
