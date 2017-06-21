#!/bin/bash

cd ../ns-3

./waf configure -d debug

./waf 

./waf install

cd ../test05142017

./waf configure --debug

