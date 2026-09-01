
# -*- coding: utf-8 -*-
from QuModLibs.Server import *

@Listen(Events.PlayerAttackEntityEvent)
def test(args):
    playerId = args["playerId"]
    victimId = args["victimId"]

    comp = serverApi.GetEngineCompFactory().CreateItem(playerId)
    itemDict = comp.GetPlayerItem(2, 0) # 物品信息字典
    if itemDict and itemDict["newItemName"] == "minecraft:diamond":
        identifier = Entity(victimId).Identifier
        Entity.CreateEngineEntityByTypeStr(identifier, Entity(victimId).Pos, (0, 0), Entity(victimId).Dm)