import type Position from '$lib/position'
import type ResizableWall from './resizable'

export default interface WallCorner extends Position {
	position: 'NE' | 'NW' | 'SE' | 'SW'
	wall: ResizableWall
}
