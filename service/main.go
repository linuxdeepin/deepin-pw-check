package main

import (
	"time"

	"github.com/linuxdeepin/go-lib/log"
)

var logger = log.NewLogger("deepin_pw_check")

func main() {
	s := newService()
	s.serviceInit()

	s.service.SetAutoQuitHandler(time.Second*30, func() bool {
		return true
	})

	s.service.Wait()
}
