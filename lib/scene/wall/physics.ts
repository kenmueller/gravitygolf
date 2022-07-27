import type Wall from '.'
import type { Body } from 'matter-js'

export default interface WallWithPhysics extends Wall {
	physics: Body
}
