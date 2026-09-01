import bs4
import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin
lambda: "By Zero123"
"""
自动化爬虫事件分析
依赖库:
    pip install requests
    pip install beautifulsoup4
"""
__all__ = [
    "EventArgsDesc",
    "EventDesc",
    "EventDoc",
    "GEN_EVENT_PAGE_URLS",
    "EVENT_PAGE_WALK",
    "getPageName",
    "AUTO_GEN_ALL_EVENTS",
]

class EventArgsDesc:
    """
    事件参数描述对象
    """
    def __init__(self,
            attrName: str,
            attrType: str,
            attrDesc: str = "",
            *_,
        ) -> None:
        self.attrName = attrName
        self.attrType = attrType
        self.attrDesc = attrDesc
    
    def __repr__(self) -> str:
        return f"EventArgsDesc(attrName={self.attrName}, attrType={self.attrType}, attrDesc={self.attrDesc})"

class EventDesc:
    """ 事件描述对象 """
    def __init__(self, tag: bs4.Tag):
        self.mTag = tag
        self.liList: list[bs4.Tag] = [v for v in tag.contents if v.name == "li"]

    def getDescText(self) -> str:
        # return self.mTag.find("li").find_all("p")[-1].text.strip()
        return self.liList[0].find_all("p")[-1].text.strip()
    
    def getReturnText(self) -> str:
        """ 获取返回值描述 """
        # lis = self.mTag.find_all("li")
        # if len(lis) < 3:
        #     # 没有返回值描述
        #     return ""
        # return lis[2].find_all("p")[-1].text.strip()
        if len(self.liList) < 3:
            # 没有返回值描述
            return ""
        return self.liList[2].find_all("p")[-1].text.strip()
    
    def getArgsTable(self) -> list[tuple[str, ...]]:
        table = self.liList[1].find("table")
        if not table:
            return []
        args = []
        thead = table.find("thead")
        tbody = table.find("tbody")
        tr = thead.find("tr")
        args.append(tuple(th.get_text(strip=True) for th in tr.find_all("th")))  # 添加表头
        # 处理表体
        for tr in tbody.find_all("tr"):
            args.append(tuple(td.get_text(strip=True) for td in tr.find_all("td")))
        return args
    
    def getArgsDesc(self) -> list[EventArgsDesc]:
        """ 获取事件参数描述 """
        argsTable = self.getArgsTable()
        if not argsTable or len(argsTable[0]) < 3:
            return []
        argsDesc = []
        for row in argsTable[1:]:
            argsDesc.append(EventArgsDesc(*row))
        return argsDesc

class EventDoc:
    """ 事件文档对象 """
    def __init__(self, emBuff: list[bs4.Tag]):
        self.emBuff = emBuff

    def getSideType(self) -> int:
        """ 获取事件端侧类型
            0. 客户端
            1. 服务端
            2. 双端
        """
        typeNames = self.getSideTypeName()
        if "客户端" in typeNames and "服务端" in typeNames:
            return 2
        elif "服务端" in typeNames:
            return 1
        return 0

    def getSideTypeName(self) -> list[str]:
        """ 获取事件端侧类型文本 """
        return [v.text.strip() for v in self.emBuff[2].find_all("span")]

    def getEventName(self) -> str:
        """ 获取游戏事件名 """
        text = str(self.emBuff[0].text)
        return text[text.find("#") + 1:].strip()

    def getEventDesc(self) -> EventDesc:
        """ 获取事件描述 """
        index = 4
        if index >= len(self.emBuff):
            raise RuntimeError("事件参数异常")
        for i in range(index, len(self.emBuff)):
            if self.emBuff[i].name == "ul":
                return EventDesc(self.emBuff[i])
        raise RuntimeError("事件描述不存在")
        # return EventDesc(self.emBuff[4])

def GEN_EVENT_PAGE_URLS():
    """
    获取EVENTS页面表
    """
    res = requests.get("https://mc.163.com/dev/mcmanual/mc-dev/mcdocs/1-ModAPI/%E4%BA%8B%E4%BB%B6/%E4%BA%8B%E4%BB%B6%E7%B4%A2%E5%BC%95%E8%A1%A8.html")
    u8Text = res.content.decode("utf-8")
    parser = BeautifulSoup(u8Text, "html.parser")
    eventContent = parser.find("div", class_="theme-default-content content__default")
    for table in eventContent.find_all("table"):
        tbody = table.find("tbody")
        # 拿到每一组table下第一个事件标签
        tr = tbody.find("tr")    
        # 找到目标跳转页面
        href = tr.find("a")["href"]
        targetUrl = href[:href.rfind("#")]
        fullUrl = urljoin(res.url, targetUrl)
        yield fullUrl

def EVENT_PAGE_WALK(url: str):
    """ 爬虫分析Event页面 """
    res = requests.get(url)
    u8Text = res.content.decode("utf-8")
    parser = BeautifulSoup(u8Text, "html.parser")
    content = parser.find("div", class_="theme-default-content content__default")
    # 分析页面内容
    buffer = []
    for node in content:
        if node.name == "h2":
            if buffer:
                yield buffer[::]
            buffer.clear()
            buffer.append(node)
        elif buffer:
            buffer.append(node)
    if buffer:
        yield buffer[::]

def getPageName(url: str) -> str:
    """ 基于URL获取PAGENAME """
    return url[url.rfind("/") + 1:-len(".html")].strip()

def AUTO_GEN_ALL_EVENTS() -> dict[str, list[EventDoc]]:
    """ 自动化生成所有事件的描述对象"""
    allEvents: dict[str, list[EventDoc]] = {}
    for url in GEN_EVENT_PAGE_URLS():
        eventList = []
        allEvents[getPageName(url)] = eventList
        for emBuff in EVENT_PAGE_WALK(url):
            event = EventDoc(emBuff)
            eventList.append(event)
    return allEvents

if __name__ == "__main__":
    for url in GEN_EVENT_PAGE_URLS():
        print(f"Type: {getPageName(url)}")
        for emBuff in EVENT_PAGE_WALK(url):
            event = EventDoc(emBuff)
            print(event.getEventName())
            print(event.getSideTypeName())
            eventDesc = event.getEventDesc()
            print(eventDesc.getDescText())
            print(eventDesc.getArgsDesc())
            print(eventDesc.getReturnText())
            # if event.getEventName() == "EntityStopRidingEvent":
            #     exit(0)
            print()