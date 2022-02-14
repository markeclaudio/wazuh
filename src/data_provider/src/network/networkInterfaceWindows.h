/*
 * Wazuh SYSINFO
 * Copyright (C) 2015, Wazuh Inc.
 * November 3, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef _NETWORK_INTERFACE_WINDOWS_H
#define _NETWORK_INTERFACE_WINDOWS_H

#include "inetworkInterface.h"
#include "inetworkWrapper.h"

class FactoryWindowsNetwork
{
    public:
        static std::shared_ptr<IOSNetwork> create(const std::shared_ptr<INetworkInterfaceWrapper>& interfaceAddress);
};

template <int osNetworkType>
class WindowsNetworkImpl final : public IOSNetwork
{
        std::shared_ptr<INetworkInterfaceWrapper> m_interfaceAddress;
    public:
        explicit WindowsNetworkImpl(const std::shared_ptr<INetworkInterfaceWrapper>& interfaceAddress)
            : m_interfaceAddress(interfaceAddress)
        { }
        // LCOV_EXCL_START
        ~WindowsNetworkImpl() = default;
        // LCOV_EXCL_STOP
        void buildNetworkData(nlohmann::json& /*network*/) override
        {
            throw std::runtime_error { "Non implemented specialization." };
        }
};

#endif // _NETWORK_INTERFACE_WINDOWS_H