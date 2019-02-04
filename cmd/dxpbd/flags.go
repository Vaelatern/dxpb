package main

import (
	"log"

	"github.com/spf13/viper"
	"github.com/spf13/pflag"
)

func Start() {
	pflag.BoolP("debug", "d", false, "Debug mode")
	pflag.Parse()
	viper.BindPFlags(pflag.CommandLine)
	viper.SetDefault("githubhook", map[string]string{"bind": ":8080", "path": "/github", "secret": ""})
	viper.SetDefault("irc", map[string]string{"nick": "dxpbd", "server": "irc.example.com:6697", "channel": "#dxpb"})
	viper.SetDefault("irc.ssl", true)
	viper.SetConfigName("dxpbd")
	viper.AddConfigPath("/etc/dxpb")
	viper.AddConfigPath(".")
	err := viper.ReadInConfig()
	if err != nil {
		log.Panicf("Could not read in configuration file: %s\n", err)
	}
}
