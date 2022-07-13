import type Position from '$lib/position'
import type WallCorner from './corner'

export default interface ResizableWall extends Position {
	width: number
	height: number
	corners: WallCorner[]
}
