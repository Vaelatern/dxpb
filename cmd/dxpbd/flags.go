package main

import (
	"github.com/spf13/pflag"
	"github.com/spf13/viper"
)

func Start() error {
	pflag.StringP("output-conf", "O", "", "Output sample configuration file")
	pflag.BoolP("debug", "d", false, "Debug mode")
	pflag.Parse()
	viper.BindPFlags(pflag.CommandLine)
	viper.SetDefault("githubhook", map[string]string{"bind": ":8080", "path": "/github", "secret": ""})
	viper.SetConfigName("dxpbd")
	viper.AddConfigPath("/etc/dxpb")
	viper.AddConfigPath(".")
	return viper.ReadInConfig()
}
