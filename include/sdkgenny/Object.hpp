#pragma once

#include <algorithm>
#include <filesystem>
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

    template <typename T> bool is_a() const { return dynamic_cast<const T*>(this) != nullptr; }

    // Searches for an owner of the correct type.
    template <typename T> const T* owner() const {
        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (owner->is_a<T>()) {
                return (const T*)owner;
            }
        }

        return nullptr;
    }

    template <typename T> T* owner() { return (T*)((const Object*)this)->owner<T>(); }

    template <typename T> const T* topmost_owner() const {
        const T* topmost{};

        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (owner->is_a<T>()) {
                topmost = (const T*)owner;
            }
        }

        return topmost;
    }

    template <typename T> T* topmost_owner() { return (T*)((const Object*)this)->topmost_owner<T>(); }

    auto direct_owner() const { return m_owner; }

    template <typename T> std::vector<T*> owners() const {
        std::vector<T*> owners{};

        for (auto owner = m_owner; owner != nullptr; owner = owner->m_owner) {
            if (owner->is_a<T>()) {
                owners.emplace_back((T*)owner);
            }
        }

        return owners;
    }

    template <typename T> std::vector<T*> get_all() const {
        std::vector<T*> children{};

        for (auto&& child : m_children) {
            if (child->is_a<T>()) {
                children.emplace_back((T*)child.get());
            }
        }

        return children;
    }

    template <typename T> void get_all_in_children(std::unordered_set<T*>& objects) const {
        if (is_a<T>()) {
            objects.emplace((T*)this);
        }

        for (auto&& child : m_children) {
            child->get_all_in_children(objects);
        }
    }

    template <typename T> bool has_any() const {
        return std::any_of(
            m_children.cbegin(), m_children.cend(), [](const auto& child) { return child->template is_a<T>(); });
    }

    template <typename T> bool has_any_in_children() const {
        return std::any_of(m_children.cbegin(), m_children.cend(),
            [](const auto& child) { return child->template is_a<T>() || child->template has_any_in_children<T>(); });
    }

    template <typename T> bool is_child_of(T* obj) const {
        const auto o = owners<T>();
        return std::any_of(o.cbegin(), o.cend(), [obj](const auto& owner) { return owner == obj; });
    }

    bool is_direct_child_of(Object* obj) const { return m_owner == obj; }

    template <typename T> T* add(std::unique_ptr<T> object) {
        object->m_owner = this;
        return (T*)m_children.emplace_back(std::move(object)).get();
    }

    template <typename T> T* find(std::string_view name) const {
        for (auto&& child : m_children) {
            if (child->is_a<T>() && child->m_name == name) {
                return (T*)child.get();
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
            if (!std::isalnum(c) && std::strchr(allowed_chars, c) == nullptr) {
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

    std::filesystem::path path();

    auto skip_generation(bool g) {
        m_skip_generation = g;
        return this;
    }
    auto skip_generation() { return m_skip_generation; }

protected:
    friend class Type;
    friend class Pointer;
    friend class Namespace;
    friend class Sdk;

    Object* m_owner{};

    std::string m_name{};
    std::vector<std::unique_ptr<Object>> m_children{};
    std::vector<std::string> m_metadata{};

    bool m_skip_generation{};
};
} // namespace sdkgenny
