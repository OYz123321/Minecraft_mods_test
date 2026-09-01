#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#ifdef _WIN32
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <direct.h>
    #include <windows.h>
#endif

// type: data / resources
// uuid: uuid4标准
// name: 模块名称
// description: 模块描述
static const std::string MANIFEST_TEMPLATE = R"manifest({
    "format_version": 2,
    "header": {
        "description": "<description>",
        "name": "<name>",
        "uuid": "<uuid1>",
        "version": [0, 0, 1],
        "min_engine_version": [1, 20, 0]
    },
    "modules": [
        {
            "description": "<description>",
            "type": "<type>",
            "uuid": "<uuid2>",
            "version": [0, 0, 1]
        }
    ]
})manifest";

struct ManifestArtifact {
    std::string folderName;
    std::string headerUuid;
    std::string moduleUuid;
    std::string content;
};

static std::string generateUuidV4() {
    static std::uint64_t state = [] {
        FILETIME fileTime{};
        GetSystemTimeAsFileTime(&fileTime);

        LARGE_INTEGER counter{};
        QueryPerformanceCounter(&counter);

        return
            (static_cast<std::uint64_t>(fileTime.dwHighDateTime) << 32) ^
            static_cast<std::uint64_t>(fileTime.dwLowDateTime) ^
            static_cast<std::uint64_t>(counter.QuadPart) ^
            static_cast<std::uint64_t>(GetCurrentProcessId()) ^
            (static_cast<std::uint64_t>(GetCurrentThreadId()) << 16);
    }();

    const auto nextRandom64 = []() -> std::uint64_t {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 2685821657736338717ull;
    };

    std::uint8_t bytes[16]{};

    for (std::size_t offset = 0; offset < sizeof(bytes); offset += sizeof(std::uint64_t)) {
        const std::uint64_t randomValue = nextRandom64();

        for (std::size_t i = 0; i < sizeof(std::uint64_t) && (offset + i) < sizeof(bytes); ++i) {
            bytes[offset + i] = static_cast<std::uint8_t>((randomValue >> (i * 8)) & 0xFFu);
        }
    }

    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0F) | 0x40);
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3F) | 0x80);

    constexpr char HEX_DIGITS[] = "0123456789abcdef";

    std::string uuid;
    uuid.reserve(36);

    for (std::size_t i = 0; i < sizeof(bytes); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            uuid.push_back('-');
        }

        uuid.push_back(HEX_DIGITS[bytes[i] >> 4]);
        uuid.push_back(HEX_DIGITS[bytes[i] & 0x0F]);
    }

    return uuid;
}

// 高效的manifest生成函数，避免引入完整的JSON DOM解析
static std::string generateManifest(
    std::string_view type,
    std::string_view name,
    std::string_view uuid1,
    std::string_view uuid2,
    std::string_view description) {
    std::string manifest;
    manifest.reserve(
        MANIFEST_TEMPLATE.size() +
        type.size() +
        name.size() +
        uuid1.size() +
        uuid2.size() +
        description.size() * 2);

    std::string_view templateView = MANIFEST_TEMPLATE;
    std::size_t cursor = 0;

    while (cursor < templateView.size()) {
        const std::size_t tokenBegin = templateView.find('<', cursor);
        if (tokenBegin == std::string_view::npos) {
            manifest.append(templateView.substr(cursor));
            break;
        }

        manifest.append(templateView.substr(cursor, tokenBegin - cursor));

        const std::size_t tokenEnd = templateView.find('>', tokenBegin + 1);
        if (tokenEnd == std::string_view::npos) {
            manifest.append(templateView.substr(tokenBegin));
            break;
        }

        const std::string_view token = templateView.substr(tokenBegin, tokenEnd - tokenBegin + 1);

        if (token == "<type>") {
            manifest.append(type);
        } else if (token == "<uuid1>") {
            manifest.append(uuid1);
        } else if (token == "<uuid2>") {
            manifest.append(uuid2);
        } else if (token == "<name>") {
            manifest.append(name);
        } else if (token == "<description>") {
            manifest.append(description);
        } else {
            manifest.append(token);
        }

        cursor = tokenEnd + 1;
    }

    return manifest;
}

static std::string generateManifest(
    std::string_view type,
    std::string_view name,
    std::string_view description) {
    const std::string uuid1 = generateUuidV4();
    const std::string uuid2 = generateUuidV4();
    return generateManifest(type, name, uuid1, uuid2, description);
}

static std::string makeAddonFolderName(std::string_view uuid, char suffix) {
    std::string folderName;
    folderName.reserve(uuid.size());

    for (const char ch : uuid) {
        if (ch == '-') {
            continue;
        }

        folderName.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }

    if (folderName.empty()) {
        return "A";
    }

    folderName.front() = 'A';
    folderName.back() = static_cast<char>(std::toupper(static_cast<unsigned char>(suffix)));
    return folderName;
}

static ManifestArtifact generateManifestArtifact(
    std::string_view type,
    std::string_view description,
    char folderSuffix) {
    ManifestArtifact artifact;
    artifact.headerUuid = generateUuidV4();
    artifact.moduleUuid = generateUuidV4();
    artifact.folderName = makeAddonFolderName(artifact.headerUuid, folderSuffix);
    artifact.content = generateManifest(
        type,
        artifact.folderName,
        artifact.headerUuid,
        artifact.moduleUuid,
        description);
    return artifact;
}

static std::string joinPath(std::string_view left, std::string_view right) {
    std::string path;
    path.reserve(left.size() + right.size() + 1);
    path.append(left);

    if (!path.empty() && path.back() != '\\' && path.back() != '/') {
        path.push_back('\\');
    }

    path.append(right);
    return path;
}

static std::string toDisplayPath(std::string path) {
    for (char& ch : path) {
        if (ch == '\\') {
            ch = '/';
        }
    }

    return path;
}

static bool tryGetTargetDirectoryFromArgs(int argc, char** argv, std::string& currentDir, std::string& errorMessage) {
    if (argc < 2) {
        return false;
    }
    const std::string_view argPath(argv[1]);
    // 跳过引号
    if (argPath.size() >= 2 && argPath.front() == '"' && argPath.back() == '"') {
        currentDir = argPath.substr(1, argPath.size() - 2);
    } else {
        currentDir = argPath;
    }
    if (currentDir.empty()) {
        errorMessage = "提供的路径参数为空";
        return false;
    }
    return true;
}

static bool getCurrentDirectoryPath(std::string& currentDir, std::string& errorMessage) {
    char* rawPath = _getcwd(nullptr, 0);
    if (rawPath == nullptr) {
        errorMessage = "读取当前目录失败";
        return false;
    }

    currentDir = rawPath;
    std::free(rawPath);
    return true;
}

static bool createDirectoryIfNeeded(const std::string& path, std::string& errorMessage) {
    if (_mkdir(path.c_str()) == 0 || errno == EEXIST) {
        return true;
    }

    errorMessage = "创建目录失败: " + toDisplayPath(path);
    return false;
}

static bool writeTextFile(
    const std::string& filePath,
    std::string_view content,
    std::string& errorMessage) {
    std::FILE* output = nullptr;
#ifdef _WIN32
    if (fopen_s(&output, filePath.c_str(), "wb") != 0 || output == nullptr) {
#else
    output = std::fopen(filePath.c_str(), "wb");
    if (output == nullptr) {
#endif
        errorMessage = "无法打开输出文件: " + toDisplayPath(filePath);
        return false;
    }

    const std::size_t writtenSize = std::fwrite(content.data(), 1, content.size(), output);
    const int closeResult = std::fclose(output);
    if (writtenSize != content.size() || closeResult != 0) {
        errorMessage = "写入文件失败: " + toDisplayPath(filePath);
        return false;
    }

    return true;
}

static bool writePack(
    const std::string& targetDir,
    std::string_view contentDir,
    const ManifestArtifact& artifact,
    std::string& manifestPath,
    std::string& errorMessage) {
    const std::string packDir = joinPath(targetDir, artifact.folderName);
    const std::string contentPath = joinPath(packDir, contentDir);

    if (!createDirectoryIfNeeded(packDir, errorMessage) ||
        !createDirectoryIfNeeded(contentPath, errorMessage)) {
        return false;
    }

    manifestPath = joinPath(packDir, "manifest.json");
    return writeTextFile(manifestPath, artifact.content, errorMessage);
}

static void logGeneratedPack(
    std::string_view label,
    std::string_view manifestPath,
    const ManifestArtifact& artifact) {
    const std::string manifestPathText = toDisplayPath(std::string(manifestPath));

    std::printf(
        "[%.*s]\n目录名: %s\nHeader UUID: %s\nModule UUID: %s\nManifest: %s\n%s\n\n",
        static_cast<int>(label.size()),
        label.data(),
        artifact.folderName.c_str(),
        artifact.headerUuid.c_str(),
        artifact.moduleUuid.c_str(),
        manifestPathText.c_str(),
        artifact.content.c_str());
}

int main(int argc, char** argv) {
#ifdef _WIN32
    // Set the console output code page to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    int exitCode = 0;

    std::string targetDir;
    std::string errorMessage;
    if (!tryGetTargetDirectoryFromArgs(argc, argv, targetDir, errorMessage) &&
        !getCurrentDirectoryPath(targetDir, errorMessage)
        ) 
    {
        std::fprintf(stderr, "%s\n", errorMessage.c_str());
        exitCode = 1;
    } else {
        const auto behaviorPack = generateManifestArtifact(
            "data",
            "Auto generated behavior pack",
            'B');
        const auto resourcePack = generateManifestArtifact(
            "resources",
            "Auto generated resource pack",
            'R');

        std::string behaviorManifestPath;
        std::string resourceManifestPath;

        if (!writePack(targetDir, "entities", behaviorPack, behaviorManifestPath, errorMessage) ||
            !writePack(targetDir, "textures", resourcePack, resourceManifestPath, errorMessage)) {
            std::fprintf(stderr, "生成Addon失败: %s\n", errorMessage.c_str());
            exitCode = 1;
        } else {
            const std::string targetDirText = toDisplayPath(targetDir);
            std::printf("生成根目录: %s\n\n", targetDirText.c_str());
            logGeneratedPack("Behavior Pack", behaviorManifestPath, behaviorPack);
            logGeneratedPack("Resource Pack", resourceManifestPath, resourcePack);
        }
    }

#ifdef _WIN32
    std::puts("Press Enter to continue...");
    std::getchar();
#endif
    return exitCode;
}
