#pragma once

#include <Cabrankengine/Core/Window.h>

struct GLFWwindow; // Forward declaration of the GLFWwindow struct

namespace cbk {

    namespace rendering {
        class GraphicsContext; // Forward declaration of the GraphicsContext class
    }

    class LinuxWindow : public Window {
        public:
            LinuxWindow(const WindowProps& props);
            virtual ~LinuxWindow();

            // Callback for the window update
            virtual void onUpdate() override;

            // Returns the window width
            virtual int getWidth() const override { return m_Data.Width; }
        
            // Returns the window height
            virtual int getHeight() const override { return m_Data.Height; }

            // Sets the callback function for the event polling
            virtual void setEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
        
            // Sets whether VSync is enabled or not
            virtual void setVSync(bool enabled) override;
        
            // Returns whether VSync is enabled or not
            virtual bool isVSync() const override;

            rendering::GraphicsContext* getContext() const override;

            // Returns the Windows specific window.
		    // TODO: I think it must be marked override
		    virtual void* getNativeWindow() const override { return m_Window; }

        private:
            // Initialize the window from the given properties
            void init(const WindowProps& props);
        
            // Shutsdown GLFW correctly to destroy the window
            void shutdown();

            GLFWwindow* m_Window; // Actual window object
		    Scope<rendering::GraphicsContext> m_Context; // Graphics context for rendering

            // Data of the window to work with GLFW
            struct WindowData {
                std::string Title;
                int Width, Height;
                bool VSync;

                EventCallbackFn EventCallback;
            };

            WindowData m_Data; // Current window data
    };

}
