import type Position from '$lib/position'
import type ImageRef from '$lib/image/ref'

export default interface Ball extends Position {
	radius: number
	coeff_restitution: number
	vx: number
	vy: number
	image: ImageRef
}
