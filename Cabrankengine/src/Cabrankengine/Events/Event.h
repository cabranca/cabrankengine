#pragma once

#include <pch.h>

#include <Cabrankengine/Core/Core.h>

namespace cbk {

	// Every type of event used must be declared here
	enum class EventType {
		None = 0,
		WindowClose,
		WindowResize,
		WindowFocus,
		WindowLostFocus,
		WindowMoved,
		AppTick,
		AppUpdate,
		AppRender,
		KeyPressed,
		KeyReleased,
		KeyTyped,
		MouseButtonPressed,
		MouseButtonReleased,
		MouseMoved,
		MouseScrolled
	};

	// Categories to filter events with bit masks
	enum EventCategory {
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4)
	};

// Macros to avoid boilerplate code regarding the event type handling
// TODO: try to apply CRTP to replace this
#define EVENT_CLASS_TYPE(type)                                                                                                             \
	static EventType getStaticType() {                                                                                                     \
		return EventType::type;                                                                                                            \
	}                                                                                                                                      \
	virtual EventType getEventType() const override {                                                                                      \
		return getStaticType();                                                                                                            \
	}                                                                                                                                      \
	virtual const char* getName() const override {                                                                                         \
		return #type;                                                                                                                      \
	}

#define EVENT_CLASS_CATEGORY(category)                                                                                                     \
	virtual int getCategoryFlags() const override {                                                                                        \
		return category;                                                                                                                   \
	}

	// TODO: could this be a struct instead of a class?
	class Event {
	  public:
		virtual ~Event() = default;

		// Returns the event type
		[[nodiscard]] virtual EventType getEventType() const = 0;

		// Returns the event name for debugging
		virtual const char* getName() const = 0;

		// Returns the category BITmask
		virtual int getCategoryFlags() const = 0;

		// Returns the string form of the event name
		virtual std::string toString() const {
			return getName();
		}

		// Returns whether the event category matches by the given category
		bool isInCategory(EventCategory category) const {
			return getCategoryFlags() & category;
		}

		// Returns whether the event was handled
		bool handled() const {
			return m_Handled;
		}

		// Sets the handled member to true
		void setHandled() {
			m_Handled = true;
		}

	  private:
		bool m_Handled = false; // This is a private member to avoid direct manipulation, use the public Handled member instead
	};

	class EventDispatcher {
	  public:
		EventDispatcher(Event& event) : m_Event(event) {}

		// Checks the event type calls the callback function. It also sets the handled member
		template <typename T, typename F>
		bool dispatch(const F& func) {
			if (m_Event.getEventType() == T::getStaticType()) {
				if (func(static_cast<T&>(m_Event)))
					m_Event.setHandled();
				return true;
			}
			return false;
		}

	  private:
		Event& m_Event; // Event to dispatch
	};

	// TODO: this does not seem to be working. It is supposed to flow the string to the stream to be logged without a function call in the
	// logger methods
	inline std::ostream& operator<<(std::ostream& os, const Event& e) {
		return os << e.toString();
	}

} // namespace cbk