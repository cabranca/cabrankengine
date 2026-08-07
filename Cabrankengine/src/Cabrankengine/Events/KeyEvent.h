#pragma once

#include <Cabrankengine/Core/KeyCodes.h>

#include "Event.h"

namespace cbk {

	class KeyEvent : public Event {
	  public:
		// Returns the numeric code for the key
		KeyCode getKeyCode() const {
			return m_KeyCode;
		}

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	  protected:
		KeyEvent(const KeyCode keycode) : m_KeyCode(keycode) {}

		KeyCode m_KeyCode; // Numeric code for the key
	};

	class KeyPressedEvent : public KeyEvent {
	  public:
		KeyPressedEvent(const KeyCode keycode, bool isRepeat = false) : KeyEvent(keycode), m_IsRepeat(isRepeat) {}

		// Returns whether the key is being pressed repeatedly
		bool isRepeat() const {
			return m_IsRepeat;
		}

		// Returns the string form of the event
		virtual std::string toString() const override {
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_KeyCode << " (repeat = " << m_IsRepeat << ")";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)

	  private:
		bool m_IsRepeat; // Whether the key is being pressed repeatedly
	};

	class KeyReleasedEvent : public KeyEvent {
	  public:
		KeyReleasedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

		// Returns the string form of the event
		virtual std::string toString() const override {
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyEvent {
	  public:
		KeyTypedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

		// Returns the string form of the event
		virtual std::string toString() const override {
			std::stringstream ss;
			ss << "KeyTypedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};
} // namespace cbk
