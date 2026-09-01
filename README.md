# QuModLibs v1.4
适用于网易MC_MOD开发的免费开源框架 (如需摘录部分源代码到其他同类别项目请署名原作者)。
> 交流反馈群: Q494731530

## 版本说明
当前版本已废弃 `MINI` 精简版，并将部分不常用模块移至 `Optional/Modules` 可选部分；同时移除了内置补全库，完全改为使用系统补全库。

如需旧版的完整包体，请查阅 `release/1.4-full` 分支。

### 补全库安装
- Python 2：`pip install mc-netease-sdk`
- Python 3：`pip install mc-netease-sdk-nyrev`

## 创建项目
您可以通过以下步骤创建一个基于`QuModLibs`的网易MCMOD项目。
### 项目结构
推荐放置在 `Scripts/QuModLibs`目录下，但不是必须：
```
├── 行为包
│   └── 脚本目录
│       └── QuModLibs(库目录)
|           ├── __init__.py
|           └── ...
│       ├── __init__.py
│       ├── modMain.py
│       ├── Server.py
│       └── Client.py
```
```python
# modMain.py
# -*- coding: utf-8 -*-
from .QuModLibs.QuMod import * # 导入 QuModLibs.QuMod 的全部功能
myMod = EasyMod()              # 便捷的MOD构建类

# 服务端与客户端注册 将加载: 脚本目录/Server.py 和 脚本目录/Client.py
myMod.Server("Server")
myMod.Client("Client")
```

### 开发范式建议
QuModLibs 推荐以 **Feature（业务特性）** 为边界组织代码：每个 Feature 独立承载一组完整业务能力，并通过 `import` 完成事件注册、RPC 声明、服务初始化等副作用加载，从而保持入口清晰、职责收敛、加载顺序可控。

> 以下两种结构均为推荐范式，可根据业务规模和模块复杂度选择使用。

| 推荐范式 | 适用场景 | 核心思想 |
| --- | --- | --- |
| **范式一：端侧入口平铺** | 中小型 Feature、逻辑边界清晰的功能 | 在 Feature 包内直接划分 `Server.py` / `Client.py` / `Common.py`，以端侧文件作为业务入口 |
| **范式二：端侧目录聚合** | 大型 Feature、子模块较多或需要持续扩展的功能 | 在 Feature 内继续按端侧拆分目录，由各端侧目录的 `__init__.py` 统一向下导入并管理子模块 |

#### 范式一：端侧入口平铺

该结构将一个 Feature 拆分为服务端、客户端与公共逻辑三个入口文件，适合业务量适中、模块层级不深的功能。上层 `Server.py` / `Client.py` 只需导入对应 Feature 的端侧入口，Feature 内部负责完成自身初始化。

```text
<Feature>/
├── __init__.py
├── Server.py   # 服务端入口：事件监听、服务注册、服务端 RPC 等
├── Client.py   # 客户端入口：UI、渲染、客户端事件、客户端 RPC 等
└── Common.py   # 双端公共逻辑：常量、工具函数、数据结构等
```

**推荐用法：**

```python
# Server.py
from .Features.ExampleFeature import Server

# Client.py
from .Features.ExampleFeature import Client
```

#### 范式二：端侧目录聚合

当单个 Feature 内部继续拆分出多个子能力时，建议将端侧入口升级为端侧目录。每个端侧目录通过自身的 `__init__.py` 作为聚合入口，集中导入下级模块，让 `import <Feature>.ServerSide` / `import <Feature>.ClientSide` 即可触发该端侧全部业务加载。

```text
<Feature>/
├── __init__.py
├── ServerSide/
│   ├── __init__.py  # 服务端聚合入口：统一 import 下级服务端模块
│   ├── xxx.py       # 服务端子模块
│   └── xxx.py
├── ClientSide/
│   ├── __init__.py  # 客户端聚合入口：统一 import 下级客户端模块
│   ├── xxx.py       # 客户端子模块
│   └── xxx.py
└── Common/
    ├── __init__.py  # 公共聚合入口：统一 import 下级公共模块
    ├── xxx.py       # 公共子模块
    └── xxx.py
```

**推荐用法：**

```python
# Server.py
from .Features.ExampleFeature import ServerSide

# Client.py
from .Features.ExampleFeature import ClientSide
```

该范式强调“**入口聚合，向下管理**”：对外只暴露稳定的端侧入口，对内由 `__init__.py` 维护子模块加载关系，既能降低跨业务耦合，也能避免初始化逻辑散落在多个上层入口中。

### 事件监听
通过`@Listen`装饰器注册事件监听。
```python
# Server/Client.py
from .QuModLibs.Server import *
# from .QuModLibs.Client import *

@Listen("OnScriptTickServer")
def OnScriptTickServer(_={}):
    print("game tick")

@Listen("OnCarriedNewItemChangedServerEvent")
def OnCarriedNewItemChangedServerEvent(args={}):
    # type: (dict) -> None
    playerId = args["playerId"] # 触发事件的玩家
    comp = serverApi.GetEngineCompFactory().CreateCommand(levelId)
    comp.SetCommand("/say 我切换了手持物品", playerId)

@DestroyFunc
def onGameClose():
    print("游戏端侧关闭时触发, 等价于Destroy")
```

### RPC通信
通过`@AllowCall`与`Call`实现服务端与客户端的通信。
#### 声明函数
所有需要远程调用的函数必须使用`@AllowCall`装饰器声明。
```python
# Client.py
@AllowCall
def testFunc():
    # 声明的函数将被登记到通信调用中，以便远程调用
    pass
```

#### 调用函数
QuMod提供了封装的`Call`函数用于跨端调用特定函数。
```python
# Server.py
# 服务端需要指定玩家Id调用特定客户端 其中 "*" 作为保留字段用于全体玩家广播
Call(playerId, "testFunc", ...args)
```

#### 批量调用
QuMod提供了封装的`MultiClientsCall`函数可用于批量发包调用。
```python
# MultiClientsCall为服务端独占功能 用于给多个玩家同时发包调用对立客户端函数
# 该方法相比for循环+Call的批量发包性能更好
MultiClientsCall([playerId1, playerId2, ...], "testFunc", ...args)
```

#### 数据包来源
通过使用`@InjectRPCPlayerId`装饰器可在被调用函数中捕获发起调用的玩家Id。
```python
# 服务端独占
from .QuModLibs.Server import *

@AllowCall
@InjectRPCPlayerId
def testFunc(playerId, ...args):
    # playerId为发起调用的玩家Id 会被InjectRPCPlayerId自动插入到第一个参数上
    pass
```
### IMP即初始化
QuMod推崇`import`即初始化的理念，推荐在功能模块加载时完成必要的初始化工作。

### Tools工具模块
`QuModLibs/Tools`提供了一组工具集，可帮助开发者快速实现某些功能，或进行项目优化。

## 更多功能
请参考`QuModLibs`官网文档、讨论群交流，或查看各模块的源代码注释。