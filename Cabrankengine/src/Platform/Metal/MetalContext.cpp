#include <pch.h>
#include "MetalContext.h"

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

// Para interactuar con la ventana de GLFW (Cocoa) desde C++
#include <objc/runtime.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "MetalRendererAPI.h"

namespace cbk::platform::metal {

	MetalContext::MetalContext(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle) {
		CBK_CORE_ASSERT(m_WindowHandle, "Window handle is null!");

		m_Device = MTL::CreateSystemDefaultDevice();
	}

	MetalContext::~MetalContext() {
		if (m_Swapchain)
			m_Swapchain->release();
		if (m_CommandQueue)
			m_CommandQueue->release();
		if (m_Device)
			m_Device->release();
	}

	void MetalContext::init() {
		CBK_PROFILE_FUNCTION();
		CBK_CORE_INFO("Metal Context (C++) - GPU: {0}", m_Device->name()->utf8String());

		m_CommandQueue = m_Device->newCommandQueue();

		// --- MAGIA para conectar GLFW con Metal-cpp sin usar .mm ---
		// 1. Obtener la NSWindow (void*)
		void* nswindow = glfwGetCocoaWindow(m_WindowHandle);

		// 2. Obtener la contentView: [nswindow contentView]
		// Usamos objc_msgSend para llamar métodos de ObjC desde C++
		void* contentView = ((void* (*)(id, SEL))objc_msgSend)((id)nswindow, sel_registerName("contentView"));

		// 3. Configurar el Layer
		m_Swapchain = CA::MetalLayer::layer();
		m_Swapchain->setDevice(m_Device);
		m_Swapchain->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

		// 4. Asignar el layer: [view setLayer:layer] y [view setWantsLayer:YES]
		((void (*)(id, SEL, void*))objc_msgSend)((id)contentView, sel_registerName("setLayer:"), (void*)m_Swapchain);
		((void (*)(id, SEL, BOOL))objc_msgSend)((id)contentView, sel_registerName("setWantsLayer:"), YES);
		// ------------------------------------------------------------
	}

	void MetalContext::shutdown() {
		
	}

	void MetalContext::swapBuffers() {
		
		// Liberamos el drawable del frame actual para que el próximo frame pida uno nuevo
		if (m_CurrentDrawable) {
			m_CurrentDrawable->release();
			m_CurrentDrawable = nullptr;
		}
	}

	CA::MetalDrawable* MetalContext::getCurrentDrawable() {
		// Si ya tenemos uno asignado para este frame, lo devolvemos
        if (m_CurrentDrawable) 
            return m_CurrentDrawable;

        // Si no, pedimos uno nuevo y lo guardamos
        m_CurrentDrawable = m_Swapchain->nextDrawable();
        
        // (Opcional pero recomendado) Si m_CurrentDrawable es null, podrías loguear un warning.
        return m_CurrentDrawable;
	}
} // namespace cbk::platform::metal
