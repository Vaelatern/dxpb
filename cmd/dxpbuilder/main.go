package main

import (
	"log"

	"github.com/spf13/pflag"
	"github.com/spf13/viper"

	"github.com/dxpb/dxpb/internal/builder"
)

func startConfig() {
	pflag.BoolP("debug", "d", false, "Debug mode")
	pflag.StringP("listen", "l", ":9191", "Listen port")
	pflag.StringP("config", "c", "dxpbuilder", "Config file")
	pflag.StringP("arch", "a", "noarch", "xbps-src architecture")
	pflag.StringP("hostdir", "h", "./", "Hostdir path")
	pflag.Parse()
	viper.BindPFlags(pflag.CommandLine)
	viper.SetConfigName(viper.GetString("config"))
	viper.AddConfigPath("/etc/dxpb")
	viper.AddConfigPath(".")
	err := viper.ReadInConfig()
	if err != nil {
		log.Panicf("Could not read in configuration file: %s\n", err)
	}
}

func main() {
	log.Println("hello!")
	startConfig()

	builder.ListenAndWork(viper.GetString("listen"))
}
