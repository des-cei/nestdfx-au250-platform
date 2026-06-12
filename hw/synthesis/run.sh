#!/bin/bash -e

tcl_name=$1

vivado -notrace -nojournal -nolog -mode batch -source  ${tcl_name}
