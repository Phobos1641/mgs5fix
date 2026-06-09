#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <intrin.h>
#include <winternl.h>

#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <numbers>
#include <iostream>
#include <algorithm>
#include <format>

#include <safetyhook.hpp>
#include "logger.h"