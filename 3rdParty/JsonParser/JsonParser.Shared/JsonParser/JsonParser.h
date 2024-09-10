#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WINDOWS_LEAN_AND_MEAN
#define WINDOWS_LEAN_AND_MEAN
#endif
#include <Windows.h>

#define JS_STL_MAP 1
#define JS_STL_SET 1
#include <JsonParser/json_struct/json_struct.h> // https://github.com/jorgen/json_struct
#include "Preprocessor.h"

namespace JS {
    template <typename T>
    struct JsonObjectWrapper {
        JsonObjectWrapper(T& valueRef)
            : valueRef{ valueRef }
        {}

        T& valueRef;
    };

    template <typename T>
    struct TypeHandler<JsonObjectWrapper<T>>
    {
        static inline Error to(JsonObjectWrapper<T>& to_type, ParseContext& context)
        {
            if (context.token.value_type != JS::Type::ObjectStart)
                return Error::ExpectedObjectStart;

            Error error = context.nextToken();
            if (error != JS::Error::NoError)
                return error;

            while (context.token.value_type != JS::Type::ObjectEnd)
            {
                error = TypeHandler<T>::to(to_type.valueRef, context);
                if (error != JS::Error::NoError)
                    break;

                error = context.nextToken();
                if (error != JS::Error::NoError)
                    break;
            }

            return error;
        }

        static inline void from(const JsonObjectWrapper<T>& from_type, Token& token, Serializer& serializer)
        {
            token.value_type = Type::ObjectStart;
            token.value = DataRef("{");
            serializer.write(token);

            token.name = DataRef("");
            TypeHandler<T>::from(from_type.valueRef, token, serializer);
            token.name = DataRef("");

            token.value_type = Type::ObjectEnd;
            token.value = DataRef("}");
            serializer.write(token);
        }
    };


    namespace details {
        inline std::string WStrToStr(const std::wstring& wstr, int codePage = CP_ACP) {
            if (wstr.size() == 0)
                return std::string{};

            int sz = WideCharToMultiByte(codePage, 0, wstr.c_str(), -1, 0, 0, 0, 0);
            std::string res(sz, 0);
            WideCharToMultiByte(codePage, 0, wstr.c_str(), -1, &res[0], sz, 0, 0);
            return res.c_str(); // To delete '\0' use .c_str()
        }

        inline std::wstring StrToWStr(const std::string& str, int codePage = CP_ACP) {
            if (str.size() == 0)
                return std::wstring{};

            int sz = MultiByteToWideChar(codePage, 0, str.c_str(), -1, 0, 0);
            std::wstring res(sz, 0);
            MultiByteToWideChar(codePage, 0, str.c_str(), -1, &res[0], sz);
            return res.c_str(); // To delete '\0' use .c_str()
        }
    }

    template <>
    struct TypeHandler<std::wstring>
    {
        static inline Error to(std::wstring& to_type, ParseContext& context) {
            std::string str;
            auto error = TypeHandler<std::string>::to(str, context);
            to_type = details::StrToWStr(str);
            return error;
        }

        static inline void from(const std::wstring& wstr, Token& token, Serializer& serializer) {
            TypeHandler<std::string>::from(details::WStrToStr(wstr), token, serializer);
        }
    };



    class LoggerCallback {
    private:
        LoggerCallback() = default;
        static LoggerCallback& GetInstance();
    public:
        ~LoggerCallback() = default;
        static bool Register(std::function<void(std::string)> callback);

    private:
        template <typename T>
        friend bool ParseTo(const char*, size_t, T&);
        
        template <typename T>
        friend bool ParseTo(const std::vector<char>&, T&);

        static void SafeInvoke(const std::string& msg);

    private:
        std::function<void(std::string)> loggerCallback;
    };



    template <typename T>
    bool ParseTo(const char* jsonRawData, size_t jsonRawDataSize, T& structure) {
        ParseContext parseContext(jsonRawData, jsonRawDataSize);

        if (parseContext.parseTo(structure) != JS::Error::NoError) {
            LoggerCallback::SafeInvoke(parseContext.makeErrorString());
            assert(false && "--> Error parsing json");
            return false;
        }
        return true;
    }

    template <typename T>
    bool ParseTo(const std::vector<char>& jsonRawData, T& structure) {
        return ParseTo<T>(jsonRawData.data(), jsonRawData.size(), structure);
    }
}

// NOTE: When use enum in structs that need parse don't forget set default value
//       to avoid pareser error = "InvalidData".

#define __JSONPARSER_ENUM_DECLARE_STRING_PARSER(namespaceName, enumName, ...) \
namespace namespaceName { \
	struct js_##enumName##_string_struct \
	{ \
	  template <size_t N> \
	  explicit js_##enumName##_string_struct(const char(&data)[N]) \
	  { \
		JS::Internal::populateEnumNames(_strings, data); \
	  } \
	  std::vector<JS::DataRef> _strings; \
	  \
	  static const std::vector<JS::DataRef>& strings() \
	  { \
		static js_##enumName##_string_struct ret(#__VA_ARGS__); \
		return ret._strings; \
	  } \
	}; \
} \
namespace JS \
{ \
	template <> \
	struct TypeHandler<namespaceName::enumName> \
	{ \
		static inline Error to(namespaceName::enumName& to_type, ParseContext& context) \
		{ \
			return Internal::EnumHandler<namespaceName::enumName, namespaceName::js_##enumName##_string_struct>::to(to_type, context); \
		} \
		static inline void from(const namespaceName::enumName& from_type, Token& token, Serializer& serializer) \
		{ \
			return Internal::EnumHandler<namespaceName::enumName, namespaceName::js_##enumName##_string_struct>::from(from_type, token, serializer); \
		} \
	}; \
}

#define JSONPARSER_ENUM_DECLARE_STRING_PARSER(...) PP_EXPAND(__JSONPARSER_ENUM_DECLARE_STRING_PARSER(__VA_ARGS__))