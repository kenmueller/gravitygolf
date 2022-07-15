import type SceneEvents from '../events'
import type ResizableWall from '../wall/resizable'

export default interface EditorEvents extends SceneEvents {
	clear: []
	fixedStars: [number]
	wall: [ResizableWall | null]
}
