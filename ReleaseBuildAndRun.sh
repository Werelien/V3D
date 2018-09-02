#!/bin/bash
make config=release
File="Logs/$(date -Iseconds).Release.Log"
bin/Release/V3D > $File
