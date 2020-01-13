#!/bin/bash
git config --global alias.updver "! f() { . ./increment.sh && updver \"\$@\"; }; f"
git config --global alias.curver "! f() { . ./increment.sh && curver; }; f"
