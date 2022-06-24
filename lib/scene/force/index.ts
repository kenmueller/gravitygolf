import type ImageRef from '$lib/image/ref'

export default interface Force {
	x: number
	y: number
	direction: 1 | -1
	image: ImageRef
}
