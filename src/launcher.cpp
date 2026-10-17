/*
 * Copyright (c) 2024, shAdE424
 * All rights reserved.
 *
 * This file is part of Rex, licensed under the BSD 3-Clause License.
 * See the LICENSE file at the root of this repository for full details.
 */

#include "rex.hpp"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        Rex rex;
        rex.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Rex: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
