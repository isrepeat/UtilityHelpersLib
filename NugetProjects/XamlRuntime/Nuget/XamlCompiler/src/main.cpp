#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
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

    struct Binding {
        std::string elementVariable;
        std::string property;
        std::string sourceProperty;
        std::string mode;
    };

    class XamlCompiler {
    public:
        void Compile(const std::filesystem::path& input, const std::filesystem::path& outputPath);

    private:
        std::string ReadFile(const std::filesystem::path& path) {
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                throw std::runtime_error("Cannot read " + path.string());
            }
            return {
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>(),
            };
        }

        std::string EscapeCpp(const std::string& value) {
            std::string result;
            for (const char character : value) {
                if (character == '\\' || character == '"') {
                    result += '\\';
                }
                result += character;
            }
            return result;
        }

        size_t LineNumber(const std::string& source, size_t offset) {
            return 1 + static_cast<size_t>(
                std::count(source.begin(), source.begin() + offset, '\n'));
        }

        [[noreturn]] void Fail(
            const std::filesystem::path& path,
            const std::string& source,
            size_t offset,
            const std::string& message) {
            throw std::runtime_error(
                path.string() + ":" + std::to_string(this->LineNumber(source, offset))
                + ": " + message);
        }

        void AppendElement(
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
                this->Fail(path, source, offset, "only one root element is allowed");
            }
            root = std::move(element);
            hasRoot = true;
        }

        Element Parse(const std::filesystem::path& path, const std::string& source) {
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
                        this->Fail(path, source, offset, "unexpected closing tag </" + name + ">");
                    }
                    Element element = std::move(stack.back());
                    stack.pop_back();
                    this->AppendElement(std::move(element), stack, root, hasRoot, path, source, offset);
                    continue;
                }

                std::smatch nameMatch;
                std::regex_search(content, nameMatch, namePattern);
                if (nameMatch.empty()) {
                    this->Fail(path, source, offset, "element name is required");
                }
                Element element{nameMatch[1].str(), {}, {}, offset};
                for (std::sregex_iterator attribute(
                    content.begin(), content.end(), attributePattern);
                    attribute != end;
                    ++attribute) {
                    element.attributes.emplace_back((*attribute)[1].str(), (*attribute)[2].str());
                }
                if (selfClosing) {
                    this->AppendElement(std::move(element), stack, root, hasRoot, path, source, offset);
                } else {
                    stack.push_back(std::move(element));
                }
            }

            if (!stack.empty()) {
                this->Fail(path, source, stack.back().offset, "element is not closed");
            }
            if (!hasRoot) {
                this->Fail(path, source, 0, "root element is required");
            }
            return root;
        }

        std::string ElementTypeName(const Element& element) {
            if (element.name == "Page") {
                return "page";
            }
            if (element.name == "StackPanel") {
                return "stackPanel";
            }
            if (element.name == "TextBlock") {
                return "textBlock";
            }
            if (element.name == "Button") {
                return "button";
            }
            if (element.name == "Border") {
                return "border";
            }
            if (element.name == "ToggleSwitch") {
                return "toggleSwitch";
            }
            throw std::runtime_error("Unsupported XAML element <" + element.name + ">");
        }

        std::string FloatLiteral(const std::string& value) {
            return std::to_string(std::stof(value)) + "f";
        }

        std::string ThicknessLiteral(const std::string& value) {
            std::string normalized = value;
            std::replace(normalized.begin(), normalized.end(), ',', ' ');
            std::istringstream input(normalized);
            std::vector<float> values;
            float component = 0.0f;
            while (input >> component) {
                values.push_back(component);
            }
            if (values.size() == 1) {
                values = {values[0], values[0], values[0], values[0]};
            }
            if (values.size() != 4) {
                throw std::runtime_error("Thickness must contain one or four values: left right top bottom");
            }
            return "attr::Thickness{" + std::to_string(values[0]) + "f, " + std::to_string(values[1])
                + "f, " + std::to_string(values[2]) + "f, " + std::to_string(values[3]) + "f}";
        }

        std::string ColorLiteral(const std::string& name, const std::string& value) {
            if (value.size() != 7 || value.front() != '#') {
                throw std::runtime_error(name + " must use #RRGGBB");
            }
            const unsigned long color = std::stoul(value.substr(1), nullptr, 16);
            return "attr::Color{" + std::to_string((color >> 16) & 0xff) + ".0f / 255.0f, "
                + std::to_string((color >> 8) & 0xff) + ".0f / 255.0f, "
                + std::to_string(color & 0xff) + ".0f / 255.0f, 1.0f}";
        }

        bool TryEmitBinding(
            const std::string& elementVariable,
            const std::string& name,
            const std::string& value,
            std::vector<Binding>& bindings) {
            const std::regex bindingPattern(
                R"(^\{Binding\s+([A-Za-z][A-Za-z0-9]*)(?:\s*,\s*Mode\s*=\s*(OneWay|TwoWay))?\s*\}$)");
            std::smatch match;
            if (!std::regex_match(value, match, bindingPattern)) {
                return false;
            }
            bindings.push_back({
                elementVariable,
                name,
                match[1].str(),
                match[2].matched && match[2].str() == "TwoWay" ? "twoWay" : "oneWay",
            });
            return true;
        }

        void EmitProperty(
            const Element& element,
            const std::string& variable,
            const std::string& name,
            const std::string& value,
            std::ostringstream& output,
            const std::string& elementVariable,
            std::vector<Binding>& bindings) {
            // Атрибуты переводятся в явные вызовы setter'ов. Поэтому итоговый код
            // не разбирает строки в рантайме и остаётся обычным C++.
            if (this->TryEmitBinding(elementVariable, name, value, bindings)) {
                return;
            }
            if (name == "id") {
                output << "            " << variable << "->SetId(\"" << this->EscapeCpp(value) << "\");\n";
            } else if (name == "text") {
                output << "            " << variable << "->SetText(\"" << this->EscapeCpp(value) << "\");\n";
            } else if (name == "fontSize") {
                output << "            " << variable << "->SetFontSize(" << this->FloatLiteral(value) << ");\n";
            } else if (name == "fontFamily") {
                output << "            " << variable << "->SetFontFamily(\"" << this->EscapeCpp(value) << "\");\n";
            } else if (name == "fontWeight") {
                output << "            " << variable << "->SetFontWeight(\"" << this->EscapeCpp(value) << "\");\n";
            } else if (name == "margin" || name == "padding") {
                output << "            " << variable << "->Set" << (name == "margin" ? "Margin" : "Padding")
                    << "(" << this->ThicknessLiteral(value) << ");\n";
            } else if (name == "border" || name == "borderThickness") {
                output << "            " << variable << "->SetBorderThickness(" << this->ThicknessLiteral(value) << ");\n";
            } else if (name == "cornerRadius") {
                output << "            " << variable << "->SetCornerRadius(" << this->FloatLiteral(value) << ");\n";
            } else if (name == "width") {
                output << "            " << variable << "->SetWidth(" << this->FloatLiteral(value) << ");\n";
            } else if (name == "height") {
                output << "            " << variable << "->SetHeight(" << this->FloatLiteral(value) << ");\n";
            } else if (name == "isOn") {
                if (value != "True" && value != "False") {
                    throw std::runtime_error("IsOn must be True or False");
                }
                output << "            " << variable << "->SetIsOn(" << (value == "True" ? "true" : "false") << ");\n";
            } else if (name == "orientation") {
                const std::string orientation = value == "Horizontal" ? "horizontal"
                    : value == "Vertical" ? "vertical" : "";
                if (orientation.empty()) {
                    throw std::runtime_error("Orientation must be Horizontal or Vertical");
                }
                output << "            " << variable << "->SetOrientation(attr::Orientation::"
                    << orientation << ");\n";
            } else if (name == "verticalAlignment") {
                if (value == "Top") {
                    output << "            " << variable << "->SetVerticalAlignment(attr::Alignment::top);\n";
                } else if (value != "Center") {
                    throw std::runtime_error("VerticalAlignment must be Top or Center");
                }
            } else if (name == "foreground" || name == "background" || name == "borderColor") {
                const std::string setter = name == "foreground" ? "Foreground"
                    : name == "background" ? "Background" : "BorderColor";
                output << "            " << variable << "->Set" << setter << "(" << this->ColorLiteral(name, value) << ");\n";
            } else if (name != "horizontalAlignment") {
                throw std::runtime_error(
                    "Unsupported attribute " + name + " on <" + element.name + ">");
            }
        }

        std::string ElementVariableName(
            const Element& element,
            std::map<std::string, size_t>& elementCounts) {
            std::string result = element.name;
            result.front() = static_cast<char>(result.front() - 'A' + 'a');
            if (element.name == "Page") {
                return result;
            }
            return result + std::to_string(++elementCounts[element.name]);
        }

        std::string EmitElement(
            const Element& element,
            std::ostringstream& output,
            std::map<std::string, size_t>& elementCounts) {
            // Сначала объявляем дочерние unique_ptr, затем передаём их родителю.
            // Это повторяет ownership-структуру исходной XAML-разметки.
            const std::string variable = this->ElementVariableName(element, elementCounts);
            std::vector<Binding> bindings;
            output << "            auto " << variable << " = std::make_unique<Element>(ElementType::"
                << this->ElementTypeName(element) << ");\n";
            for (const auto& [name, value] : element.attributes) {
                this->EmitProperty(element, variable, name, value, output, variable, bindings);
            }
            this->EmitBindings(bindings, output);
            for (size_t childIndex = 0; childIndex < element.children.size(); ++childIndex) {
                const std::string childVariable = this->EmitElement(
                    element.children[childIndex], output, elementCounts);
                output << "            " << variable << "->AddChild(std::move(" << childVariable << "));\n";
            }
            return variable;
        }

        std::string PropertyEnumName(const std::string& propertyName) {
            std::string result = propertyName;
            result.front() = static_cast<char>(result.front() - 'A' + 'a');
            return result;
        }

        void EmitBindingCall(
            const std::string& method,
            const std::vector<std::string>& arguments,
            std::ostringstream& output) {
            std::ostringstream singleLine;
            singleLine << "bindings." << method << "(";
            for (size_t index = 0; index < arguments.size(); ++index) {
                if (index != 0) {
                    singleLine << ", ";
                }
                singleLine << arguments[index];
            }
            singleLine << ");";
            if (12 + singleLine.str().size() <= 130) {
                output << "            " << singleLine.str() << "\n";
                return;
            }
            output << "            bindings." << method << "(\n";
            for (size_t index = 0; index < arguments.size(); ++index) {
                output << "                " << arguments[index];
                output << (index + 1 == arguments.size() ? "\n" : ",\n");
            }
            output << "            );\n";
        }

        void EmitBindings(const std::vector<Binding>& bindings, std::ostringstream& output) {
            for (const Binding& binding : bindings) {
                const std::string element = "*" + binding.elementVariable;
                const std::string property = "TViewModel::Property::"
                    + this->PropertyEnumName(binding.sourceProperty);
                if (binding.property == "text") {
                    if (binding.mode == "twoWay") {
                        this->EmitBindingCall("AddTwoWay", {
                            element,
                            "viewModel",
                            "&TViewModel::" + binding.sourceProperty,
                            "&TViewModel::Set" + binding.sourceProperty,
                            "&Element::Text",
                            "&Element::SetText",
                            property,
                        }, output);
                    } else {
                        this->EmitBindingCall("AddOneWay", {
                            element,
                            "viewModel",
                            "&TViewModel::" + binding.sourceProperty,
                            "&Element::SetText",
                            property,
                        }, output);
                    }
                } else if (binding.property == "isOn") {
                    if (binding.mode == "twoWay") {
                        this->EmitBindingCall("AddTwoWay", {
                            element,
                            "viewModel",
                            "&TViewModel::" + binding.sourceProperty,
                            "&TViewModel::Set" + binding.sourceProperty,
                            "&Element::IsOn",
                            "&Element::SetIsOn",
                            property,
                        }, output);
                    } else {
                        this->EmitBindingCall("AddOneWay", {
                            element,
                            "viewModel",
                            "&TViewModel::" + binding.sourceProperty,
                            "&Element::SetIsOn",
                            property,
                        }, output);
                    }
                } else {
                    throw std::runtime_error("Unsupported binding target: " + binding.property);
                }
            }
        }

    };

    void XamlCompiler::Compile(const std::filesystem::path& input, const std::filesystem::path& outputPath) {
        // Каждая XAML-страница получает собственный тип. Контроллеры работают
        // с MainPage::Create(), а не с неявной свободной функцией.
        const Element root = this->Parse(input, this->ReadFile(input));
        const std::string pageName = input.stem().string();
        if (!std::regex_match(pageName, std::regex(R"([A-Za-z][A-Za-z0-9]*)"))) {
            throw std::runtime_error("XAML filename must be a valid C++ type name");
        }
        const std::filesystem::path headerPath = outputPath.parent_path()
            / (outputPath.stem().string() + ".h");

        std::ostringstream header;
        header << "// Generated by XamlCompiler. Do not edit.\n"
            << "#pragma once\n\n"
            << "#include \"XamlRuntime/Binding.h\"\n"
            << "#include \"XamlRuntime/XamlLayout.h\"\n\n"
            << "namespace xaml::generated {\n"
            << "    class " << pageName << " final {\n"
            << "    public:\n"
            << "        template <typename TViewModel>\n"
            << "        static std::unique_ptr<Element> Create(TViewModel& viewModel, BindingScope& bindings) {\n";
        std::map<std::string, size_t> elementCounts;
        const std::string rootVariable = this->EmitElement(root, header, elementCounts);
        header << "            return " << rootVariable << ";\n"
            << "        }\n"
            << "    };\n"
            << "}";

        std::ostringstream output;
        output << "// Generated by XamlCompiler. Do not edit.\n"
            << "#include \"" << headerPath.filename().string() << "\"";
        std::filesystem::create_directories(outputPath.parent_path());
        std::ofstream headerFile(headerPath, std::ios::binary | std::ios::trunc);
        if (!headerFile) {
            throw std::runtime_error("Cannot write " + headerPath.string());
        }
        headerFile << header.str();
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
        XamlCompiler compiler;
        compiler.Compile(argv[1], argv[2]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "XamlCompiler: " << error.what() << '\n';
        return 1;
    }
}