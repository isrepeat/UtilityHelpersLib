#pragma once
#include <vector>
#if defined(min) || defined(max)
#undef min
#undef max
#endif
#include <json_struct/json_struct.h>

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