package main

/*
#include <shadow.h>
typedef struct spwd cspwd;
*/
import "C"
import (
	"fmt"
	"sync"

	"github.com/godbus/dbus"
	"pkg.deepin.io/lib/dbusutil"
)

const (
	dBusServiceName   = "com.deepin.daemon.Passwd"
	dBusServicePath   = "/com/deepin/daemon/Passwd"
	dBusInterfaceName = dBusServiceName
)

type manager struct {
	service *dbusutil.Service
	mu      sync.Mutex

	methods *struct {
		GetPasswdHash func() `in:"user" out:"pw"`
	}
}

func newService() *manager {
	return &manager{}
}

func (m *manager) GetInterfaceName() string {
	return dBusInterfaceName
}

func (m *manager) serviceInit() {
	service, err := dbusutil.NewSystemService()
	if err != nil {
		logger.Warning(err)
		return
	}

	err = service.RequestName(dBusServiceName)
	if err != nil {
		logger.Warning(err)
		return
	}

	m.service = service
	service.Export(dBusServicePath, m)
}

func (m *manager) GetPasswdHash(user string) (string, *dbus.Error) {
	m.mu.Lock()
	defer m.mu.Unlock()

	var pw *C.cspwd
	pw = C.getspnam(C.CString(user))
	if pw == nil {
		return "", dbusutil.ToError(fmt.Errorf("get passwd for %s failed", user))
	}
	return C.GoString(pw.sp_pwdp), nil
}
