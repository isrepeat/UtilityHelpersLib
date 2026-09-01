#include "XamlRuntime/ElementBuilder.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace xaml::_details {
    attr::Color ParseColor(std::string_view value) {
        std::string normalized(value);
        if (normalized == "black") {
            normalized = "#000000";
        } else if (normalized == "blue") {
            normalized = "#0000FF";
        } else if (normalized == "gray") {
            normalized = "#808080";
        } else if (normalized == "green") {
            normalized = "#008000";
        } else if (normalized == "red") {
            normalized = "#FF0000";
        } else if (normalized == "white") {
            normalized = "#FFFFFF";
        }
        if ((normalized.size() != 7 && normalized.size() != 9) || normalized.front() != '#') {
            throw std::invalid_argument("color must use #RRGGBB, #AARRGGBB or a supported name");
        }
        const unsigned long color = std::stoul(normalized.substr(1), nullptr, 16);
        const unsigned long alpha = normalized.size() == 9 ? (color >> 24) & 0xff : 0xff;
        return {
            static_cast<float>((color >> 16) & 0xff) / 255.0f,
            static_cast<float>((color >> 8) & 0xff) / 255.0f,
            static_cast<float>(color & 0xff) / 255.0f,
            static_cast<float>(alpha) / 255.0f,
        };
    }

    attr::Thickness ParseThickness(std::string_view value) {
        std::string normalized(value);
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
            throw std::invalid_argument("thickness must contain left right top bottom");
        }
        return {values[0], values[1], values[2], values[3]};
    }

    attr::Alignment ParseAlignment(std::string_view value) {
        if (value == "Stretch") {
            return attr::Alignment::stretch;
        }
        if (value == "Left") {
            return attr::Alignment::left;
        }
        if (value == "Right") {
            return attr::Alignment::right;
        }
        if (value == "Top") {
            return attr::Alignment::top;
        }
        if (value == "Bottom") {
            return attr::Alignment::bottom;
        }
        if (value == "Center") {
            return attr::Alignment::center;
        }
        throw std::invalid_argument("unsupported alignment");
    }

    bool ParseBool(std::string_view value) {
        if (value == "True" || value == "true") {
            return true;
        }
        if (value == "False" || value == "false") {
            return false;
        }
        throw std::invalid_argument("boolean value must be True or False");
    }
}

namespace xaml {
    ElementType ParseElementType(std::string_view name) {
        if (name == "Page") {
            return ElementType::page;
        }
        if (name == "StackPanel") {
            return ElementType::stackPanel;
        }
        if (name == "TextBlock") {
            return ElementType::textBlock;
        }
        if (name == "Button") {
            return ElementType::button;
        }
        if (name == "Border") {
            return ElementType::border;
        }
        if (name == "ToggleSwitch") {
            return ElementType::toggleSwitch;
        }
        if (name == "Grid") {
            return ElementType::grid;
        }
        if (name == "ScrollViewer") {
            return ElementType::scrollViewer;
        }
        if (name == "Image") {
            return ElementType::image;
        }
        if (name == "SvgImage") {
            return ElementType::svgImage;
        }
        if (name == "IconButton") {
            return ElementType::iconButton;
        }
        if (name == "ListView") {
            return ElementType::listView;
        }
        throw std::invalid_argument("unsupported XAML element");
    }

    void SetAttribute(Element& element, std::string_view name, std::string_view value) {
        if (name == "id") {
            element.SetId(std::string(value));
        } else if (name == "text") {
            element.SetText(std::string(value));
        } else if (name == "fontSize") {
            element.SetFontSize(std::stof(std::string(value)));
        } else if (name == "fontFamily") {
            element.SetFontFamily(std::string(value));
        } else if (name == "fontWeight") {
            element.SetFontWeight(std::string(value));
        } else if (name == "source") {
            element.SetSource(std::string(value));
        } else if (name == "tint") {
            element.SetTint(_details::ParseColor(value));
        } else if (name == "command") {
            element.SetCommand(std::string(value));
        } else if (name == "foreground") {
            element.SetForeground(_details::ParseColor(value));
        } else if (name == "orientation") {
            if (value != "Horizontal" && value != "Vertical") {
                throw std::invalid_argument("orientation must be Horizontal or Vertical");
            }
            element.SetOrientation(value == "Horizontal"
                ? attr::Orientation::horizontal : attr::Orientation::vertical);
        } else if (name == "verticalAlignment") {
            element.SetVerticalAlignment(_details::ParseAlignment(value));
        } else if (name == "horizontalAlignment") {
            element.SetHorizontalAlignment(_details::ParseAlignment(value));
        } else if (name == "gridRow") {
            element.SetGridRow(std::stoi(std::string(value)));
        } else if (name == "gridColumn") {
            element.SetGridColumn(std::stoi(std::string(value)));
        } else if (name == "rows") {
            element.SetRows(std::string(value));
        } else if (name == "columns") {
            element.SetColumns(std::string(value));
        } else if (name == "background") {
            element.SetBackground(_details::ParseColor(value));
        } else if (name == "borderBrush") {
            element.SetBorderColor(_details::ParseColor(value));
        } else if (name == "margin") {
            element.SetMargin(_details::ParseThickness(value));
        } else if (name == "padding") {
            element.SetPadding(_details::ParseThickness(value));
        } else if (name == "borderThickness") {
            element.SetBorderThickness(_details::ParseThickness(value));
        } else if (name == "cornerRadius") {
            element.SetCornerRadius(std::stof(std::string(value)));
        } else if (name == "width") {
            element.SetWidth(std::stof(std::string(value)));
        } else if (name == "height") {
            element.SetHeight(std::stof(std::string(value)));
        } else if (name == "isOn") {
            element.SetIsOn(_details::ParseBool(value));
        } else if (name == "visibility") {
            if (value != "Collapsed" && value != "Hidden" && value != "Visible") {
                throw std::invalid_argument("unsupported visibility");
            }
            element.SetVisibility(value == "Collapsed" ? attr::Visibility::collapsed
                : value == "Hidden" ? attr::Visibility::hidden : attr::Visibility::visible);
        } else if (name == "isEnabled") {
            element.SetIsEnabled(_details::ParseBool(value));
        } else if (name == "opacity") {
            element.SetOpacity(std::stof(std::string(value)));
        } else {
            throw std::invalid_argument("unsupported XAML attribute");
        }
    }
}