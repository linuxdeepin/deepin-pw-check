#include <systemd/sd-bus.h>
#include <stdio.h>
#include "debug.h"

#define DBUS_SERVICE "com.deepin.daemon.Passwd"
#define DBUS_PATH "/com/deepin/daemon/Passwd"
#define DBUS_INTERFACE "com.deepin.daemon.Passwd"


int get_user_hash(const char* user,char* hash) {
    sd_bus *bus = NULL;
    sd_bus_error err = SD_BUS_ERROR_NULL;
    sd_bus_message* reply = NULL;
    char* res;
    if (user == NULL || hash == NULL){
        return -1;
    }

    int ret = sd_bus_open_system(&bus);
    if (ret < 0) {
        return ret;
    }
    do {
        DEBUG("call dbus method GetPasswdHash");
        ret = sd_bus_call_method(bus, DBUS_SERVICE, DBUS_PATH , DBUS_INTERFACE ,
                            "GetPasswdHash", &err , &reply , "s" , user);
        if (ret < 0) {
            ret = -2;
            break;
        }

        ret = sd_bus_message_read(reply, "s", &res);
        if (ret < 0) {
            break;
        }
        sprintf(hash,"%s",res);

    }while(0);

    sd_bus_error_free(&err);
    sd_bus_message_unref(reply);

    return ret;
}