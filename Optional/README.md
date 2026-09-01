# QuModLibs 可选扩展
该目录包含QuModLibs的可选扩展模块，可以根据需要选择性地合并到项目中使用。 

## EventRegistry 模块
EventRegistry模块提供了一套适用于Py2面向对象的事件注册解决方案。

```python
# -*- coding: utf-8 -*-
from .QuModLibs.Client import *
from .QuModLibs.Modules.EventRegistry.ClientBus import SubscribeEvent, GameEvents

@SubscribeEvent
def onTick(event=GameEvents.OnScriptTickClient()):
    print("Tick event received!")

@SubscribeEvent
def onJump(event=GameEvents.ClientJumpButtonPressDownEvent()):
    event.continueJump = False
    print("Jump event received!")
```
由于Py2中并不支持类型注解，作为替代，事件类型需要通过默认参数的方式进行指定。

## MultiplayerLobby 模块
练级大厅模块提供了一组可以快速使用的成品解决方案，这可能不是最优解但作为可选项允许用户自行组装使用。

> 在旧版 `release/1.4-full` 中 `MultiplayerLobby` 作为内置包使用，介于大多数场景无关此功能故迁移到可选板块中。

## Camera 模块
提供了一组可以快速使用的运镜系统，但由于相关实现已过时现作为可选项单独分离。