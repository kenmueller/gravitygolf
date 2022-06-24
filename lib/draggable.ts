const draggable = (
	element: HTMLElement,
	onDrop: ((position: { x: number; y: number }) => void) | null
): SvelteActionReturnType => {
	let elementStart: { x: number; y: number } | null = null

	let start: { x: number; y: number } | null = null
	let current: { x: number; y: number } | null = null

	let copy: typeof element | null = null

	const down = ({ clientX: x, clientY: y }: MouseEvent) => {
		if (!onDrop) return

		elementStart = element.getBoundingClientRect()
		start = current = { x, y }

		copy = element.cloneNode(true) as typeof element

		copy.dataset.copy = ''

		copy.style.position = 'absolute'

		copy.style.left = `${elementStart.x}px`
		copy.style.top = `${elementStart.y}px`

		document.body.append(copy)
	}

	const move = ({ clientX: x, clientY: y }: MouseEvent) => {
		if (!(onDrop && elementStart && start && copy)) return

		current = { x, y }

		copy.style.left = `${elementStart.x + (current.x - start.x)}px`
		copy.style.top = `${elementStart.y + (current.y - start.y)}px`
	}

	const up = () => {
		if (!(onDrop && elementStart && start && current && copy)) return

		onDrop({
			x: elementStart.x + (current.x - start.x),
			y: elementStart.y + (current.y - start.y)
		})

		elementStart = start = current = null
		copy.remove()
	}

	element.addEventListener('mousedown', down)
	document.addEventListener('mousemove', move)
	document.addEventListener('mouseup', up)

	return {
		update: (newOnDrop: typeof onDrop) => {
			onDrop = newOnDrop
		},
		destroy: () => {
			element.removeEventListener('mousedown', down)
			document.removeEventListener('mousemove', move)
			document.removeEventListener('mouseup', up)
		}
	}
}

export default draggable
