import type Cursor from '.'
import type CursorEvent from './event'
import toCursor from './to'

const cursorHandler =
	(handler: (cursor: Cursor, event: CursorEvent) => void) =>
	(event: CursorEvent) => {
		const cursor = toCursor(event)
		if (cursor) return handler(cursor, event)
	}

export default cursorHandler
