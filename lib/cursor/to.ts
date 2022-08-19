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

	const touch = event.touches[0]
	if (!touch) return null

	return {
		x: touch.clientX,
		y: touch.clientY,
		button: 0,
		mouse: false
	}
}

export default toCursor
