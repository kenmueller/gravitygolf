type AnyListener<
	Events extends {
		[event in keyof Events]: unknown[]
	}
> = (...args: Events[keyof Events]) => void

export default class EventDispatcher<
	Events extends {
		[event in keyof Events]: unknown[]
	}
> {
	private readonly listeners = new Map<AnyListener<Events>, keyof Events>()

	protected readonly dispatchEvent = <Event extends keyof Events>(
		event: Event,
		...args: Events[Event]
	) => {
		for (const [listener, listenerEvent] of this.listeners)
			if (listenerEvent === event) listener(...args)
	}

	readonly addEventListener = <Event extends keyof Events>(
		event: Event,
		listener: (...args: Events[Event]) => void
	) => {
		this.listeners.set(listener as AnyListener<Events>, event)
	}

	readonly removeEventListener = <Event extends keyof Events>(
		_event: Event,
		listener: (...args: Events[Event]) => void
	) => {
		this.listeners.delete(listener as AnyListener<Events>)
	}

	readonly removeAllEventListeners = () => {
		this.listeners.clear()
	}
}
