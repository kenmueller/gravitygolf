const store: Record<string, unknown> = {}

const cache = <Value>(id: string, factory: () => Value) =>
	Object.prototype.hasOwnProperty.call(store, id)
		? (store[id] as Value)
		: (store[id] = factory())

export default cache
