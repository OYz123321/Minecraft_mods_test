# CopyZombieServerSystem.py
# -*- coding: utf-8 -*-

import mod.server.extraServerApi as serverApi

ServerSystem = serverApi.GetServerSystemCls()
CompFactory = serverApi.GetEngineCompFactory()
ItemPosType = serverApi.GetMinecraftEnum().ItemPosType


class CopyZombieServerSystem(ServerSystem):

    def __init__(self, namespace, systemName):
        ServerSystem.__init__(self, namespace, systemName)

        self.ListenForEvent(
            serverApi.GetEngineNamespace(),
            serverApi.GetEngineSystemName(),
            "PlayerAttackEntityEvent",
            self,
            self.OnPlayerAttackEntity
        )

    def OnPlayerAttackEntity(self, args):
        playerId = args["playerId"]
        victimId = args["victimId"]

        # 1. 判断攻击目标是否为普通僵尸
        victimType = CompFactory.CreateEngineType(victimId).GetEngineTypeStr()
        if victimType != "minecraft:zombie":
            return

        # 2. 判断玩家主手是否为钻石剑
        itemComp = CompFactory.CreateItem(playerId)
        carriedItem = itemComp.GetPlayerItem(ItemPosType.CARRIED, 0)

        if not carriedItem:
            return

        # 兼容不同 SDK 物品字典字段
        itemName = carriedItem.get("newItemName", carriedItem.get("itemName"))
        if itemName != "minecraft:diamond_sword":
            return

        # 3. 获取被攻击僵尸的坐标与维度
        posComp = CompFactory.CreatePos(victimId)
        pos = posComp.GetFootPos()

        dimensionComp = CompFactory.CreateDimension(victimId)
        dimensionId = dimensionComp.GetEntityDimensionId()

        # 4. 在原僵尸旁边生成一只新僵尸
        spawnPos = (pos[0] + 1.0, pos[1], pos[2] + 1.0)
        self.CreateEngineEntityByTypeStr(
            "minecraft:zombie",
            spawnPos,
            (0, 0),
            dimensionId
        )

    def Destroy(self):
        self.UnListenForEvent(
            serverApi.GetEngineNamespace(),
            serverApi.GetEngineSystemName(),
            "PlayerAttackEntityEvent",
            self,
            self.OnPlayerAttackEntity
        )