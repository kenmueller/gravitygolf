type _Func = (...args: never[]) => unknown

type Debounced<Func extends _Func> = Func & { clear(): void; flush(): void }

type Debounce = <Func extends _Func>(
	func: Func,
	interval: number,
	immediate?: boolean
) => Debounced<Func>

const debounce: Debounce = (func, wait, immediate) => {
	let timeout: NodeJS.Timeout | number | null = null
	let args: never[] | null = null
	let context: unknown = null
	let timestamp: number | null = null
	let result: unknown = null

	const later = () => {
		const last = Date.now() - (timestamp as number)

		if (last < wait && last >= 0) {
			timeout = setTimeout(later, wait - last)
		} else {
			timeout = null

			if (!immediate) {
				result = func.apply(context, args as never[])
				context = args = null
			}
		}
	}

	const debounced = function (this: unknown, ..._args) {
		// eslint-disable-next-line @typescript-eslint/no-this-alias
		context = this

		args = _args
		timestamp = Date.now()

		const callNow = immediate && !timeout
		if (!timeout) timeout = setTimeout(later, wait)
		if (callNow) {
			result = func.apply(context, args)
			context = args = null
		}

		return result
	} as Debounced<typeof func>

	debounced.clear = () => {
		if (timeout) {
			clearTimeout(timeout)
			timeout = null
		}
	}

	debounced.flush = () => {
		if (timeout) {
			result = func.apply(context, args as never[])
			context = args = null

			clearTimeout(timeout)
			timeout = null
		}
	}

	return debounced
}

export default debounce
