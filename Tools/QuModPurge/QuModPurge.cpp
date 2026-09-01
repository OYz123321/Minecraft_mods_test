#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include "EnvEncode.h"

class QuModPackage
{
private:
	std::filesystem::path dirPath;
public:
	QuModPackage(const std::filesystem::path& path): dirPath(path) {}

	QuModPackage(std::filesystem::path&& path) noexcept: dirPath(std::move(path)) {}

	// 安全的删除目标文件夹
	static void safeRemoveDir(const std::filesystem::path& path)
	{
		if (std::filesystem::exists(path))
		{
			std::filesystem::remove_all(path);
			std::cout << "删除文件夹: " << path.filename() << "\n";
		}
	}

	// 清除无效代码块(if 1>2)
	static std::string removeDeadConditionBlocks(const std::string& code)
	{
		// if 1 > 2的正则表达式
		const std::regex pattern("((?:^|\\r?\\n)[ \\t]*if\\s+1\\s*>\\s*2\\s*:\\s*(?:\\r?\\n[ \\t]+.*)*)");
		return std::regex_replace(
			code,
			pattern,
			""
		);
	}

	// 删除空类型声明(# type: xxx)
	static std::string removeNullTypeDefine(const std::string& code)
	{
		std::istringstream stream(code);
		std::string line;
		bool foundFuncOrCls = false;
		std::string cleanedContent;

		// 正则匹配 # type: xxx 注释
		const std::regex typeCommentPattern(R"(#\s*type\s*:\s*.*)", std::regex::ECMAScript | std::regex::optimize);

		while (std::getline(stream, line))
		{
			std::string trimmed = line;
			while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
			{
				trimmed.erase(trimmed.begin());
			}

			// 检测是否是函数或类定义
			if (trimmed.starts_with("def ") || trimmed.starts_with("class "))
			{
				foundFuncOrCls = true;
			}

			// 只有在 def/class 之前才会删除 # type: xxx
			if (!foundFuncOrCls)
			{
				line = std::regex_replace(line, typeCommentPattern, "");
			}
			cleanedContent.append(line + "\n");
		}
		return cleanedContent;
	}

	// 读取文本文件
	static void readTxtFile(const std::filesystem::path& path, std::string& out)
	{
		std::ifstream file(path, std::ios::binary);  // 以二进制模式打开
		if (!file)
		{
			throw std::runtime_error("无法打开目标文件: " + path.filename().string());
		}
		// 获取文件大小
		file.seekg(0, std::ios::end);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		// 预分配内存提高效率
		out.resize(size);
		// 读取整个文件内容
		if (size > 0)
		{
			file.read(&out[0], size);
		}
	}

	// 删除所有辅助定义
	static std::string removeAllNullDefine(const std::string& code)
	{
		std::string _NO_DCB = removeDeadConditionBlocks(code);
		return removeNullTypeDefine(_NO_DCB);
	}

	// 删除目标文件的所有空定义
	static void removeFileAllNullDefine(const std::filesystem::path& path)
	{
		std::string fileTxt;
		readTxtFile(path, fileTxt);
		std::string newTxt = removeAllNullDefine(fileTxt);
		std::ofstream outFile(path, std::ios::binary);
		if (!outFile)
		{
			throw std::runtime_error("无法写入目标文件: " + path.filename().string());
		}
		outFile.write(newTxt.data(), newTxt.size());
		std::cout << "删除补全依赖: " << path.filename() << "\n";
	}

	// 文段替换
	static void replaceAll(std::string& content, const std::string& from, const std::string& to)
	{
		if (from.empty()) return;
		size_t pos = 0;
		while ((pos = content.find(from, pos)) != std::string::npos)
		{
			content.replace(pos, from.length(), to);
			pos += to.length();
		}
	}

	// 删除UI文件的所有空定义
	static void removeUIFileNullDefine(const std::filesystem::path& path)
	{
		std::string fileTxt;
		readTxtFile(path, fileTxt);
		// 高版本ScreenNodeWrapper继承处理
		std::regex wrapperUIRe(R"(class\s+ScreenNodeWrapper\s*\(([^)]*?),\s*BaseScreenNode\s*(.*?)\):)");
		fileTxt = std::regex_replace(fileTxt, wrapperUIRe, R"(class ScreenNodeWrapper($1$2):)");

		// BaseScreenNode处理
		std::regex pattern(
			R"(\bfrom\s+\.?QuClientApi\.ui\.screenNode\s+import\s+ScreenNode\s+as\s+BaseScreenNode\b)"
		);
		std::string content = std::regex_replace(fileTxt, pattern, "", std::regex_constants::format_first_only);
		replaceAll(content, "BaseScreenNode", "ScreenNode");	// 补全依赖替换
		std::ofstream outFile(path, std::ios::binary);
		if (!outFile)
		{
			throw std::runtime_error("无法写入目标文件: " + path.filename().string());
		}
		outFile.write(content.data(), content.size());
		std::cout << "删除UI补全依赖: " << path.filename() << "\n";
	}

	// 项目清理
	void clearPackage() const
	{
		// 补全库源文件删除
		safeRemoveDir(dirPath / "QuServerApi");
		safeRemoveDir(dirPath / "QuClientApi");
		// 删除依赖
		removeFileAllNullDefine(dirPath / "Server.py");
		removeFileAllNullDefine(dirPath / "Client.py");
		removeUIFileNullDefine(dirPath / "UI.py");
		std::cout << "清理工作已完成\n";
	}
};

int main()
{
	// 统一编码环境为UTF8
	Encoding::initEnvcode();

	std::string libsPath;
	std::cout << "该工具用于清理完整的QuModLibs中内置的补全库 以便降低包体体积(By Zero123)\n";
	std::cout << "请输入QuModLibs完整路径: ";
	std::cin >> libsPath;
	std::cout << "\n";
	std::filesystem::path dirPath{ libsPath };

	if (!std::filesystem::exists(dirPath))
	{
		std::cout << "无效的目录 请检查\n";
	}
	else
	{
		try
		{
			if (std::filesystem::exists(dirPath / "modMain.py"))
			{
				throw std::runtime_error("请提供QuModLibs文件夹路径 而非MOD脚本路径");
			}
			auto package = QuModPackage(std::move(dirPath));
			package.clearPackage();
		}
		catch (const std::exception& e)
		{
			std::cerr << "\033[31;1m" << e.what() << "\033[0m\n";
		}
	}
	system("pause");
	return 0;
}
