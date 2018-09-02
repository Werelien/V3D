#!/bin/bash
make config=debug
rm Logs/*.Log
File="Logs/$(date -Iseconds).Debug.Log"
./bin/Debug/V3D >$File

