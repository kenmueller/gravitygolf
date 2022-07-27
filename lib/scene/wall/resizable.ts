import type Wall from '.'
import type WallCorner from './corner'
import type { Body } from 'matter-js'

export default interface ResizableWall extends Wall {
	corners: WallCorner[]
	physics: Body
}
