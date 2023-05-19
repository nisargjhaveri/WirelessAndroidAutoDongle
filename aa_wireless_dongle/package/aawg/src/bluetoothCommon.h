#pragma once

#include <dbus-cxx.h>

namespace DBus {
    typedef std::map<std::string, DBus::Variant> Properties;
    typedef std::map<std::string, Properties> Interfaces;
    typedef std::map<DBus::Path, Interfaces> ManagedObjects;
}
