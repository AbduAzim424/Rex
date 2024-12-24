#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <limits>

#include <unistd.h>
#include <string.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xinput.h>
#include <xcb/xproto.h>

#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>
#include <pango/pangocairo.h>