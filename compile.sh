#!/bin/bash

cd ../ns-3

./waf configure -d debug

./waf 

./waf install

cd ../test05082017

./waf configure --debug