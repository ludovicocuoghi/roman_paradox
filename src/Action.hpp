#pragma once

#include <sstream>
#include <string>

class Action {
private:
    std::string m_name;
    std::string m_type;

public:
    Action() = default;
    Action(const std::string& name, const std::string& type) : m_name(name), m_type(type) {}

    const std::string& name() const { return m_name; }
    const std::string& type() const { return m_type; }

    std::string toString() const {
        std::ostringstream ss;
        ss << "Action [name: " << m_name << ", type: " << m_type << "]";
        return ss.str();
    }

    bool operator==(const Action& other) const {
        return m_name == other.m_name && m_type == other.m_type;
    }

    bool operator!=(const Action& other) const {
        return !(*this == other);
    }
};
