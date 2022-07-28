import type Wall from '.'
import type WallCorner from './corner'

export default interface ResizableWall extends Wall {
	corners: WallCorner[]
}
