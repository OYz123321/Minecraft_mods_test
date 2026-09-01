# -*- coding: utf-8 -*-
from mod.common.mod import Mod
import mod.server.extraServerApi as serverApi

@Mod.Binding(name="QuModLibs", version="0.0.1")
class QuModLibs(object):

    @Mod.InitServer()
    def InitServer(self):
        # 替换为你的系统的实际导入路径：
        # 命名空间: "QuModLibs"
        # 系统名称: "CopyZombieServerSystem"
        # 脚本路径: "Scripts.Server.CopyZombieServerSystem"
        serverApi.RegisterSystem(
            "QuModLibs",
            "CopyZombieServerSystem",
            "Scripts.Server.CopyZombieServerSystem"
        )

    @Mod.DestroyServer()
    def DestroyServer(self):
        pass