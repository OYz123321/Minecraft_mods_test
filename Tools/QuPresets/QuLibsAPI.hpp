#pragma once
#include <iostream>
#include <vector>
#include <cstdint>	// int32_t, uint32_t 的定义

#ifdef QU_PRODUCER_MODE
	#define CUSTOM_API extern "C"
#else
	#ifdef _MSC_VER
		#define CUSTOM_API extern "C" __declspec(dllexport)
	#elif defined(__GNUC__)
		#define CUSTOM_API extern "C" __attribute__((visibility("default")))
	#else
		#define CUSTOM_API extern "C"
	#endif
#endif

// By Zero123 TIME: 2024/10/19
namespace QuAPI
{
	// C部分共有协定
	extern "C"
	{
		// 复合类型
		#pragma pack(8)
		struct _QuCompositeType
		{
			int32_t type;
			void* valuePtr;
			/* end -2 null -1 char 0 bool 1 int32 2 float 3 cString 4 ptr 5 */
		};

		// 复合Args参数结构
		#pragma pack(8)
		struct _CArgs
		{
			int32_t size;
			_QuCompositeType* headPtr;
		};

		typedef void* (*LIB_GET_ENV_TYPE)(const char*);						// Lib获取环境信息函数类型 返回对应数据指针
		typedef void (*UPDATE_ENV_DATA_TYPE)(void*, void*, LIB_GET_ENV_TYPE);	// Lib更新环境数据函数类型
		typedef void (*API_FUNCTION_TYPE)(_CArgs*);							// API函数类型
		typedef const char* (*GET_PATH_TYPE)(void*);							// 字符串目录函数类型
		typedef uint32_t* API_VERSION_TYPE;												// API版本类型
		typedef char* (*QPRESET_GET_VARIABLE_TYPE)(void*, const char*);
	}

	// env基本数据 在UPDATE_ENV_DATA触发时更新
	void* PRESET_PTR = nullptr;
	void* ADDON_PTR = nullptr;
	QuAPI::LIB_GET_ENV_TYPE GET_ENV_PTR = nullptr;

	// QuCompositeType C++封装类型
	class AnyObject
	{
	private:
		int32_t type = 0;
		std::string str{};
		float f = 0;
		int32_t i32 = 0;
		char c = 0;
		bool b = 0;
		void* p = nullptr;
	public:
		explicit AnyObject(_QuCompositeType* quCompositeData)
		{
			type = quCompositeData->type;	// 类型字段
			auto vPtr = quCompositeData->valuePtr;
			p = vPtr;
			if (isString())
			{
				str = std::string((char*)vPtr);
			}
			else if (isInt32())
			{
				i32 = *(int32_t*)vPtr;
			}
			else if (isFloat())
			{
				f = *(float*)vPtr;
			}
			else if (isChar())
			{
				c = *(char*)vPtr;
			}
			else if (isBool())
			{
				b = *(bool*)vPtr;
			}
		}

		// 自动类型转换
		operator int() const { return (int) getNumber(); }
		operator float() const { return getNumber(); }
		operator bool() const { return getBool(); }
		operator std::string() { return getString(); }

		friend std::ostream& operator<<(std::ostream& os, const AnyObject& _self)
		{
			if (_self.isInt32())
			{
				os << _self.getInt(0);
			}
			else if (_self.isFloat())
			{
				os << _self.getFloat();
			}
			else if (_self.isChar())
			{
				os << _self.getChar();
			}
			else if (_self.isNull())
			{
				os << "null";
			}
			else if (_self.isBool())
			{
				os << _self.getBool();
			}
			else if (_self.isString())
			{
				os << std::string(_self.getString());
			}
			else
			{
				os << "unknow";
			}
			return os;
		};

		// 获取字符串 如果是
		std::string getString(const std::string& errorValue = "") const
		{
			if (isString())
			{
				return str;
			}
			return errorValue;
		}

		// 获取Int 如果是
		int getInt(int errorValue = -1) const
		{
			if (isInt32())	// i32确保内存大小一致性(在不同编译器中) 并重新转换到本地编译器的int长度
			{
				return (int)i32;
			}
			return errorValue;
		}

		// 获取Float 如果是
		float getFloat(float errorValue = -1.f) const
		{
			if (isFloat())
			{
				return f;
			}
			return errorValue;
		}

		// 获取Char 如果是
		char getChar(char errorValue = 0) const
		{
			if (isChar())
			{
				return c;
			}
			return errorValue;
		}

		// 获取Bool 如果是
		bool getBool(bool errorValue = false) const
		{
			if (isBool())
			{
				return b;
			}
			return errorValue;
		}

		// 获取数字 当类型为int/float时都将按float返回
		float getNumber(float errorValue = 0.f) const
		{
			if (isInt32())
			{
				return (float) getInt();
			}
			else if (isFloat())
			{
				return getFloat();
			}
			return errorValue;
		}

		// 类型检查  
		bool isNull() const { return type == -1; }
		bool isEnd() const { return type == -2; }
		bool isChar() const { return type == 0; }
		bool isBool() const { return type == 1; }
		bool isInt32() const { return type == 2; }
		bool isFloat() const { return type == 3; }
		bool isNumber() const { return isInt32() || isFloat(); }
		bool isString() const { return type == 4; }
		bool isPtr() const { return type == 5; }
	};

	// 复合不定长元组 C++封装类型
	class Args
	{
	public:
		std::vector<AnyObject> _data;
		Args(_CArgs* cArgsPtr)
		{
			for (int32_t i = 0; i < cArgsPtr->size; i++)
			{
				_data.push_back(AnyObject(&cArgsPtr->headPtr[i]));
			}
		}

		// 获取args数量
		unsigned int size() const
		{
			return (int) _data.size();
		}

		AnyObject getObject(int i)
		{
			return _data[i];
		}

		friend std::ostream& operator<<(std::ostream& os, const Args& _self)
		{
			os << "(";
			for (const auto& it : _self._data)
			{
				os << it << ", ";
			}
			os << ")";
			return os;
		};
	};

	// 释放 QuCompositeType 中的堆上指针资源
	void freeQuCompositeType(_QuCompositeType* ptr)
	{
		if (ptr == nullptr || ptr->valuePtr == nullptr)
		{
			return;
		}
		free(ptr->valuePtr);
	}

	// 创建一个null复合类型
	_QuCompositeType createNullCompositeType()
	{
		return _QuCompositeType{-1, nullptr};
	}

	// 创建一个end复合类型 (这通常描述数组结尾)
	_QuCompositeType createEndCompositeType()
	{
		return _QuCompositeType{-2, nullptr};
	}

	// 判断结构数据是否为end声明
	bool isEndCompositeType(_QuCompositeType* ptr)
	{
		return ptr->type == -2;
	}

	// 判断结构数据是否为null值
	bool isNullCompositeType(_QuCompositeType* ptr)
	{
		return ptr->type == -1;
	}

	// 获取环境信息
	void* getEnv(const std::string& keyName)
	{
		return QuAPI::GET_ENV_PTR(keyName.c_str());
	}

	// 获取行为包目录 (如果不存在返回空字符串)
	std::string getBehPath()
	{
		auto _fun = (GET_PATH_TYPE) getEnv("getBehPath");
		return std::string(_fun(ADDON_PTR));
	}

	// 获取资源包目录 (如果不存在返回空字符串)
	std::string getResPath()
	{
		auto _fun = (GET_PATH_TYPE) getEnv("getResPath");
		return std::string(_fun(ADDON_PTR));
	}

	// 获取API版本信息
	uint32_t getApiVersion()
	{
		auto u32 = (API_VERSION_TYPE) getEnv("__VERSION__");
		return *u32;
	}
}

// 环境信息更新触发(在执行动态库时将会自动执行此函数)
CUSTOM_API void UPDATE_ENV_DATA(void* presetPtr, void* addonPtr, QuAPI::LIB_GET_ENV_TYPE getEnvPtr)
{
	QuAPI::PRESET_PTR = presetPtr;
	QuAPI::ADDON_PTR = addonPtr;
	QuAPI::GET_ENV_PTR = getEnvPtr;
}
