#include "QuModMEX.h"
#include <fstream>
// By Zero123

// 匹配分析Python代码中引用的import/from import语句块（字面量）
std::vector<std::string> QuModMEX::extractImportedModules(const std::string& sourceCode)
{
    std::vector<std::string> modules;
    std::istringstream stream(sourceCode);
    std::string line;

    // 正则表达式模式，匹配 import 和 from ... import 语句
    static const std::regex importPattern(R"(^\s*import\s+([\w\.]+))");
    static const std::regex fromImportPattern(R"(^\s*from\s+((?:\.+)?[\w\.]+)\s+import)");

    try
    {
        while (std::getline(stream, line))
        {
            std::smatch match;
            if (std::regex_search(line, match, importPattern))
            {
                modules.push_back(match[1].str());
            }
            else if (std::regex_search(line, match, fromImportPattern))
            {
                modules.push_back(match[1].str());
            }
        }
    }
    catch (const std::regex_error& e)
    {
        // 捕获正则表达式异常，返回已收集的模块列表
        std::cerr << "Regex error: " << e.what() << "\n";
        return modules;
    }
    catch (const std::exception& e)
    {
        // 捕获其他异常，返回已收集的模块列表
        std::cerr << "Error: " << e.what() << "\n";
        return modules;
    }

    return modules;
}

QuModMEX::ModuleView::ModuleView(const std::string& path) : path(path) {}

QuModMEX::ModuleView::ModuleView(std::string&& path): path(std::move(path)) {}

std::filesystem::path QuModMEX::ModuleView::toPath() const
{
    std::string formatPath{ path };
    std::replace(formatPath.begin(), formatPath.end(), '.', '/');
    return std::filesystem::path(formatPath);
}

bool QuModMEX::ModuleView::operator==(const ModuleView& other) const
{
    return this->toPath() == other.toPath();
}

// 根据特定路径获得当前所在Module名
std::string QuModMEX::ModuleView::getCurrentModuleName(const std::filesystem::path& targetPath) const
{
    auto myPath = toPath();
    std::filesystem::path absPath = std::filesystem::relative(myPath, targetPath);
    // 获取第一级{xxx}/.../...名称
    std::string moduleName = absPath.generic_string();
    if (moduleName.starts_with("."))
    {
        return std::string();
    }
    size_t pos = moduleName.find_first_of('/');
    if(pos != std::string::npos)
	{
		moduleName = moduleName.substr(0, pos);
		return moduleName;
	}
    return std::string();
}

std::vector<QuModMEX::ModuleView> QuModMEX::extractImportedModulesView(const std::string& sourceCode, const std::filesystem::path& objectPath)
{
    std::vector<std::string> modules = extractImportedModules(sourceCode);
    std::vector<ModuleView> moduleViews;
    for (const auto& module : modules)
    {
        std::string fullPath = module;
        // 相对路径处理
        if (fullPath.starts_with("."))
        {
            std::string currentPath = objectPath.parent_path().string();
            std::string relativePath = fullPath.substr(1);
            size_t dotCount = 0;
            size_t pos = 0;
            // 根据相对运算符"."的数量确定父目录
            while (pos < relativePath.size() && relativePath[pos] == '.')
            {
                ++dotCount;
                ++pos;
            }
            std::string parentPath = currentPath;
            for (size_t i = 0; i < dotCount; ++i)
            {
                parentPath = std::filesystem::path(parentPath).parent_path().string();
            }
            fullPath = (std::filesystem::path(parentPath) / std::filesystem::path(relativePath.substr(pos))).generic_string();
        }
        moduleViews.emplace_back(std::move(fullPath));
    }
    return moduleViews;
}

// QuMod项目分析器
QuModMEX::QuModLibsAnalyzer::QuModLibsAnalyzer(const std::filesystem::path& libsPath) : libsPath(libsPath) {}

static std::string readFileText(const std::filesystem::path& f)
{
    if (!std::filesystem::exists(f))
    {
        return std::string();
    }
    // 二进制模式读取文件
    std::ifstream file(f, std::ios::binary);
    if (!file.is_open())
    {
        return std::string();
    }
    std::string content(std::istreambuf_iterator<char>(file), {});
    return content;
}

// 获取所有模块(QuModLibs/Modules/*)
std::vector<std::filesystem::path> QuModMEX::QuModLibsAnalyzer::getAllModules() const
{
    auto modulePath = libsPath / "Modules";
    if (!std::filesystem::exists(modulePath))
    {
        // 无效的目录
        return std::vector<std::filesystem::path>();
    }
    std::vector<std::filesystem::path> modules;
    for (const auto& entry : std::filesystem::directory_iterator(modulePath))
    {
        if (entry.is_directory() && std::filesystem::exists(entry.path() / "__init__.py"))
        {
            modules.push_back(entry.path());
        }
    }
    return modules;
}

// 获取所有模块引用关系视图(QuModLibs/Modules/*)
std::vector<QuModMEX::ModuleRelationView> QuModMEX::QuModLibsAnalyzer::getAllModulesRelationViews() const
{
    auto allModulePath = getAllModules();
    // 模块引用管理表
    std::unordered_map<std::string, ModuleRelationView> moduleRelations;
    auto modulePath = libsPath / "Modules";

    // 计算模块引用关系
    for (const auto& path : allModulePath)
    {
        auto name = path.filename().string();
        if (moduleRelations.find(name) == moduleRelations.end())
        {
            moduleRelations[name] = ModuleRelationView(name);
        }
        auto& targetModule = moduleRelations[name];
        // 遍历该目录下所有文件
        for (auto& entry : std::filesystem::recursive_directory_iterator(path))
        {
            if (!entry.is_regular_file())
            {
                // 仅处理文件而非文件夹
                continue;
            }
            auto& filePath = entry.path();
            if (filePath.extension() != ".py")
            {
                // 仅处理.py文件
                continue;
            }
            auto mFile = QuModMEX::extractImportedModulesView(
                readFileText(filePath),
                filePath
            );
            for (const auto& m : mFile)
            {
                auto moduleName = m.getCurrentModuleName(modulePath);
                if (moduleName.empty())
                {
                    continue;
                }
                // 添加引用模块
                targetModule.addRefModules(moduleName);

                if (moduleRelations.find(moduleName) == moduleRelations.end())
                {
                    moduleRelations[moduleName] = ModuleRelationView(moduleName);
                }
                // 添加被引用模块
                auto& refModule = moduleRelations[moduleName];
                refModule.addRefedModules(name);
            }
        }
    }

    // 将最终计算结果移动到列表中
    std::vector<ModuleRelationView> movedValues;
    movedValues.reserve(moduleRelations.size());

    for (auto& [key, value] : moduleRelations)
    {
        movedValues.push_back(std::move(value));
    }
    return movedValues;
}

QuModMEX::ModuleRelationView::ModuleRelationView(const std::string& moduleName) : moduleName(moduleName) {}

// 获取模块名字
std::string QuModMEX::ModuleRelationView::getName() const
{
    return moduleName;
}

// 查询引用模块
std::unordered_set<std::string>& QuModMEX::ModuleRelationView::getRefModules()
{
    return refModules;
}

// 查询引用模块
const std::unordered_set<std::string>& QuModMEX::ModuleRelationView::getRefModules() const
{
    return refModules;
}

std::unordered_set<std::string>& QuModMEX::ModuleRelationView::getRefedModules()
{
    return refedModules;
}

// 添加引用模块
bool QuModMEX::ModuleRelationView::addRefModules(const std::string& ref)
{
    if (ref == moduleName)
    {
        return false;
    }
    if (refModules.find(ref) == refModules.end())
    {
        refModules.insert(ref);
        return true;
    }
    return false;
}

bool QuModMEX::ModuleRelationView::addRefModules(std::string&& ref)
{
    if (ref == moduleName)
    {
        return false;
    }
    if (refModules.find(ref) == refModules.end())
    {
        refModules.insert(std::move(ref));
        return true;
    }
    return false;
}

// 添加被引用模块
bool QuModMEX::ModuleRelationView::addRefedModules(const std::string& refed)
{
    if (refed == moduleName)
    {
        return false;
    }
    if (refedModules.find(refed) == refedModules.end())
    {
        refedModules.insert(refed);
        return true;
    }
    return false;
}

bool QuModMEX::ModuleRelationView::addRefedModules(std::string&& refed)
{
    if (refed == moduleName)
    {
        return false;
    }
    if (refedModules.find(refed) == refedModules.end())
    {
        refedModules.insert(std::move(refed));
        return true;
    }
    return false;
}

bool QuModMEX::ModuleRelationView::removeRefModules(const std::string& ref)
{
    if (refModules.find(ref) != refModules.end())
    {
        refModules.erase(ref);
        return true;
    }
    return false;
}

bool QuModMEX::ModuleRelationView::removeRefedModules(const std::string& refed)
{
    if (refedModules.find(refed) != refedModules.end())
    {
        refedModules.erase(refed);
        return true;
    }
    return false;
}

bool QuModMEX::ModuleRelationView::hasRef(const std::string& ref) const
{
    return refModules.find(ref) != refModules.end();
}

// 获取引用模块数量
unsigned int QuModMEX::ModuleRelationView::getRefCount() const
{
    return static_cast<unsigned int>(refModules.size());
}

// 获取被引用模块数量
unsigned int QuModMEX::ModuleRelationView::getRefedCount() const
{
    return static_cast<unsigned int>(refedModules.size());
}

// 模块引用关系视图管理器
QuModMEX::ModuleRelationViewManager::ModuleRelationViewManager(const std::vector<ModuleRelationView>& viewVc)
{
    for(const auto& view : viewVc)
	{
		viewDatas[view.getName()] = view;
	}
}

QuModMEX::ModuleRelationViewManager::ModuleRelationViewManager(std::vector<ModuleRelationView>&& viewVc)
{
	for(auto& view : viewVc)
	{
		viewDatas[view.getName()] = std::move(view);
	}
}

// 检查模块是否存在
bool QuModMEX::ModuleRelationViewManager::hasModule(const std::string& moduleName) const
{
    if (viewDatas.find(moduleName) != viewDatas.end())
    {
        return true;
    }
    return false;
}

QuModMEX::ModuleRelationView& QuModMEX::ModuleRelationViewManager::getModuleView(const std::string& moduleName)
{
    if(hasModule(moduleName))
	{
		return viewDatas[moduleName];
	}
    throw std::runtime_error("找不到特定模块: " + moduleName);
}

// 计算可以安全剔除白名单外的模块视图
std::vector<std::string> QuModMEX::ModuleRelationViewManager::getRemoveUnwantedModules(const std::unordered_set<std::string>& whiteList)
{
    std::vector<std::string> removedModules;
    std::unordered_set<std::string> allWhiteList {whiteList};
    // 统计所有白名单模块的所有深度引用
    for (const auto& whiteModule : allWhiteList)
    {
        auto deepRef = getAllRef(whiteModule);
        allWhiteList.insert(deepRef.begin(), deepRef.end());
    }
    
    for (const auto& [name, view] : viewDatas)
    {
        if(allWhiteList.find(name) != allWhiteList.end())
		{
			// 模块在白名单中 跳过检查
            continue;
		}
        // 递归分析模块是否引用白名单模块
        bool hasRefed = false;
        for(const auto& whiteModule : allWhiteList)
		{
            if (hasRefModule(whiteModule, name))
            {
                // 存在引用特定模块
                hasRefed = true;
                break;
            }
		}
        // 如果模块没有引用白名单中的模块，则可以安全删除
        if (!hasRefed)
        {
            removedModules.push_back(name);
        }
    }
    return removedModules;
}

// 获取所有视图对象
std::unordered_map<std::string, QuModMEX::ModuleRelationView>& QuModMEX::ModuleRelationViewManager::getViewDatas()
{
    return viewDatas;
}

// 管理器内删除特定模块
bool QuModMEX::ModuleRelationViewManager::removeModule(const std::string& moduleName, bool safeCheck)
{
    if (!hasModule(moduleName))
    {
        return false;
    }
    auto& m = viewDatas[moduleName];
    if (safeCheck && m.getRefedCount() > 0)
    {
        // 该模块被引用，无法安全删除
		return false;
    }
    // 解引用
    for (const auto& ref : m.getRefModules())
	{
        if (hasModule(ref))
        {
            viewDatas[ref].removeRefedModules(moduleName);
        }
	}
    // 删除模块
	viewDatas.erase(moduleName);
	return true;
}

bool QuModMEX::ModuleRelationViewManager::hasRefModule(const std::string& moduleName, const std::string& refModuleName) const
{
    if (moduleName == refModuleName)
    {
        return true;
    }
    if(hasModule(moduleName) && hasModule(refModuleName))
	{
		auto& m = viewDatas.at(moduleName);
		if (m.hasRef(refModuleName))
		{
			return true;
		}
		// 递归检查引用关系
		for (auto& ref : m.getRefModules())
		{
            if (hasRefModule(ref, refModuleName))
            {
                return true;
            }
		}
	}
    return false;
}

// 获取一个模块的所有引用
std::unordered_set<std::string> QuModMEX::ModuleRelationViewManager::getAllRef(const std::string& moduleName) const
{
    std::unordered_set<std::string> vc;
    if (hasModule(moduleName))
    {
        for(const auto& ref : viewDatas.at(moduleName).getRefModules())
		{
			vc.insert(ref);
			auto refed = getAllRef(ref);
			vc.insert(refed.begin(), refed.end());
		}
    }
    return vc;
}
