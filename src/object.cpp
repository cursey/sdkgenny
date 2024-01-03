#include <sstream>

#include <sdkgenny/namespace.hpp>
#include <sdkgenny/struct.hpp>

#include <sdkgenny/object.hpp>

namespace sdkgenny {
Object::Object(std::string_view name) : m_name{name} {
}

void Object::generate_metadata(std::ostream& os) const {
    if (m_metadata.empty()) {
        return;
    }

    os << "// Metadata: ";

    for (auto&& md : m_metadata) {
        os << md;
        if (&md != &*m_metadata.rbegin()) {
            os << md << ", ";
        }
    }

    os << "\n";
}

void Object::generate_comment(std::ostream& os) const {
    if (m_comment.empty()) {
        return;
    }

    std::istringstream iss{m_comment};
    std::string line{};

    while (std::getline(iss, line)) {
        os << "// " << line << "\n";
    }
}

std::unique_ptr<Object> Object::remove(Object* obj) {
    obj->m_owner = nullptr;

    if (auto search = std::ranges::find_if(m_children, [obj](auto&& c) { return c.get() == obj; });
        search != m_children.end()) {
        auto p = std::move(*search);
        m_children.erase(search);
        return p;
    }

    return nullptr;
}

std::filesystem::path Object::path() const {
    if (m_owner == nullptr) {
        return usable_name();
    }

    std::filesystem::path p{};
    auto os = owners<Object>();

    std::ranges::reverse(os);

    for (auto&& o : os) {
        if (o->template is_a<Namespace>()) {
            p /= o->usable_name();
        } else if (o->is_a<Struct>()) {
            p /= o->usable_name();
            break;
        }
    }

    if (m_owner->template is_a<Namespace>()) {
        p /= usable_name();
    }

    return p;
}
} // namespace sdkgenny
