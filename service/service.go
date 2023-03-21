// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

package main

import (
	"fmt"
	"io/ioutil"

	polkit "github.com/linuxdeepin/go-dbus-factory/system/org.freedesktop.policykit1"

	"github.com/linuxdeepin/go-lib/keyfile"

	"github.com/godbus/dbus/v5"
	"github.com/linuxdeepin/go-lib/dbusutil"
)

const (
	dBusServiceName   = "org.deepin.dde.PasswdConf1"
	dBusServicePath   = "/org/deepin/dde/PasswdConf1"
	dBusInterfaceName = dBusServiceName
)

const (
	pwdConfFile                = "/etc/deepin/dde.conf"
	pwdConfBackupFile          = "/etc/deepin/dde.conf.bak"
	pwdConfSection             = "Password"
	pwdConfKeyStrongPassword   = "STRONG_PASSWORD"
	pwdConfKeyMinLength        = "PASSWORD_MIN_LENGTH"
	pwdConfKeyMaxLength        = "PASSWORD_MAX_LENGTH"
	pwdConfKeyValidatePolicy   = "VALIDATE_POLICY"
	pwdConfKeyValidateRequired = "VALIDATE_REQUIRED"
	pwdConfKeyFirstLetterUpper = "FIRST_LETTER_UPPERCASE"
)

const (
	modifyPwdConfActionId = "org.deepin.dde.passwdconf.modify-config"
)

type manager struct {
	service *dbusutil.Service

	methods *struct {
		WriteConfig         func() `in:"data"`
		ReadConfig          func() `out:"data"`
		GetLengthLimit      func() `out:"minLength,maxLength"`
		SetLengthLimit      func() `in:"minLength,maxLength"`
		GetValidatePolicy   func() `out:"validatePolicy"`
		SetValidatePolicy   func() `in:"validatePolicy"`
		GetValidateRequired func() `out:"validateRequired"`
		SetValidateRequired func() `in:"validateRequired"`
		SetEnabled          func() `in:"enabled"`
		GetEnabled          func() `out:"enabled"`
		SetFirstLetterUpper func() `in:"enabled"`
		GetFirstLetterUpper func() `out:"enabled"`
		Reset               func()
		Backup              func()
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
	service.Export(dBusServicePath, m)
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

}

func checkAuth(actionId string, sysBusName string) error {
	success, err := checkAuthByPolkit(actionId, sysBusName)
	if err != nil {
		return err
	}

	if !success {
		return fmt.Errorf("exec command failed")
	}

	return nil
}

func checkAuthByPolkit(actionId string, sysBusName string) (bool, error) {
	systemBus, err := dbus.SystemBus()
	if err != nil {
		return false, err
	}
	authority := polkit.NewAuthority(systemBus)
	subject := polkit.MakeSubject(polkit.SubjectKindSystemBusName)
	subject.SetDetail("name", sysBusName)

	ret, err := authority.CheckAuthorization(0, subject,
		actionId, nil,
		polkit.CheckAuthorizationFlagsAllowUserInteraction, "")
	if err != nil {
		return false, err
	}
	return ret.IsAuthorized, nil
}

func (m *manager) WriteConfig(sender dbus.Sender, data string) *dbus.Error {
	err := checkAuth(modifyPwdConfActionId, string(sender))
	if err != nil {
		return dbusutil.ToError(err)
	}

	kf := keyfile.NewKeyFile()
	err = kf.LoadFromData([]byte(data))
	if err == nil {
		err = ioutil.WriteFile(pwdConfFile, []byte(data), 0644)
	}
	return dbusutil.ToError(err)
}

func (m *manager) ReadConfig(sender dbus.Sender) (string, *dbus.Error) {
	err := checkAuth(modifyPwdConfActionId, string(sender))
	if err != nil {
		return "", dbusutil.ToError(err)
	}

	kf := keyfile.NewKeyFile()
	err = kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return "", dbusutil.ToError(err)
	}
	bytes, err := ioutil.ReadFile(pwdConfFile)
	return string(bytes), dbusutil.ToError(err)
}

func (m *manager) GetLengthLimit() (int, int, *dbus.Error) {
	kf := keyfile.NewKeyFile()
	err := kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return 0, 0, dbusutil.ToError(err)
	}
	minLength, err := kf.GetInt(pwdConfSection, pwdConfKeyMinLength)
	if err != nil {
		return 0, 0, dbusutil.ToError(err)
	}
	maxLength, err := kf.GetInt(pwdConfSection, pwdConfKeyMaxLength)
	if err != nil {
		return 0, 0, dbusutil.ToError(err)
	}
	return minLength, maxLength, nil

}

func (m *manager) SetLengthLimit(sender dbus.Sender, minLength, maxLength int) *dbus.Error {
	err := checkAuth(modifyPwdConfActionId, string(sender))
	if err != nil {
		return dbusutil.ToError(err)
	}

	kf := keyfile.NewKeyFile()
	err = kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return dbusutil.ToError(err)
	}
	kf.SetInt(pwdConfSection, pwdConfKeyMinLength, minLength)
	kf.SetInt(pwdConfSection, pwdConfKeyMaxLength, maxLength)
	err = kf.SaveToFile(pwdConfFile)
	return dbusutil.ToError(err)
}

func (m *manager) GetValidatePolicy() (string, *dbus.Error) {
	kf := keyfile.NewKeyFile()
	err := kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return "", dbusutil.ToError(err)
	}
	validatePolicy, err := kf.GetString(pwdConfSection, pwdConfKeyValidatePolicy)
	if err != nil {
		return "", dbusutil.ToError(err)
	}
	return validatePolicy, nil
}

func (m *manager) SetValidatePolicy(sender dbus.Sender, validatePolicy string) *dbus.Error {
	err := checkAuth(modifyPwdConfActionId, string(sender))
	if err != nil {
		return dbusutil.ToError(err)
	}

	kf := keyfile.NewKeyFile()
	err = kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return dbusutil.ToError(err)
	}
	kf.SetString(pwdConfSection, pwdConfKeyValidatePolicy, validatePolicy)
	err = kf.SaveToFile(pwdConfFile)
	return dbusutil.ToError(err)
}

func (m *manager) GetValidateRequired() (int, *dbus.Error) {
	kf := keyfile.NewKeyFile()
	err := kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return 0, dbusutil.ToError(err)
	}
	validateRequired, err := kf.GetInt(pwdConfSection, pwdConfKeyValidateRequired)
	if err != nil {
		return 0, dbusutil.ToError(err)
	}
	return validateRequired, nil
}

func (m *manager) SetValidateRequired(sender dbus.Sender, validateRequired int) *dbus.Error {
	err := checkAuth(modifyPwdConfActionId, string(sender))
	if err != nil {
		return dbusutil.ToError(err)
	}

	kf := keyfile.NewKeyFile()
	err = kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return dbusutil.ToError(err)
	}
	kf.SetInt(pwdConfSection, pwdConfKeyValidateRequired, validateRequired)
	err = kf.SaveToFile(pwdConfFile)
	return dbusutil.ToError(err)
}

func (m *manager) SetEnabled(sender dbus.Sender, enable bool) *dbus.Error {
	err := checkAuth(modifyPwdConfActionId, string(sender))
	if err != nil {
		return dbusutil.ToError(err)
	}
	kf := keyfile.NewKeyFile()
	err = kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return dbusutil.ToError(err)
	}
	kf.SetBool(pwdConfSection, pwdConfKeyStrongPassword, enable)
	err = kf.SaveToFile(pwdConfFile)
	return dbusutil.ToError(err)
}

func (m *manager) GetEnabled() (bool, *dbus.Error) {
	kf := keyfile.NewKeyFile()
	err := kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return false, dbusutil.ToError(err)
	}
	enable, err := kf.GetBool(pwdConfSection, pwdConfKeyStrongPassword)
	if err != nil {
		return false, dbusutil.ToError(err)
	}
	return enable, nil
}

func (m *manager) SetFirstLetterUpper(sender dbus.Sender, enable bool) *dbus.Error {
	err := checkAuth(modifyPwdConfActionId, string(sender))
	if err != nil {
		return dbusutil.ToError(err)
	}
	kf := keyfile.NewKeyFile()
	err = kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return dbusutil.ToError(err)
	}
	kf.SetBool(pwdConfSection, pwdConfKeyFirstLetterUpper, enable)
	err = kf.SaveToFile(pwdConfFile)
	return dbusutil.ToError(err)
}

func (m *manager) GetFirstLetterUpper() (bool, *dbus.Error) {
	kf := keyfile.NewKeyFile()
	err := kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return false, dbusutil.ToError(err)
	}
	enable, err := kf.GetBool(pwdConfSection, pwdConfKeyFirstLetterUpper)
	if err != nil {
		return false, dbusutil.ToError(err)
	}
	return enable, nil
}

func (m *manager) Reset(sender dbus.Sender) *dbus.Error {
	err := checkAuth(modifyPwdConfActionId, string(sender))
	if err != nil {
		return dbusutil.ToError(err)
	}

	kf := keyfile.NewKeyFile()
	err = kf.LoadFromFile(pwdConfBackupFile)
	if err != nil {
		return dbusutil.ToError(err)
	}
	err = kf.SaveToFile(pwdConfFile)
	return dbusutil.ToError(nil)
}

func (m *manager) Backup(sender dbus.Sender) *dbus.Error {
	err := checkAuth(modifyPwdConfActionId, string(sender))
	if err != nil {
		return dbusutil.ToError(err)
	}

	kf := keyfile.NewKeyFile()
	err = kf.LoadFromFile(pwdConfFile)
	if err != nil {
		return dbusutil.ToError(err)
	}
	err = kf.SaveToFile(pwdConfBackupFile)
	return dbusutil.ToError(nil)
}
