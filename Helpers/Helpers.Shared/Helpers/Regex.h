#pragma once
#include "common.h"
#include <vector>
#include <string>
#include <regex>

// Docs:
// https://habr.com/ru/companies/otus/articles/532056/

namespace HELPERS_NS {
    namespace Regex {
        template<typename T>
        using string_t = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;
        
        template<typename T>
        using regex_t = std::basic_regex<T>;

        template<typename T>
        using regex_match_t = std::match_results<typename string_t<T>::const_iterator>;
        
        template<typename T>
        using regex_token_iterator_t = std::regex_token_iterator<typename string_t<T>::const_iterator>;

        template<typename T>
        struct RegexMatchResult {
            std::vector<string_t<T>> capturedGroups;
            string_t<T> preffix;
            string_t<T> suffix;

            RegexMatchResult(const regex_match_t<T>& match) {
                for (auto& res : match) {
                    this->capturedGroups.push_back(res);
                }
                this->preffix = match.prefix();
                this->suffix = match.suffix();
            }
        };

        template<typename T>
        inline std::vector<RegexMatchResult<T>> GetRegexMatches(const string_t<T>& text, const regex_t<T>& rx) {
            std::vector<RegexMatchResult<T>> matches;

            const regex_token_iterator_t<T> endIt;
            for (regex_token_iterator_t<T> it(text.cbegin(), text.cend(), rx); it != endIt; ++it) {
                string_t<T> matchString = *it;
                regex_match_t<T> matchResult;
                if (std::regex_search(matchString, matchResult, rx)) {
                    matches.push_back(matchResult);
                }
            }
            return matches;
        }

        inline bool FindInsideTagWithRegex(const std::wstring& text, const std::wstring& tag, const std::wregex& innerRx) {
            const std::wregex rx(L"([^<]*)<" + tag + L"[^>]*>(.+?)<[/]" + tag + L">([^<]*)");

            auto matches = GetRegexMatches<wchar_t>(text, rx);
            for (auto& match : matches) {
                std::wsmatch matchResult;
                if (std::regex_search(match.capturedGroups[2], matchResult, innerRx)) {
                    return true;
                }
            }
            return false;
        }

        inline bool FindInsideAnyTagWithRegex(const std::wstring& text, const std::wregex& innerRx) {
            const std::wstring anyTag = L"[^>]*";
            const std::wregex rx(L"([^<]*)<" + anyTag + L">(.+?)<[/]" + anyTag + L">([^<]*)");

            auto matches = GetRegexMatches<wchar_t>(text, rx);
            for (auto& match : matches) {
                std::wsmatch matchResult;
                if (std::regex_search(match.capturedGroups[2], matchResult, innerRx)) {
                    return true;
                }
            }
            return false;
        }
    }
}