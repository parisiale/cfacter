#include <facter/facts/resolvers/operating_system_resolver.hpp>
#include <facter/facts/os.hpp>
#include <facter/facts/os_family.hpp>
#include <facter/facts/scalar_value.hpp>
#include <facter/facts/map_value.hpp>
#include <facter/facts/collection.hpp>
#include <facter/facts/fact.hpp>

using namespace std;

namespace facter { namespace facts { namespace resolvers {

    operating_system_resolver::operating_system_resolver() :
        resolver(
            "operating system",
            {
                fact::os,
                fact::operating_system,
                fact::os_family,
                fact::operating_system_release,
                fact::operating_system_major_release,
                fact::lsb_dist_id,
                fact::lsb_dist_release,
                fact::lsb_dist_codename,
                fact::lsb_dist_description,
                fact::lsb_dist_major_release,
                fact::lsb_dist_minor_release,
                fact::lsb_release,
            })
    {
    }

    void operating_system_resolver::resolve_facts(collection& facts)
    {
        auto data = collect_data(facts);

        auto os = make_value<map_value>();
        auto family = determine_os_family(facts, data.name);
        if (!family.empty()) {
            // TODO: remove flat fact
            facts.add(fact::os_family, make_value<string_value>(family));
            os->add("family", make_value<string_value>(move(family)));
        }

        if (!data.release.empty()) {
            auto value = make_value<map_value>();

            string major, minor;
            tie(major, minor) = parse_release(data.name, data.release);

            if (!major.empty()) {
                // TODO: remove flat fact
                facts.add(fact::operating_system_major_release, make_value<string_value>(major));
                value->add("major", make_value<string_value>(move(major)));
            }
            if (!minor.empty()) {
                value->add("minor", make_value<string_value>(move(minor)));
            }

            // TODO: remove flat fact
            facts.add(fact::operating_system_release, make_value<string_value>(data.release));
            value->add("full", make_value<string_value>(move(data.release)));

            os->add("release", move(value));
        }

        // Add distro facts
        auto distro = make_value<map_value>();
        if (!data.distro.id.empty()) {
            // TODO: remove flat fact
            facts.add(fact::lsb_dist_id, make_value<string_value>(data.distro.id));
            distro->add("id", make_value<string_value>(move(data.distro.id)));
        }
        if (!data.distro.codename.empty()) {
            // TODO: remove flat fact
            facts.add(fact::lsb_dist_codename, make_value<string_value>(data.distro.codename));
            distro->add("codename", make_value<string_value>(move(data.distro.codename)));
        }
        if (!data.distro.description.empty()) {
            // TODO: remove flat fact
            facts.add(fact::lsb_dist_description, make_value<string_value>(data.distro.description));
            distro->add("description", make_value<string_value>(move(data.distro.description)));
        }
        if (!data.distro.release.empty()) {
            auto value = make_value<map_value>();

            string major, minor;
            tie(major, minor) = parse_release(data.name, data.distro.release);

            if (major.empty()) {
                major = data.distro.release;
            }
            // TODO: remove flat fact
            facts.add(fact::lsb_dist_major_release, make_value<string_value>(major));
            value->add("major", make_value<string_value>(move(major)));

            if (!minor.empty()) {
                // TODO: remove flat fact
                facts.add(fact::lsb_dist_minor_release, make_value<string_value>(minor));
                value->add("minor", make_value<string_value>(move(minor)));
            }

            // TODO: remove flat fact
            facts.add(fact::lsb_dist_release, make_value<string_value>(data.distro.release));
            value->add("full", make_value<string_value>(move(data.distro.release)));
            distro->add("release", move(value));
        }
        if (!data.specification_version.empty()) {
            // TODO: remove flat fact
            facts.add(fact::lsb_release, make_value<string_value>(data.specification_version));
            distro->add("specification", make_value<string_value>(move(data.specification_version)));
        }

        // Add the name last since the above release parsing is dependent on it
        if (!data.name.empty()) {
            // TODO: remove flat fact
            facts.add(fact::operating_system, make_value<string_value>(data.name));
            os->add("name", make_value<string_value>(move(data.name)));
        }

        if (!distro->empty()) {
             os->add("distro", move(distro));
        }

        if (!os->empty()) {
            facts.add(fact::os, move(os));
        }
    }

    operating_system_resolver::data operating_system_resolver::collect_data(collection& facts)
    {
        data result;

        auto kernel = facts.get<string_value>(fact::kernel);
        if (kernel) {
            result.name = kernel->value();
        }

        auto release = facts.get<string_value>(fact::kernel_release);
        if (release) {
            result.release = release->value();
        }

        return result;
    }

    tuple<string, string> operating_system_resolver::parse_release(string const& name, string const& release) const
    {
        auto pos = release.find('.');
        if (pos != string::npos) {
            auto second = release.find('.', pos + 1);
            return make_tuple(release.substr(0, pos), release.substr(pos + 1, second - (pos + 1)));
        }
        return make_tuple(release, string());
    }

    string operating_system_resolver::determine_os_family(collection& facts, string const& name) const
    {
        if (!name.empty()) {
            static map<string, string> const systems = {
                { string(os::redhat),                   string(os_family::redhat) },
                { string(os::fedora),                   string(os_family::redhat) },
                { string(os::centos),                   string(os_family::redhat) },
                { string(os::scientific),               string(os_family::redhat) },
                { string(os::scientific_cern),          string(os_family::redhat) },
                { string(os::ascendos),                 string(os_family::redhat) },
                { string(os::cloud_linux),              string(os_family::redhat) },
                { string(os::psbm),                     string(os_family::redhat) },
                { string(os::oracle_linux),             string(os_family::redhat) },
                { string(os::oracle_vm_linux),          string(os_family::redhat) },
                { string(os::oracle_enterprise_linux),  string(os_family::redhat) },
                { string(os::amazon),                   string(os_family::redhat) },
                { string(os::xen_server),               string(os_family::redhat) },
                { string(os::linux_mint),               string(os_family::debian) },
                { string(os::ubuntu),                   string(os_family::debian) },
                { string(os::debian),                   string(os_family::debian) },
                { string(os::cumulus),                  string(os_family::debian) },
                { string(os::suse_enterprise_server),   string(os_family::suse) },
                { string(os::suse_enterprise_desktop),  string(os_family::suse) },
                { string(os::open_suse),                string(os_family::suse) },
                { string(os::suse),                     string(os_family::suse) },
                { string(os::sunos),                    string(os_family::solaris) },
                { string(os::solaris),                  string(os_family::solaris) },
                { string(os::nexenta),                  string(os_family::solaris) },
                { string(os::omni),                     string(os_family::solaris) },
                { string(os::open_indiana),             string(os_family::solaris) },
                { string(os::smart),                    string(os_family::solaris) },
                { string(os::gentoo),                   string(os_family::gentoo) },
                { string(os::archlinux),                string(os_family::archlinux) },
                { string(os::mandrake),                 string(os_family::mandrake) },
                { string(os::mandriva),                 string(os_family::mandrake) },
                { string(os::mageia),                   string(os_family::mandrake) },
            };
            auto const& it = systems.find(name);
            if (it != systems.end()) {
                return it->second;
            }
        }

        // Default to the same value as the kernel
        auto kernel = facts.get<string_value>(fact::kernel);
        if (!kernel) {
            return {};
        }
        return  kernel->value();
    }

}}}  // namespace facter::facts::resolvers
