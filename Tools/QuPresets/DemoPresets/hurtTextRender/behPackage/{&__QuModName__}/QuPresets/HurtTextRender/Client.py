# -*- coding: utf-8 -*-
from ...QuModLibs.Client import *
from ...QuModLibs.Modules.Services.Client import BaseBusiness, BaseService
from random import random

RENDER_HURT_EVENT_KEY = "__{}_hurtValue__".format(ModDirName)

class HurtText(BaseBusiness):
    """ 文字受伤业务 """
    def __init__(self, hurtText = "", pos = (0, 0, 0)):
        BaseBusiness.__init__(self)
        self.hurtText = hurtText
        self.pos = tuple(v + random() * 0.85 for v in pos)
        self.color = (1.0, 1.0, 1.0, 1.0)
        self.textId = None
        self.scale = 8.5
        self.minScale = 3.0
        self.removeTime = 1.0
    
    def onCreate(self):
        BaseBusiness.onCreate(self)
        comp = clientApi.GetEngineCompFactory().CreateTextBoard(levelId)
        self.textId = comp.CreateTextBoardInWorld(str(self.hurtText), self.color, (0, 0, 0, 0), True)
        comp.SetBoardPos(self.textId, self.pos)
        comp.SetBoardScale(self.textId, (self.scale, self.scale))
        self.addTimer(
            BaseBusiness.Timer(
                self.stopBusiness, time = self.removeTime
            )
        )

    def onTick(self):
        BaseBusiness.onTick(self)
        mut = 25.0
        self.scale = max(self.scale - 0.033 * mut, self.minScale)
        comp = clientApi.GetEngineCompFactory().CreateTextBoard(levelId)
        comp.SetBoardScale(self.textId, (self.scale, self.scale))
    
    def onStop(self):
        BaseBusiness.onStop(self)
        comp = clientApi.GetEngineCompFactory().CreateTextBoard(levelId)
        comp.RemoveTextBoard(self.textId)

class TextRenderService(BaseService):
    """ 文字渲染服务 """
    @classmethod
    def addHurtText(cls, textObj):
        # type: (HurtText) -> None
        cls.access().addBusiness(
            textObj
        )

@BaseService.Init
class HURT_TEXT_SERVICE(BaseService):
    """ 客户端受伤文本服务 """
    @BaseService.Listen(Events.AddEntityClientEvent)
    def AddEntityClientEvent(self, args={}):
        """ 实体增加事件 """
        entityId = args["id"]
        comp = clientApi.GetEngineCompFactory().CreateModAttr(entityId)
        comp.RegisterUpdateFunc(RENDER_HURT_EVENT_KEY, self.EntityHurtEvent)

    @BaseService.Listen(Events.RemoveEntityClientEvent)
    def RemoveEntityClientEvent(self, args={}):
        """ 实体移除事件 """
        entityId = args["id"]
        comp = clientApi.GetEngineCompFactory().CreateModAttr(entityId)
        comp.UnRegisterUpdateFunc(RENDER_HURT_EVENT_KEY, self.EntityHurtEvent)

    def EntityHurtEvent(self, args):
        """ 实体受伤触发 """
        newValue = args["newValue"]
        entityId = args["entityId"]
        TextRenderService.addHurtText(
            HurtText(
                str(int(newValue)), Entity(entityId).Vec3Pos.addTuple((0, 0.92, 0)).addVec(Entity(playerId).Vec3DirFromRot.multiplyOf(-1.0)).getTuple()
            )
        )