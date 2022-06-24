import type ImageRef from '$lib/image/ref'

export default interface Ball {
	x: number
	y: number
	radius: number
	vx: number
	vy: number
	image: ImageRef
}
