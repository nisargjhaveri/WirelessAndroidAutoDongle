#!/usr/bin/python3

from dbus_next import Variant
from dbus_next.aio import MessageBus
from dbus_next.constants import BusType
from dbus_next.service import ServiceInterface, method

import asyncio

class BluezProfile(ServiceInterface):
    INTERFACE_BLUEZ_PROFILE = "org.bluez.Profile1"

    def __init__(self):
        super().__init__(self.INTERFACE_BLUEZ_PROFILE)

    def Release(self):
        """Cleanup profile object"""
        return

    def NewConnection(self, device_path: "o", fd: "h", fd_properties: "a{sv}"):
        """Cleanup profile object"""
        print(device_path)
        print(fd)
        print(fd_properties)
        return

    def RequestDisconnection(self, device_path: "o"):
        """Cleanup profile object"""
        print(device_path)
        return

class AAWirelessProfile(BluezProfile):
    @method()
    def Release(self):
        return super().Release()

    @method()
    def NewConnection(self, device_path: "o", fd: "h", fd_properties: "a{sv}"):
        print("AA Wireless NewConnection")
        return super().NewConnection(device_path, fd, fd_properties)

    @method()
    def RequestDisconnection(self, device_path: "o"):
        print("AA Wireless RequestDisconnection")
        return super().RequestDisconnection(device_path)

class HSPHSProfile(BluezProfile):
    @method()
    def Release(self):
        return super().Release()

    @method()
    def NewConnection(self, device_path: "o", fd: "h", fd_properties: "a{sv}"):
        print("HSP HS NewConnection")
        return super().NewConnection(device_path, fd, fd_properties)

    @method()
    def RequestDisconnection(self, device_path: "o"):
        print("HSP HS RequestDisconnection")
        return super().RequestDisconnection(device_path)


class AABluetoothProfileHandler:
    bus = None
    adapter = None

    BLUEZ_BUS_NAME = "org.bluez"
    BLUEZ_ROOT_OBJECT_PATH = "/"
    BLUEZ_OBJECT_PATH = "/org/bluez"
    INTERFACE_OBJECT_MANAGER = "org.freedesktop.DBus.ObjectManager"
    INTERFACE_BLUEZ_ADAPTER = "org.bluez.Adapter1"
    INTERFACE_BLUEZ_DEVICE = "org.bluez.Device1"
    INTERFACE_BLUEZ_PROFILE_MANAGER = "org.bluez.ProfileManager1"

    AAWG_PROFILE_OBJECT_PATH = "/com/aawgd/bluetooth/aawg"
    AAWG_PROfILE_UUID = "4de17a00-52cb-11e6-bdf4-0800200c9a66"

    HSP_HS_PROFILE_OBJECT_PATH = "/com/aawgd/bluetooth/hsp"
    HSP_AG_UUID = "00001112-0000-1000-8000-00805f9b34fb"
    HSP_HS_UUID = "00001108-0000-1000-8000-00805f9b34fb"

    ADAPTER_ALIAS = "AA Wireless Gateway Dongle"

    async def get_object(self, bus_name: str, obj_path: str):
        introspection = await self.bus.introspect(bus_name, obj_path)
        return self.bus.get_proxy_object(bus_name, obj_path, introspection)
    
    async def get_interface(self, bus_name: str, obj_path: str, interface: str):
        obj = await self.get_object(bus_name, obj_path)
        return obj.get_interface(interface)

    async def get_bluez_objects(self):
        bluez_obj_manager = await self.get_interface(self.BLUEZ_BUS_NAME, self.BLUEZ_ROOT_OBJECT_PATH, self.INTERFACE_OBJECT_MANAGER)
        return await bluez_obj_manager.call_get_managed_objects()

    async def init_adapter(self):
        bluez_objects = await self.get_bluez_objects()

        adapters = list(filter(lambda path: any([interface == self.INTERFACE_BLUEZ_ADAPTER for interface in bluez_objects[path].keys()]), bluez_objects))
        adapter_path = None
        if len(adapters):
            print("Found %d bluetooth adapters" % len(adapters))
            adapter_path = adapters[0]
            print("Using bluetooth adapter at path: %s" % adapter_path)
        else:
            print("Did not find any bluetooth adapters")
            return

        self.adapter = await self.get_interface(self.BLUEZ_BUS_NAME, adapter_path, self.INTERFACE_BLUEZ_ADAPTER)

    async def power_on(self):
        if not self.adapter:
            return

        await self.adapter.set_alias(self.ADAPTER_ALIAS)
        await self.adapter.set_powered(True)
        print("Bluetooth adapter was Powered On")
    
    async def set_pairable(self, pairable: bool):
        if not self.adapter:
            return

        await self.adapter.set_discoverable(pairable)
        await self.adapter.set_pairable(pairable)
        print("Bluetooth adapter is now discoverable and pairable")

    async def export_profile(self):
        profile_manager = await self.get_interface(self.BLUEZ_BUS_NAME, self.BLUEZ_OBJECT_PATH, self.INTERFACE_BLUEZ_PROFILE_MANAGER)

        aaw_profile = AAWirelessProfile()
        self.bus.export(self.AAWG_PROFILE_OBJECT_PATH, aaw_profile)
        await profile_manager.call_register_profile(self.AAWG_PROFILE_OBJECT_PATH, self.AAWG_PROfILE_UUID, {
            "Name": Variant("s", "AA Wireless"),
            "Role": Variant("s", "server"),
            "Channel": Variant("q", 8)
        })
        print("Bluetooth AA Wireless profile active")

        hsp_hs_profile = HSPHSProfile()
        self.bus.export(self.HSP_HS_PROFILE_OBJECT_PATH, hsp_hs_profile)
        await profile_manager.call_register_profile(self.HSP_HS_PROFILE_OBJECT_PATH, self.HSP_HS_UUID, {
            "Name": Variant("s", "HSP HS"),
        })
        print("HSP Handset profile active")

    async def conect_device(self):
        bluez_objects = await self.get_bluez_objects()
        devices = list(filter(lambda path: any([interface == self.INTERFACE_BLUEZ_DEVICE for interface in bluez_objects[path].keys()]), bluez_objects))

        device_path = None
        if len(devices):
            print("Found %d bluetooth devices" % len(devices))
            device_path = devices[0]
            print("Using bluetooth device at path: %s" % device_path)
        else:
            print("Did not find any connected bluetooth device")
            return

        device = await self.get_interface(self.BLUEZ_BUS_NAME, device_path, self.INTERFACE_BLUEZ_DEVICE)
        await device.call_connect_profile(self.HSP_AG_UUID)
        print("ConnectProfile completed")

    async def init(self):
        self.bus = await MessageBus(bus_type=BusType.SYSTEM, negotiate_unix_fd=True).connect()

        await self.init_adapter()
        await self.export_profile()
        await self.power_on()
        await self.set_pairable(True)
        await self.conect_device()

        await self.bus.wait_for_disconnect()

async def main():
    h = AABluetoothProfileHandler()
    await h.init()

asyncio.get_event_loop().run_until_complete(main())
