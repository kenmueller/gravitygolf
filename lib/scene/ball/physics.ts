import type Ball from '.'
import type { Body } from 'matter-js'

export default interface BallWithPhysics extends Ball {
	physics: Body
}
