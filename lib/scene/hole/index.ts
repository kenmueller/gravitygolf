import type ImageRef from '$lib/image/ref'

export default interface Hole {
	x: number
	y: number
	radius: number
	image: ImageRef
}
