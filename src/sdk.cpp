#include <sdkgenny/sdk.hpp>

namespace sdkgenny {
Sdk::Sdk() : Object{"Sdk"} {
    m_global_ns->m_owner = this;
}

void Sdk::generate(const std::filesystem::path& sdk_path) const {
    // erase the file_list.txt
    std::filesystem::remove(sdk_path / "file_list.txt");

    generate_namespace(sdk_path, m_global_ns.get());
}

void Sdk::generate_namespace(const std::filesystem::path& sdk_path, Namespace* ns) const {
    generate<Enum>(sdk_path, ns);
    generate<Struct>(sdk_path, ns);

    for (auto&& child : ns->get_all<Namespace>()) {
        generate_namespace(sdk_path, child);
    }
}

} // namespace sdkgenny