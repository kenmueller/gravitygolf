import type SceneEvents from '../events'
import type ResizableWall from '../wall/resizable'

export default interface EditorEvents extends Omit<SceneEvents, 'win'> {
	clear: []
	reset: []
	fixedStars: [number]
	wall: [ResizableWall | null]

	/** If the win overlay is showing */
	win: [boolean]
}
