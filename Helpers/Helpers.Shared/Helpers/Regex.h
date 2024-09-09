#pragma once
#include "common.h"
#include <vector>
#include <string>
#include <regex>

// Docs:
// https://habr.com/ru/companies/otus/articles/532056/

namespace HELPERS_NS {
    namespace Regex {
        struct RegexMatchResult {
            std::vector<std::wstring> capturedGroups;
            std::wstring preffix;
            std::wstring suffix;

            RegexMatchResult(const std::wsmatch& match) {
                for (auto& res : match) {
                    this->capturedGroups.push_back(res);
                }
                this->preffix = match.prefix();
                this->suffix = match.suffix();
            }
        };


        inline std::vector<RegexMatchResult> GetRegexMatches(const std::wstring& text, const std::wregex& rx) {
            std::vector<RegexMatchResult> matches;
            const std::wsregex_token_iterator end_i;
            for (std::wsregex_token_iterator i(text.cbegin(), text.cend(), rx); i != end_i; ++i) {
                std::wstring matchString = *i;
                std::wsmatch matchResult;
                if (std::regex_search(matchString, matchResult, rx)) {
                    matches.push_back(matchResult);
                }
            }
            return matches;
        }

        inline bool FindInsideTagWithRegex(const std::wstring& text, const std::wstring& tag, const std::wregex& innerRx) {
            const std::wregex rx(L"([^<]*)<" + tag + L"[^>]*>(.+?)<[/]" + tag + L">([^<]*)");

            auto matches = GetRegexMatches(text, rx);
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

            auto matches = GetRegexMatches(text, rx);
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