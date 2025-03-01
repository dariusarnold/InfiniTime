#!/usr/bin/env bash

set euo -pipefail

cmake --preset pinetime-release
cmake --build --preset pinetime-release
