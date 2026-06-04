#include <pch.h>
#include "MetalDeviceContext.h"

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace cbk::platform::metal {

	MetalDeviceContext::~MetalDeviceContext() {
		shutdown();
	}

	void MetalDeviceContext::init() {
		m_Device = MTL::CreateSystemDefaultDevice();
		m_CommandQueue = m_Device->newCommandQueue();

		CBK_CORE_INFO("Metal Context (C++) - GPU: {0}", m_Device->name()->utf8String());
	}

	void MetalDeviceContext::shutdown() {
		if (m_CommandQueue)
			m_CommandQueue->release();
		if (m_Device)
			m_Device->release();
	}

	MTL::Device* MetalDeviceContext::getDevice() const {
		return m_Device;
	}

	MTL::CommandQueue* MetalDeviceContext::getCommandQueue() const {
		return m_CommandQueue;
	}
} // namespace cbk::platform::metal
