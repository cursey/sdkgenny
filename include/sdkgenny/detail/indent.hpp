#pragma once

#include <streambuf>
#include <string>

namespace sdkgenny::detail {
class Indent : public std::streambuf {
public:
    explicit Indent(std::streambuf* dest, int indent = 4);
    explicit Indent(std::ostream& dest, int indent = 4);
    ~Indent() override;

protected:
    int overflow(int ch) override;

private:
    std::streambuf* m_dest{};
    bool m_is_at_start_of_line{true};
    std::string m_indent{};
    std::ostream* m_owner{};
};
} // namespace sdkgenny::detail
