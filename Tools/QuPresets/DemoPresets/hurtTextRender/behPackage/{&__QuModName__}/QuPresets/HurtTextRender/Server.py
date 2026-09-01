# -*- coding: utf-8 -*-
from ...QuModLibs.Server import *
from ...QuModLibs.Modules.Services.Server import BaseService
from random import random

RENDER_HURT_EVENT_KEY = "__{}_hurtValue__".format(ModDirName)

@BaseService.Init
class HURT_TEXT_SERVICE(BaseService):
    """ 服务端受伤文本服务 """
    @BaseService.Listen(Events.ActuallyHurtServerEvent)
    def ActuallyHurtServerEvent(self, args = {}):
        data = Events.ActuallyHurtServerEvent(args)
        damage = data.damage
        entityId = data.entityId
        comp = serverApi.GetEngineCompFactory().CreateModAttr(entityId)
        comp.SetAttr(
            RENDER_HURT_EVENT_KEY, damage + random() / 100.0
        )
