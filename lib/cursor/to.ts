import type Cursor from '.'
import type CursorEvent from './event'

const toCursor = (event: CursorEvent): Cursor | null => {
	if (!('touches' in event))
		return {
			x: event.clientX,
			y: event.clientY,
			button: event.button,
			mouse: true
		}

	if (event.changedTouches.length !== 1) return null
	const touch = event.changedTouches[0]

	return {
		x: touch.clientX,
		y: touch.clientY,
		button: 0,
		mouse: false
	}
}

export default toCursor
