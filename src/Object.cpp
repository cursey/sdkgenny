#include "sdkgenny/Namespace.hpp"
#include "sdkgenny/Struct.hpp"

#include "sdkgenny/Object.hpp"

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

std::unique_ptr<Object> Object::remove(Object* obj) {
    obj->m_owner = nullptr;

    if (auto search = std::find_if(m_children.begin(), m_children.end(), [obj](auto&& c) { return c.get() == obj; });
        search != m_children.end()) {
        auto p = std::move(*search);
        m_children.erase(search);
        return p;
    }
    /* m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(), [obj](auto&& c) { return c.get() == obj; }));*/
    return nullptr;
}

std::filesystem::path Object::path() {
    if (m_owner == nullptr) {
        return usable_name();
    }

    std::filesystem::path p{};
    auto os = owners<Object>();

    std::reverse(os.begin(), os.end());

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
