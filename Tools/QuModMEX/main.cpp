#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <regex>
#include "utils/EnvEncode.h"
#include "QuModMEX.h"
// By Zero123

// 列表转字符串
static std::string listToString(const std::vector<std::string>& vc)
{
	std::string result = "[";
	for (const auto& str : vc) {
		result.append(str+", ");
	}
	if(result.size() >= 2) {
		result.erase(result.end() - 2, result.end());
	}
	result.append("]");
	return result;
}

// set转字符串
static std::string listToString(const std::unordered_set<std::string>& vc)
{
	// set转vector
	std::vector<std::string> vec(vc.begin(), vc.end());
	return listToString(vec);
}

// 按空格化分字符串参数
static std::vector<std::string> splitString(const std::string& str)
{
	std::vector<std::string> result;
	std::istringstream iss(str);
	std::string token;
	while (iss >> token) {
		result.push_back(token);
	}
	return result;
}

// 刷新输入缓冲区
static void flushCin()
{
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static std::string readLine(std::istream& in = std::cin)
{
	std::string line;
	std::getline(std::cin, line);
	return line;
}

// 获取用户输入的参数
static std::vector<std::string> getUserInputArgs()
{
	return splitString(readLine());
}

// 删除空Python目录
static void removeNullPythonDirs(const std::filesystem::path& path)
{
	if(!std::filesystem::is_directory(path)) {
		return;
	}
	unsigned int count = 0;
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		// 忽略 __init__.py 文件
		if (std::filesystem::is_regular_file(entry) && entry.path().filename() != "__init__.py") {
			count++;
		} else if (std::filesystem::is_directory(entry)) {
			count++;
		}
	}
	if (count == 0) {
		std::cout << "删除空目录: " << path.generic_string() << "\n";
		std::filesystem::remove_all(path);
		return;
	}
}

// 白名单处理模式
static void whiteMode(const std::filesystem::path& quModPath, const std::unordered_set<std::string>& targets)
{
	auto analyzer = QuModMEX::QuModLibsAnalyzer(quModPath);
	auto manager = QuModMEX::ModuleRelationViewManager { analyzer.getAllModulesRelationViews() };
	std::cout << "保留目标: " << listToString(targets) << "\n";
	auto canRemove = manager.getRemoveUnwantedModules(targets);
	std::cout << "根据分析，可安全移除（按下回车执行）: " << listToString(canRemove) << "\n";
	system("pause");
	auto mPath = quModPath / "Modules";
	for (const auto& m : canRemove) {
		auto path = mPath / m;
		if (std::filesystem::exists(path)) {
			std::cout << "删除模块: " << path.generic_string() << "\n";
			std::filesystem::remove_all(path);
		}
		else {
			std::cout << "模块不存在: " << path.generic_string() << "\n";
		}
	}
	removeNullPythonDirs(mPath);
}

// 白名单处理模式
static void whiteMode(const std::filesystem::path& quModPath)
{
	std::cout << "请输入需要保留的白名单模块(空格分隔): ";
	auto userArgs = getUserInputArgs();
	std::cout << "\n";
	std::unordered_set<std::string> targets(userArgs.begin(), userArgs.end());
	whiteMode(quModPath, targets);
}

// 黑名单处理模式
static void blackMode(const std::filesystem::path& quModPath)
{
	auto analyzer = QuModMEX::QuModLibsAnalyzer(quModPath);
	auto mRv = analyzer.getAllModulesRelationViews();
	auto manager = QuModMEX::ModuleRelationViewManager{ mRv };
	std::unordered_set<std::string> targets;
	std::cout << "请输入需要剔除的黑名单模块(空格分隔): ";
	auto userArgs = getUserInputArgs();
	targets = std::unordered_set<std::string>(userArgs.begin(), userArgs.end());
	std::cout << "移除目标: " << listToString(targets) << "\n";
	// 反向白名单生成
	std::unordered_set<std::string> targets2;
	for (const auto& [name, _] : manager.getViewDatas()) {
		if(targets.find(name) == targets.end()) {
			targets2.insert(name);
		}
	}
	auto canRemove = manager.getRemoveUnwantedModules(targets2);
	if (canRemove.empty()) {
		std::cout << "\n根据分析，没有相关可安全移除的模块。\n";
		for (auto& view : mRv) {
			if (targets.find(view.getName()) != targets.end()) {
				std::cout << view.getName() << " 被" << view.getRefedCount() << "个模块引用。\n";
				std::cout << "以下模块依赖: " << listToString(view.getRefedModules()) << "\n";
				std::cout << "\n";
			}
		}
		return;
	}
	std::cout << "根据分析，可安全移除（按下回车执行）: " << listToString(canRemove) << "\n";
	system("pause");
	auto mPath = quModPath / "Modules";
	for (const auto& m : canRemove) {
		auto path = mPath / m;
		if (std::filesystem::exists(path)) {
			std::cout << "删除模块: " << path.generic_string() << "\n";
			std::filesystem::remove_all(path);
		}
		else {
			std::cout << "模块不存在: " << path.generic_string() << "\n";
		}
	}
	removeNullPythonDirs(mPath);
}

// 仅计算依赖关系
static void onlyListMode(const std::filesystem::path& quModPath)
{
	auto analyzer = QuModMEX::QuModLibsAnalyzer(quModPath);
	auto relationViews = analyzer.getAllModulesRelationViews();
	for (auto& view : relationViews) {
		std::cout << view.getName() << " 共引用" << view.getRefCount() << "个模块，被" << view.getRefedCount() << "个模块引用。\n";
		std::cout << "引用: " << listToString(view.getRefModules()) << "\n";
		std::cout << "被引用: " << listToString(view.getRefedModules()) << "\n";
		std::cout << "\n";
	}
}

static void _testExtractImportedModules(const std::filesystem::path& path, std::unordered_set<std::string>& modules)
{
	// 递归遍历文件/文件夹并排除QuModLibs目录
	for(const auto& entry : std::filesystem::directory_iterator(path))
	{
		if(entry.is_directory()) {
			// 排除 QuModLibs 目录
			if(entry.path().filename() == "QuModLibs") {
				continue;
			}
			_testExtractImportedModules(entry.path(), modules);
		}
		else if(entry.is_regular_file() && entry.path().extension() == ".py") {
			std::string code;
			std::ifstream file(entry.path(), std::ios::binary);
			if (file) {
				std::ostringstream ss;
				ss << file.rdbuf();
				code = ss.str();
			}
			else {
				std::cout << "无法打开文件: " << entry.path().generic_string() << "\n";
				continue;
			}
			auto imported = QuModMEX::extractImportedModules(code);
			for(const auto& m : imported) {
				modules.insert(m);
			}
		}
	}
}

// 计算MOD中QuModLibs模块引用列表
static std::vector<std::string> testExtractImportedModules(const std::filesystem::path& quModPath)
{
	std::unordered_set<std::string> allModules;
	std::unordered_set<std::string> modules;
	// 从 QuModLibs上级目录开始搜索
	_testExtractImportedModules(quModPath.parent_path(), allModules);
	if(std::filesystem::exists(quModPath / "Include")) {
		modules.insert("Services");	// Include扩展必须包含Services模块
	}
	// 解析 QuModLibs.Modules.{}.* 字段
	static std::regex moduleRegex(R"((?:^|\.)QuModLibs\.Modules\.([A-Za-z0-9_]+)(?:\.|$))");
	for (const auto& quModule : allModules) {
		std::smatch match;
		if (std::regex_search(quModule, match, moduleRegex)) {
			if (match.size() > 1) {
				modules.insert(match[1].str());
			}
		}
	}
	return std::vector<std::string>(modules.begin(), modules.end());
}

// 用户列出当前项目引用列表
static void testListCurrentProjectRefs(const std::filesystem::path& quModPath)
{
	auto refs = testExtractImportedModules(quModPath);
	std::cout << "项目路径: " << quModPath.parent_path().generic_string() << "\n";
	std::cout << "引用模块: " << listToString(refs) << "\n";
}

// 用户自动分析移除无关项目的模块
// 不支持反射/跨包等不可见引用
static void testAutoRemoveUnrelatedModules(const std::filesystem::path& quModPath)
{
	auto refs = testExtractImportedModules(quModPath);
	std::unordered_set<std::string> targets(refs.begin(), refs.end());
	whiteMode(quModPath, targets);
}

int main()
{
	Encoding::initEnvcode();
	std::cout << R"(------------------------------------------------------
QuModMEX使用说明：
- 该项目用于分析依赖并剔除无用QuModLibs模块
- 提供两种剔除模式:
  0. 白名单模式，保留特定模块及其依赖
  1. 黑名单模式，移除特定模块（依赖检查）
  2. 仅列出依赖关系，不作其他处理
  3. 自动分析移除无关项目的模块（不支持反射/跨包等不可见引用）
  4. 仅列出当前项目的引用模块（限制条件与3相同）
- 若需要移除补全库，另见：QuModPurge.exe
- 特殊: Include扩展无互相依赖关系，若无需求可直接删除
------------------------------------------------------
)";
	std::filesystem::path targetPath;
	std::cout << "\n请输入目标路径(QuModLibs): ";
	targetPath = readLine();
	std::cout << "目标路径：" << targetPath << "\n\n";
	if (!std::filesystem::exists(targetPath / "QuMod.py"))
	{
		std::cout << "目标路径不存在QuMod.py文件，请检查路径是否正确。\n";
		system("pause");
		return 0;
	}
	std::cout << "请输入处理策略(0/1/2/3/4): ";
	std::vector<std::string> modeArgs = getUserInputArgs();
	std::cout << "\n";
	if (modeArgs.size() != 1) {
		std::cout << "输入参数错误，请检查输入格式。\n";
		system("pause");
		return 0;
	}
	if (modeArgs[0] == "0") {
		whiteMode(targetPath);
	}
	else if (modeArgs[0] == "1") {
		blackMode(targetPath);
	}
	else if (modeArgs[0] == "2") {
		onlyListMode(targetPath);
	}
	else if(modeArgs[0] == "3") {
		testAutoRemoveUnrelatedModules(targetPath);
	}
	else if(modeArgs[0] == "4") {
		testListCurrentProjectRefs(targetPath);
	}
	else {
		std::cout << "不支持的策略模式：" << modeArgs[0] << "\n";
	}
	system("pause");
	return 0;
}