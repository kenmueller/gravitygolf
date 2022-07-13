import type SceneEvents from '../events'
import type ResizableWall from '../wall/resizable'

export default interface EditorEvents extends SceneEvents {
	clear: []
	defaultStars: [number]
	wall: [ResizableWall | null]
}
