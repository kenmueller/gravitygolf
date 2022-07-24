import type Position from '$lib/position'
import type { Body } from 'matter-js'

export default interface Wall extends Position {
	width: number
	height: number
	physics: Body
}
