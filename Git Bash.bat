@echo off
"C:\Program Files\Git\bin\sh.exe"
cd..
eval `ssh-agent -s`
ssh-add key
pause