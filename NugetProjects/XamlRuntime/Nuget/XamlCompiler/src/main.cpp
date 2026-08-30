#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    struct Element {
        // Промежуточное дерево: оно не зависит от runtime и существует только
        // во время компиляции XAML в C++.
        std::string name;
        std::vector<std::pair<std::string, std::string>> attributes;
        std::vector<Element> children;
        size_t offset = 0;
    };

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Cannot read " + path.string());
        }
        return {
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>(),
        };
    }

    std::string escapeCpp(const std::string& value) {
        std::string result;
        for (const char character : value) {
            if (character == '\\' || character == '"') {
                result += '\\';
            }
            result += character;
        }
        return result;
    }

    size_t lineNumber(const std::string& source, size_t offset) {
        return 1 + static_cast<size_t>(
            std::count(source.begin(), source.begin() + offset, '\n'));
    }

    [[noreturn]] void fail(
        const std::filesystem::path& path,
        const std::string& source,
        size_t offset,
        const std::string& message) {
        throw std::runtime_error(
            path.string() + ":" + std::to_string(lineNumber(source, offset))
            + ": " + message);
    }

    void appendElement(
        Element element,
        std::vector<Element>& stack,
        Element& root,
        bool& hasRoot,
        const std::filesystem::path& path,
        const std::string& source,
        size_t offset) {
        // Верхушка stack — родитель текущего закрытого/self-closing элемента.
        // Пустой stack означает, что element является корнем документа.
        if (!stack.empty()) {
            stack.back().children.push_back(std::move(element));
            return;
        }
        if (hasRoot) {
            fail(path, source, offset, "only one root element is allowed");
        }
        root = std::move(element);
        hasRoot = true;
    }

    Element parse(const std::filesystem::path& path, const std::string& source) {
        // Это намеренно небольшой XAML-диалект, а не универсальный XML-парсер:
        // он принимает только теги/атрибуты, которые способен выразить runtime.
        const std::regex tagPattern(R"(<\s*([^>]+)>)");
        const std::regex attributePattern(
            R"attr(([A-Za-z][A-Za-z0-9]*)\s*=\s*"([^"]*)")attr");
        const std::regex namePattern(R"(^\s*([A-Za-z][A-Za-z0-9]*))");
        std::vector<Element> stack;
        Element root;
        bool hasRoot = false;
        std::sregex_iterator end;

        for (std::sregex_iterator tag(source.begin(), source.end(), tagPattern);
            tag != end;
            ++tag) {
            const auto& match = *tag;
            const size_t offset = static_cast<size_t>(match.position());
            const std::string content = match[1].str();
            if (content.rfind("!--", 0) == 0 || content.rfind("?", 0) == 0) {
                continue;
            }

            const bool closing = !content.empty() && content.front() == '/';
            const bool selfClosing = !closing && !content.empty()
                && content.back() == '/';
            if (closing) {
                // Закрывающий тег завершает элемент и присоединяет его к
                // родителю. Так стек сохраняет вложенность без рекурсии.
                const std::string name = std::regex_replace(
                    content.substr(1), std::regex(R"(\s+)"), "");
                if (stack.empty() || stack.back().name != name) {
                    fail(path, source, offset, "unexpected closing tag </" + name + ">");
                }
                Element element = std::move(stack.back());
                stack.pop_back();
                appendElement(std::move(element), stack, root, hasRoot, path, source, offset);
                continue;
            }

            std::smatch nameMatch;
            std::regex_search(content, nameMatch, namePattern);
            if (nameMatch.empty()) {
                fail(path, source, offset, "element name is required");
            }
            Element element{nameMatch[1].str(), {}, {}, offset};
            for (std::sregex_iterator attribute(
                content.begin(), content.end(), attributePattern);
                attribute != end;
                ++attribute) {
                element.attributes.emplace_back((*attribute)[1].str(), (*attribute)[2].str());
            }
            if (selfClosing) {
                appendElement(std::move(element), stack, root, hasRoot, path, source, offset);
            } else {
                stack.push_back(std::move(element));
            }
        }

        if (!stack.empty()) {
            fail(path, source, stack.back().offset, "element is not closed");
        }
        if (!hasRoot) {
            fail(path, source, 0, "root element is required");
        }
        return root;
    }

    std::string elementType(const Element& element) {
        if (element.name == "StackPanel") {
            return "stackPanel";
        }
        if (element.name == "TextBlock") {
            return "textBlock";
        }
        if (element.name == "Button") {
            return "button";
        }
        throw std::runtime_error("Unsupported XAML element <" + element.name + ">");
    }

    std::string floatLiteral(const std::string& value) {
        return std::to_string(std::stof(value)) + "f";
    }

    void emitProperty(
        const Element& element,
        const std::string& variable,
        const std::string& name,
        const std::string& value,
        std::ostringstream& output) {
        // Атрибуты переводятся в явные вызовы setter'ов. Поэтому итоговый код
        // не разбирает строки в рантайме и остаётся обычным C++.
        if (name == "Id") {
            output << "        " << variable << "->setId(\"" << escapeCpp(value) << "\");\n";
        } else if (name == "Text") {
            output << "        " << variable << "->setText(\"" << escapeCpp(value) << "\");\n";
        } else if (name == "FontSize") {
            output << "        " << variable << "->setFontSize(" << floatLiteral(value) << ");\n";
        } else if (name == "Spacing") {
            output << "        " << variable << "->setSpacing(" << floatLiteral(value) << ");\n";
        } else if (name == "Orientation") {
            const std::string orientation = value == "Horizontal" ? "horizontal"
                : value == "Vertical" ? "vertical" : "";
            if (orientation.empty()) {
                throw std::runtime_error("Orientation must be Horizontal or Vertical");
            }
            output << "        " << variable << "->setOrientation(Orientation::"
                << orientation << ");\n";
        } else if (name == "Foreground") {
            if (value.size() != 7 || value.front() != '#') {
                throw std::runtime_error("Foreground must use #RRGGBB");
            }
            const unsigned long color = std::stoul(value.substr(1), nullptr, 16);
            output << "        " << variable << "->setForeground(Color{"
                << ((color >> 16) & 0xff) << ".0f / 255.0f, "
                << ((color >> 8) & 0xff) << ".0f / 255.0f, "
                << (color & 0xff) << ".0f / 255.0f, 1.0f});\n";
        } else if (name != "HorizontalAlignment" && name != "VerticalAlignment") {
            throw std::runtime_error(
                "Unsupported attribute " + name + " on <" + element.name + ">");
        }
    }

    std::string emitElement(const Element& element, std::ostringstream& output, size_t& index) {
        // Сначала объявляем дочерние unique_ptr, затем передаём их родителю.
        // Это повторяет ownership-структуру исходной XAML-разметки.
        const std::string variable = "element" + std::to_string(index++);
        output << "        auto " << variable << " = std::make_unique<Element>(ElementType::"
            << elementType(element) << ");\n";
        for (const auto& [name, value] : element.attributes) {
            emitProperty(element, variable, name, value, output);
        }
        for (const Element& child : element.children) {
            const std::string childVariable = emitElement(child, output, index);
            output << "        " << variable << "->addChild(std::move(" << childVariable << "));\n";
        }
        return variable;
    }

    void compile(const std::filesystem::path& input, const std::filesystem::path& outputPath) {
        // В generated-файл попадает только фабрика дерева; общий layout runtime
        // поставляется отдельной статической библиотекой XamlRuntime.
        const Element root = parse(input, readFile(input));
        std::ostringstream output;
        output << "// Generated by XamlCompiler. Do not edit.\n"
            << "#include \"xaml_runtime/XamlLayout.h\"\n\n"
            << "namespace mobileclock::ui {\n"
            << "    std::unique_ptr<Element> createMainPage() {\n";
        size_t index = 0;
        const std::string rootVariable = emitElement(root, output, index);
        output << "        return " << rootVariable << ";\n"
            << "    }\n"
            << "}";
        std::filesystem::create_directories(outputPath.parent_path());
        std::ofstream file(outputPath, std::ios::binary | std::ios::trunc);
        if (!file) {
            throw std::runtime_error("Cannot write " + outputPath.string());
        }
        file << output.str();
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: XamlCompiler <input.xaml> <output.xaml.cpp>\n";
        return 1;
    }
    try {
        compile(argv[1], argv[2]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "XamlCompiler: " << error.what() << '\n';
        return 1;
    }
}