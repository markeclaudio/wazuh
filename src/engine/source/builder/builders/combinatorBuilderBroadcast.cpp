/* Copyright (C) 2015-2021, Wazuh Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "combinatorBuilderBroadcast.hpp"

#include <vector>

namespace builder::internals::builders
{

base::Lifter combinatorBuilderBroadcast(const std::vector<base::Lifter> & lifters)
{
    return [=](base::Observable input) -> base::Observable
    {
        input = input.publish().ref_count();
        std::vector<base::Observable> inputs;
        for (auto op : lifters)
        {
            inputs.push_back(op(input));
        }
        return rxcpp::observable<>::iterate(inputs).merge();
    };
}

} // namespace builder::internals::builders
