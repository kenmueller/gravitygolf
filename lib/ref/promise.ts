import type Ref from '.'

const ref = <Current>(promise: Promise<Current>) => {
	const ref: Ref<Current | null> = { current: null }

	promise
		.then(current => {
			ref.current = current
		})
		.catch(console.error)

	return ref
}

export default ref
