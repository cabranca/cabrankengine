#pragma once

#include <Cabrankengine/Core/MouseCodes.h>

#include "Event.h"

namespace cbk {

	class MouseMovedEvent : public Event {
	  public:
		MouseMovedEvent(const float x, const float y) : m_MouseX(x), m_MouseY(y) {}

		// Returns the X position of the mouse in viewport coordinates
		float getX() const {
			return m_MouseX;
		}

		// Returns the X position of the mouse in viewport coordinates
		float getY() const {
			return m_MouseY;
		}

		// Returns the string form of the event
		virtual std::string toString() const override {
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	  private:
		float m_MouseX, m_MouseY; // X and Y position of the mouse in viewport coordinates
	};

	class MouseScrolledEvent : public Event {
	  public:
		MouseScrolledEvent(const float xOffset, const float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset) {}

		// Returns the scrolling offset on the X axis
		float getXOffset() const {
			return m_XOffset;
		}

		// Returns the scrolling offset on the y axis
		float getYOffset() const {
			return m_YOffset;
		}

		// Returns the string form of the event
		virtual std::string toString() const override {
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << getXOffset() << ", " << getYOffset();
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	  private:
		float m_XOffset, m_YOffset; // X and Y scrolling axis offset
	};

	class MouseButtonEvent : public Event {
	  public:
		// Returns the numeric code for the mouse button
		MouseCode getMouseButton() const {
			return m_Button;
		}

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)

	  protected:
		MouseButtonEvent(const MouseCode button) : m_Button(button) {}

		MouseCode m_Button;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent {
	  public:
		MouseButtonPressedEvent(const MouseCode button) : MouseButtonEvent(button) {}

		// Returns the string form of the event
		virtual std::string toString() const override {
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent {
	  public:
		MouseButtonReleasedEvent(const MouseCode button) : MouseButtonEvent(button) {}

		// Returns the string form of the event
		virtual std::string toString() const override {
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};

} // namespace cbk