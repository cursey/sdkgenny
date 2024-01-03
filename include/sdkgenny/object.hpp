#pragma once

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace sdkgenny {
class Struct;

class Object {
public:
    Object() = delete;
    explicit Object(std::string_view name);
    virtual ~Object() = default;

    const auto& name() const { return m_name; }
    auto name(std::string name) {
        m_name = std::move(name);
        return this;
    }

    const auto& metadata() const { return m_metadata; }
    auto& metadata() { return m_metadata; }
    virtual void generate_metadata(std::ostream& os) const;

    const std::string& comment() const { return m_comment; }
    Object* comment(std::string_view format, auto&&... args) {
        m_comment = std::vformat(format, std::make_format_args(args...)) + "\n";
        return this;
    }
    Object* append_comment(std::string_view format, auto&&... args) {
        m_comment += std::vformat(format, std::make_format_args(args...)) + "\n";
        return this;
    }
    Object* prepend_comment(std::string_view format, auto&&... args) {
        m_comment = std::vformat(format, std::make_format_args(args...)) + "\n" + m_comment;
        return this;
    }
    virtual void generate_comment(std::ostream& os) const;

    template <typename T> bool is_a() const { return dynamic_cast<const T*>(this) != nullptr; }
    template <typename T> const T* as() const { return dynamic_cast<const T*>(this); }
    template <typename T> T* as() { return dynamic_cast<T*>(this); }

    // Searches for an owner of the correct type.
    template <typename T> const T* owner() const {
        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (auto o = owner->as<T>()) {
                return o;
            }
        }

        return nullptr;
    }

    template <typename T> T* owner() { return const_cast<T*>(const_cast<const Object*>(this)->owner<T>()); }

    template <typename T> const T* topmost_owner() const {
        const T* topmost{};

        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (auto o = owner->as<const T>()) {
                topmost = o;
            }
        }

        return topmost;
    }

    template <typename T> T* topmost_owner() {
        return const_cast<T*>(const_cast<const Object*>(this)->topmost_owner<T>());
    }

    auto direct_owner() const { return m_owner; }

    template <typename T> std::vector<T*> owners() const {
        std::vector<T*> owners{};

        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (auto o = owner->as<T>()) {
                owners.emplace_back(o);
            }
        }

        return owners;
    }

    template <typename T> std::vector<T*> get_all() const {
        std::vector<T*> children{};

        for (auto&& child : m_children) {
            if (auto c = child->as<T>()) {
                children.emplace_back(c);
            }
        }

        return children;
    }

    template <typename T> void get_all_in_children(std::unordered_set<T*>& objects) const {
        if (auto o = as<T>()) {
            objects.emplace(const_cast<T*>(o));
        }

        for (auto&& child : m_children) {
            child->get_all_in_children(objects);
        }
    }

    template <typename T> bool has_any() const {
        return std::ranges::any_of(m_children, [](const auto& child) { return child->template is_a<T>(); });
    }

    template <typename T> bool has_any_in_children() const {
        return std::ranges::any_of(m_children,
            [](const auto& child) { return child->template is_a<T>() || child->template has_any_in_children<T>(); });
    }

    template <typename T> bool is_child_of(T* obj) const {
        auto o = owners<T>();
        return std::ranges::any_of(o, [obj](const auto& owner) { return owner == obj; });
    }

    bool is_direct_child_of(Object* obj) const { return m_owner == obj; }

    template <typename T> T* add(std::unique_ptr<T> object) {
        object->m_owner = this;
        return reinterpret_cast<T*>(m_children.emplace_back(std::move(object)).get());
    }

    template <typename T> T* find(std::string_view name) const {
        for (auto&& child : m_children) {
            if (auto c = child->as<T>(); c != nullptr && c->m_name == name) {
                return c;
            }
        }

        return nullptr;
    }

    template <typename T> T* find_in_owners(std::string_view name, bool include_self) const {
        auto owner = (include_self) ? this : m_owner;

        for (; owner != nullptr; owner = owner->m_owner) {
            if (auto search = owner->find<T>(name)) {
                return search;
            }
        }

        return nullptr;
    }

    template <typename T, typename... TArgs> T* find_or_add(std::string_view name, TArgs... args) {
        if (auto search = find<T>(name)) {
            return search;
        }

        return add(std::make_unique<T>(name, args...));
    }

    template <typename T, typename... TArgs> T* find_in_owners_or_add(std::string_view name, TArgs... args) {
        if (auto search = find_in_owners<T>(name, true)) {
            return search;
        }

        return add(std::make_unique<T>(name, args...));
    }

    // Returns the unique_ptr to the removed object.
    std::unique_ptr<Object> remove(Object* obj);

    template <typename T> void remove_all() {
        for (auto&& child : get_all<T>()) {
            remove(child);
        }
    }

    // Will fix up a desired name so that it's usable as a C++ identifier. Things like spaces get converted to
    // underscores, and we make sure it doesn't begin with a number. More checks could be done here in the future if
    // necessary.
    std::function<std::string()> usable_name = [this] {
        std::string name{};
        constexpr auto allowed_chars = "*&[]:";

        for (auto&& c : m_name) {
            auto cc = static_cast<unsigned char>(c);

            if (!std::isalnum(cc) && std::strchr(allowed_chars, cc) == nullptr) {
                name += '_';
            } else {
                name += c;
            }
        }

        if (!name.empty() && isdigit(name[0])) {
            name = "_" + name;
        }

        return name;
    };

    // The name used when declaring the object (only for types).
    std::function<std::string()> usable_name_decl = usable_name;

    std::filesystem::path path() const;

    auto skip_generation(bool g) {
        m_skip_generation = g;
        return this;
    }
    auto skip_generation() const { return m_skip_generation; }

protected:
    friend class Type;
    friend class Pointer;
    friend class Namespace;
    friend class Sdk;

    Object* m_owner{};

    std::string m_name{};
    std::vector<std::unique_ptr<Object>> m_children{};
    std::vector<std::string> m_metadata{};
    std::string m_comment{};

    bool m_skip_generation{};
};
} // namespace sdkgenny
