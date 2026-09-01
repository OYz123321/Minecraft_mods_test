#pragma once
#include <iostream>
#include <vector>
#include <regex>
#include <string>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
// By Zero123

namespace QuModMEX
{
    // 模块视图
    class ModuleView
    {
    private:
        std::string path;
    public:
        ModuleView() = default;
        ModuleView(const std::string& path);
        ModuleView(std::string&& path);

        // 转换为Path对象
        std::filesystem::path toPath() const;

        // 自定义==比较运算符
        bool operator==(const ModuleView& other) const;

        // 根据特定路径获得当前所在Module名
        std::string getCurrentModuleName(const std::filesystem::path& targetPath) const;
    };

    // 模块引用关系视图
    class ModuleRelationView
    {
        private:
		    std::string moduleName;
            // 引用其他模块
		    std::unordered_set<std::string> refModules;
            // 被引用表
            std::unordered_set<std::string> refedModules;
		public:
            ModuleRelationView() = default;
            ModuleRelationView(const std::string& moduleName);

            std::string getName() const;

            std::unordered_set<std::string>& getRefModules();
            const std::unordered_set<std::string>& getRefModules() const;

            std::unordered_set<std::string>& getRefedModules();

            // 添加引用模块
            bool addRefModules(const std::string&);
            bool addRefModules(std::string&&);

            // 添加被引用模块
            bool addRefedModules(const std::string&);
            bool addRefedModules(std::string&&);

            // 移除引用
            bool removeRefModules(const std::string&);
            // 移除被引用模块
            bool removeRefedModules(const std::string&);

            // 检查是否存在特定引用
            bool hasRef(const std::string&) const;

            // 查询引用计数
            unsigned int getRefCount() const;
            unsigned int getRefedCount() const;
    };

    // 模块引用关系视图管理器
    class ModuleRelationViewManager
    {
    private:
        std::unordered_map<std::string, ModuleRelationView> viewDatas;
    public:
        ModuleRelationViewManager(const std::vector<ModuleRelationView>& viewVc);
        ModuleRelationViewManager(std::vector<ModuleRelationView>&& viewVc);

        // 检查是否存在特定模块
        bool hasModule(const std::string& moduleName) const;

        // 获取特定模块视图（若不存在则抛出异常）
        ModuleRelationView& getModuleView(const std::string& moduleName);

        // 计算可以安全剔除白名单外的模块视图
        std::vector<std::string> getRemoveUnwantedModules(const std::unordered_set<std::string>& whiteList);

        // 获取所有模块引用关系视图
        std::unordered_map<std::string, ModuleRelationView>& getViewDatas();

        // 在管理器统计中删除特定模块（自动解引用），若启用安全检查，当存在其他模块引用该模块时也将返回false。
        bool removeModule(const std::string& moduleName, bool safeCheck=true);

        // 计算特定模块是否直接/间接引用了其他模块
        bool hasRefModule(const std::string& moduleName, const std::string& refModuleName) const;

        // 获取一个模块的所有引用
        std::unordered_set<std::string> getAllRef(const std::string& moduleName) const;
    };

    // QuModLibs项目分析
    class QuModLibsAnalyzer
    {
    private:
        std::filesystem::path libsPath;
    public:
        QuModLibsAnalyzer(const std::filesystem::path& libsPath);

        // 获取所有模块(QuModLibs/Modules/*)
        std::vector<std::filesystem::path> getAllModules() const;

        // 获取所有模块引用关系视图(QuModLibs/Modules/*)
        std::vector<ModuleRelationView> getAllModulesRelationViews() const;
    };

    // 计算并分析引用Modules，返回字符串Modules路径（字面量）
    std::vector<std::string> extractImportedModules(const std::string& sourceCode);

    // 计算并分析引用Modules，返回ModuleView对象（需要接收一个objectPath，用于处理相对路径的import）
    std::vector<ModuleView> extractImportedModulesView(
        const std::string& sourceCode,
        const std::filesystem::path& objectPath
    );
}