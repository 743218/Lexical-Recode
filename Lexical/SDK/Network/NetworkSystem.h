#pragma once
#include "RakNet/RakNet.h"
#include "../../Utils/MemoryUtil.h"

class RemoteConnectorComposite {
public:
	CLASS_MEMBER(RaknetConnector*, rakNet, 0x60);
};

class NetworkSystem {
public:
	CLASS_MEMBER(RemoteConnectorComposite*, remoteConnector, 0x80);
};
