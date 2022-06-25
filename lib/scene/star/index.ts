import type Position from '$lib/position'
import type ImageRef from '$lib/image/ref'

export default interface Star extends Position {
	radius: number
	hit: boolean
	image: ImageRef
}
