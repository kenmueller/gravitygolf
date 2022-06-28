import { get } from 'svelte/store'

import type Position from './position'
import mobile from './mobile'
import cursorHandler from './cursor/handler'

const draggable = (
	element: HTMLElement,
	onDrop: ((position: Position) => void) | null
): SvelteActionReturnType => {
	const $mobile = get(mobile)

	let elementStart: Position | null = null

	let start: Position | null = null
	let current: Position | null = null

	let copy: typeof element | null = null

	const down = cursorHandler(({ x, y }, event) => {
		event.preventDefault()

		if (!onDrop) return

		elementStart = element.getBoundingClientRect()
		start = current = { x, y }

		copy = element.cloneNode(true) as typeof element

		copy.dataset.copy = ''

		copy.style.position = 'absolute'

		copy.style.left = `${elementStart.x}px`
		copy.style.top = `${elementStart.y}px`

		document.body.append(copy)
	})

	const move = cursorHandler(({ x, y }) => {
		if (!(onDrop && elementStart && start && copy)) return

		current = { x, y }

		copy.style.left = `${elementStart.x + (current.x - start.x)}px`
		copy.style.top = `${elementStart.y + (current.y - start.y)}px`
	})

	const up = () => {
		if (!(onDrop && elementStart && start && current && copy)) return

		onDrop({
			x: elementStart.x + (current.x - start.x),
			y: elementStart.y + (current.y - start.y)
		})

		elementStart = start = current = null
		copy.remove()
	}

	if ($mobile) {
		element.addEventListener('touchstart', down)
		document.addEventListener('touchmove', move)
		document.addEventListener('touchend', up)
	} else {
		element.addEventListener('mousedown', down)
		document.addEventListener('mousemove', move)
		document.addEventListener('mouseup', up)
	}

	return {
		update: (newOnDrop: typeof onDrop) => {
			onDrop = newOnDrop
		},
		destroy: () => {
			if ($mobile) {
				element.removeEventListener('touchstart', down)
				document.removeEventListener('touchmove', move)
				document.removeEventListener('touchend', up)
			} else {
				element.removeEventListener('mousedown', down)
				document.removeEventListener('mousemove', move)
				document.removeEventListener('mouseup', up)
			}
		}
	}
}

export default draggable
