#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>

#include <sdkgenny/enum.hpp>
#include <sdkgenny/function.hpp>
#include <sdkgenny/namespace.hpp>
#include <sdkgenny/struct.hpp>
#include <sdkgenny/type.hpp>
#include <sdkgenny/virtual_function.hpp>

namespace sdkgenny {
class Sdk : public Object {
public:
    Sdk();

    auto global_ns() const { return m_global_ns.get(); }

    auto preamble(std::string_view preamble) {
        m_preamble = preamble;
        return this;
    }
    auto postamble(std::string_view postamble) {
        m_postamble = postamble;
        return this;
    }

    auto include(std::string_view header) {
        m_includes.emplace(header);
        return this;
    }
    auto include_local(std::string_view header) {
        m_local_includes.emplace(header);
        return this;
    }

    void generate(const std::filesystem::path& sdk_path) const;

    const auto& header_extension() const { return m_header_extension; }
    auto header_extension(std::string_view ext) {
        m_header_extension = ext;
        return this;
    }

    const auto& source_extension() const { return m_source_extension; }
    auto source_extension(std::string_view ext) {
        m_source_extension = ext;
        return this;
    }

    const auto& generate_namespaces() const { return m_generate_namespaces; }
    auto generate_namespaces(bool gen_ns) {
        m_generate_namespaces = gen_ns;
        return this;
    }

    // These are intended to be used by either the genny parser or tooling such
    // as ReGenny.
    const auto& imports() const { return m_imports; }
    auto import(std::filesystem::path filepath) {
        m_imports.emplace(std::move(filepath));
        return this;
    }

protected:
    std::unique_ptr<Namespace> m_global_ns{std::make_unique<Namespace>("")};
    std::string m_preamble{};
    std::string m_postamble{};
    std::set<std::string> m_includes{};
    std::set<std::string> m_local_includes{};
    std::set<std::filesystem::path> m_imports{};
    std::string m_header_extension{".hpp"};
    std::string m_source_extension{".cpp"};
    bool m_generate_namespaces{true};

    void generate_namespace(const std::filesystem::path& sdk_path, Namespace* ns) const;

    template <typename T> void generate_header(const std::filesystem::path& sdk_path, T* obj) const {
        if (obj->skip_generation()) {
            return;
        }

        auto obj_inc_path = sdk_path / (obj->path() += m_header_extension);
        std::ofstream file_list{sdk_path / "file_list.txt", std::ios::app};
        file_list << "\"" << obj_inc_path.string() << "\" \\\n";
        std::filesystem::create_directories(obj_inc_path.parent_path());
        std::ofstream os{obj_inc_path};

        if (!m_preamble.empty()) {
            std::istringstream sstream{m_preamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }

        os << "#pragma once\n";

        for (auto&& include : m_includes) {
            os << "#include <" << include << ">\n";
        }

        for (auto&& include : m_local_includes) {
            os << "#include \"" << include << "\"\n";
        }

        std::unordered_set<Type*> types_to_include{};
        std::unordered_set<Type*> types_to_forward_decl{};
        std::set<std::filesystem::path> includes{};

        if (auto s = dynamic_cast<Struct*>(obj)) {
            auto deps = s->dependencies();
            types_to_include = deps.hard;
            types_to_forward_decl = deps.soft;
        }

        for (auto&& ty : types_to_include) {
            includes.emplace(ty->path() += m_header_extension);
        }

        for (auto&& inc : includes) {
            os << "#include \"" << std::filesystem::relative(inc, obj->path().parent_path()).string() << "\"\n";
        }

        for (auto&& type : types_to_forward_decl) {
            auto owners = type->owners<Namespace>();

            if (owners.size() > 1 && m_generate_namespaces) {
                std::reverse(owners.begin(), owners.end());

                os << "namespace ";

                for (auto&& owner : owners) {
                    if (owner->usable_name().empty()) {
                        continue;
                    }

                    os << owner->usable_name();

                    if (owner != owners.back()) {
                        os << "::";
                    }
                }

                os << " {\n";
            }

            if (auto s = dynamic_cast<Struct*>(type)) {
                s->generate_forward_decl(os);
            } else if (auto e = dynamic_cast<Enum*>(type)) {
                e->generate_forward_decl(os);
            }

            if (owners.size() > 1 && m_generate_namespaces) {
                os << "}\n";
            }
        }

        auto owners = obj->template owners<Namespace>();

        if (owners.size() > 1 && m_generate_namespaces) {
            std::reverse(owners.begin(), owners.end());

            os << "namespace ";

            for (auto&& owner : owners) {
                if (owner->usable_name().empty()) {
                    continue;
                }

                os << owner->usable_name();

                if (owner != owners.back()) {
                    os << "::";
                }
            }

            os << " {\n";
        }

        os << "#define GENNY_PRIVATE(decl) private: decl; public:\n";
        os << "#pragma pack(push, 1)\n";
        obj->generate(os);
        os << "#pragma pack(pop)\n";
        os << "#undef GENNY_PRIVATE\n";

        if (owners.size() > 1 && m_generate_namespaces) {
            os << "}\n";
        }

        if (!m_postamble.empty()) {
            std::istringstream sstream{m_postamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }
    }

    template <typename T> void generate_source(const std::filesystem::path& sdk_path, T* obj) const {
        if (obj->skip_generation()) {
            return;
        }

        // Skip generating a source file for an object with no functions.
        if (!obj->template has_any<Function>()) {
            return;
        }

        // Skip generating a source file for an object if all the functions it does have lack a procedure.
        std::unordered_set<Function*> functions{};
        obj->template get_all_in_children<Function>(functions);

        auto any_procedure = false;

        for (auto&& fn : functions) {
            if (!fn->procedure().empty()) {
                any_procedure = true;
                break;
            }
        }

        if (!any_procedure) {
            return;
        }

        auto obj_src_path = sdk_path / (obj->path() += m_source_extension);
        std::ofstream file_list{sdk_path / "file_list.txt", std::ios::app};
        file_list << "\"" << obj_src_path.string() << "\" \\\n";

        std::filesystem::create_directories(obj_src_path.parent_path());
        std::ofstream os{obj_src_path};

        if (!m_preamble.empty()) {
            std::istringstream sstream{m_preamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }

        std::unordered_set<Type*> types_to_include{};

        if (auto s = dynamic_cast<Struct*>(obj)) {
            auto deps = s->dependencies();
            types_to_include = deps.hard;
            types_to_include.merge(deps.soft);
            types_to_include.emplace(s);
        }

        for (auto&& fn : functions) {
            auto deps = fn->dependencies();
            types_to_include.merge(deps);
        }

        std::set<std::filesystem::path> includes{};

        for (auto&& ty : types_to_include) {
            includes.emplace(ty->path() += m_header_extension);
        }

        for (auto&& inc : includes) {
            os << "#include \"" << std::filesystem::relative(inc, obj->path().parent_path()).string() << "\"\n";
        }

        for (auto&& fn : functions) {
            // Skip pure virtual functions.
            if (fn->is_a<VirtualFunction>() && fn->procedure().empty()) {
                continue;
            }

            fn->generate_source(os);
        }

        if (!m_postamble.empty()) {
            std::istringstream sstream{m_postamble};
            std::string line{};

            while (std::getline(sstream, line)) {
                os << "// " << line << "\n";
            }
        }
    }

    template <typename T> void generate(const std::filesystem::path& sdk_path, Namespace* ns) const {
        for (auto&& obj : ns->get_all<T>()) {
            generate_header(sdk_path, obj);
            generate_source(sdk_path, obj);
        }
    }
};

} // namespace sdkgenny